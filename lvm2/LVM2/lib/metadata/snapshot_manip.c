/*
 * Copyright (C) 2002-2004 Sistina Software, Inc. All rights reserved.
 * Copyright (C) 2004-2006 Red Hat, Inc. All rights reserved.
 *
 * This file is part of LVM2.
 *
 * This copyrighted material is made available to anyone wishing to use,
 * modify, copy, or redistribute it subject to the terms and conditions
 * of the GNU Lesser General Public License v.2.1.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "lib.h"
#include "metadata.h"
#include "locking.h"
#include "toolcontext.h"
#include "lv_alloc.h"
#include "activate.h"

int lv_is_origin(const struct logical_volume *lv)
{
	return lv->origin_count ? 1 : 0;
}

int lv_is_cow(const struct logical_volume *lv)
{
	return (!lv_is_origin(lv) && lv->snapshot) ? 1 : 0;
}

int lv_is_visible(const struct logical_volume *lv)
{
	if (lv->status & SNAPSHOT)
		return 0;

	if (lv_is_cow(lv)) {
		if (lv_is_virtual_origin(origin_from_cow(lv)))
			return 1;

		if (lv_is_merging_cow(lv))
			return 0;

		return lv_is_visible(origin_from_cow(lv));
	}

	return lv->status & VISIBLE_LV ? 1 : 0;
}

int lv_is_virtual_origin(const struct logical_volume *lv)
{
	return (lv->status & VIRTUAL_ORIGIN) ? 1 : 0;
}

int lv_is_merging_origin(const struct logical_volume *origin)
{
	return (origin->status & MERGING) ? 1 : 0;
}

struct lv_segment *find_merging_cow(const struct logical_volume *origin)
{
	if (!lv_is_merging_origin(origin))
		return NULL;

	return find_cow(origin);
}

int lv_is_merging_cow(const struct logical_volume *snapshot)
{
	/* checks lv_segment's status to see if cow is merging */
	return (find_cow(snapshot)->status & MERGING) ? 1 : 0;
}

/* Given a cow LV, return the snapshot lv_segment that uses it */
struct lv_segment *find_cow(const struct logical_volume *lv)
{
	return lv->snapshot;
}

/* Given a cow LV, return its origin */
struct logical_volume *origin_from_cow(const struct logical_volume *lv)
{
	if (lv->snapshot)
		return lv->snapshot->origin;
	return NULL;
}

void init_snapshot_seg(struct lv_segment *seg, struct logical_volume *origin,
		       struct logical_volume *cow, uint32_t chunk_size, int merge)
{
	seg->chunk_size = chunk_size;
	seg->origin = origin;
	seg->cow = cow;

	lv_set_hidden(cow);

	cow->snapshot = seg;

	origin->origin_count++;

	/* FIXME Assumes an invisible origin belongs to a sparse device */
	if (!lv_is_visible(origin))
		origin->status |= VIRTUAL_ORIGIN;

	seg->lv->status |= (SNAPSHOT | VIRTUAL);
	if (merge)
		init_snapshot_merge(seg, origin);

	dm_list_add(&origin->snapshot_segs, &seg->origin_list);
}

void init_snapshot_merge(struct lv_segment *cow_seg,
			 struct logical_volume *origin)
{
	/*
	 * Even though lv_is_visible(cow_seg->lv) returns 0,
	 * the cow_seg->lv (name: snapshotX) is _not_ hidden;
	 * this is part of the lvm2 snapshot fiction.  Must
	 * clear VISIBLE_LV directly (lv_set_visible can't)
	 * - cow_seg->lv->status is used to control whether 'lv'
	 *   (with user provided snapshot LV name) is visible
	 * - this also enables vg_validate() to succeed with
	 *   merge metadata (cow_seg->lv is now "internal")
	 */
	cow_seg->lv->status &= ~VISIBLE_LV;
	cow_seg->status |= MERGING;
	origin->snapshot = cow_seg;
	origin->status |= MERGING;
}

void clear_snapshot_merge(struct logical_volume *origin)
{
	/* clear merge attributes */
	origin->snapshot->status &= ~MERGING;
	origin->snapshot = NULL;
	origin->status &= ~MERGING;
}

int vg_add_snapshot(struct logical_volume *origin,
		    struct logical_volume *cow, union lvid *lvid,
		    uint32_t extent_count, uint32_t chunk_size)
{
	struct logical_volume *snap;
	struct lv_segment *seg;

	/*
	 * Is the cow device already being used ?
	 */
	if (lv_is_cow(cow)) {
		log_error("'%s' is already in use as a snapshot.", cow->name);
		return 0;
	}

	if (cow == origin) {
		log_error("Snapshot and origin LVs must differ.");
		return 0;
	}

	if (!(snap = lv_create_empty("snapshot%d",
				     lvid, LVM_READ | LVM_WRITE | VISIBLE_LV,
				     ALLOC_INHERIT, origin->vg)))
		return_0;

	snap->le_count = extent_count;

	if (!(seg = alloc_snapshot_seg(snap, 0, 0)))
		return_0;

	init_snapshot_seg(seg, origin, cow, chunk_size, 0);

	return 1;
}

int vg_remove_snapshot(struct logical_volume *cow)
{
	int merging_snapshot = 0;
	struct logical_volume *origin = origin_from_cow(cow);

	dm_list_del(&cow->snapshot->origin_list);
	origin->origin_count--;

	if (find_merging_cow(origin) == find_cow(cow)) {
		clear_snapshot_merge(origin);
		/*
		 * preload origin IFF "snapshot-merge" target is active
		 * - IMPORTANT: avoids preload if onactivate merge is pending
		 */
		if (lv_has_target_type(origin->vg->cmd->mem, origin, NULL,
				       "snapshot-merge")) {
			/*
			 * preload origin to:
			 * - allow proper release of -cow
			 * - avoid allocations with other devices suspended
			 *   when transitioning from "snapshot-merge" to
			 *   "snapshot-origin after a merge completes.
			 */
			merging_snapshot = 1;
		}
	}

	if (!lv_remove(cow->snapshot->lv)) {
		log_error("Failed to remove internal snapshot LV %s",
			  cow->snapshot->lv->name);
		return 0;
	}

	cow->snapshot = NULL;
	lv_set_visible(cow);

	if (!vg_write(origin->vg))
		return_0;
	if (!suspend_lv(origin->vg->cmd, origin)) {
		log_error("Failed to refresh %s without snapshot.",
			  origin->name);
		return 0;
	}
	if (!vg_commit(origin->vg))
		return_0;
	if (!merging_snapshot && !resume_lv(origin->vg->cmd, cow)) {
		log_error("Failed to resume %s.", cow->name);
		return 0;
	}
	if (!resume_lv(origin->vg->cmd, origin)) {
		log_error("Failed to resume %s.", origin->name);
		return 0;
	}

	return 1;
}

/*
 * Based on arch/arm/mm/ioremap.c
 *
 * (C) Copyright 1995 1996 Linus Torvalds
 * Hacked for ARM by Phil Blundell <philb@gnu.org>
 * Hacked to allow all architectures to build, and various cleanups
 * by Russell King
 * Copyright (C) 2012 ARM Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/export.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/io.h>

#include <asm/fixmap.h>
#include <asm/tlbflush.h>
#include <asm/pgalloc.h>

#ifdef CONFIG_ARM64_UNIQUE_IOMAP
#include <linux/memblock.h>

struct unique_iomapping {
 phys_addr_t phys_addr;
 size_t size;
 void *addr;
 struct list_head list;
};
LIST_HEAD(unique_iomaplist);
static void __init *early_alloc_aligned(unsigned long sz, unsigned long align)
{
	void *ptr = __va(memblock_alloc(sz, align));
	memset(ptr, 0, sz);
	return ptr;
}
void add_early_unique_iomapping(phys_addr_t phys,size_t size)
{
	struct unique_iomapping *uiop;
	struct unique_iomapping *curr_uiop;
    
	uiop = early_alloc_aligned(sizeof(*uiop), __alignof__(*uiop));

    uiop->addr=NULL;
    uiop->phys_addr=phys &PAGE_MASK;
    uiop->size=PAGE_ALIGN(size);

	list_for_each_entry(curr_uiop, &unique_iomaplist, list) {
		if (curr_uiop->phys_addr > uiop->phys_addr)
			break;
	}
	list_add_tail(&uiop->list, &curr_uiop->list);    
}

static struct unique_iomapping *find_unique_iomap_paddr(phys_addr_t paddr,
			size_t size)
{
    struct unique_iomapping *uiop;

	list_for_each_entry(uiop, &unique_iomaplist, list) {
		if (uiop->phys_addr > paddr ||
			paddr + size - 1 > uiop->phys_addr + uiop->size - 1)
			continue;

		return uiop;
	}

	return NULL;
}
static struct unique_iomapping *find_unique_iomap_vaddr(void *vaddr)
{
    struct unique_iomapping *uiop;

	list_for_each_entry(uiop, &unique_iomaplist, list) {
        if(!uiop->addr)continue;
		if (uiop->addr <= vaddr && uiop->addr + uiop->size > vaddr)
			return uiop;
	}

	return NULL;
}
#endif
static void __iomem *__ioremap_caller(phys_addr_t phys_addr, size_t size,
				      pgprot_t prot, void *caller)
{
	unsigned long last_addr;
	unsigned long offset = phys_addr & ~PAGE_MASK;
	int err;
	unsigned long addr;
	struct vm_struct *area;
#ifdef CONFIG_ARM64_UNIQUE_IOMAP    
    struct unique_iomapping *uiop=NULL;
    phys_addr_t old_phys_addr;
#endif    
	/*
	 * Page align the mapping address and size, taking account of any
	 * offset.
	 */
	phys_addr &= PAGE_MASK;
	size = PAGE_ALIGN(size + offset);

	/*
	 * Don't allow wraparound, zero size or outside PHYS_MASK.
	 */
	last_addr = phys_addr + size - 1;
	if (!size || last_addr < phys_addr || (last_addr & ~PHYS_MASK))
		return NULL;

	/*
	 * Don't allow RAM to be mapped.
	 */
	if (WARN_ON(pfn_valid(__phys_to_pfn(phys_addr))))
		return NULL;
#ifdef CONFIG_ARM64_UNIQUE_IOMAP 
	/*
	 * Try to reuse one of the static mapping whenever possible.
	 */
     
	if (size) {
        old_phys_addr=phys_addr;
		uiop = find_unique_iomap_paddr(old_phys_addr, size);
subrange:
		if (uiop) {
            if(uiop->addr){
                addr = (unsigned long)uiop->addr;
                addr += old_phys_addr - uiop->phys_addr;
                return (void __iomem *) (offset + addr);
            }//not mapped
            else
            {
                size=uiop->size;
                phys_addr=uiop->phys_addr;
            }
		}
	}
#endif    
	area = get_vm_area_caller(size, VM_IOREMAP, caller);
	if (!area)
		return NULL;
	addr = (unsigned long)area->addr;
    area->phys_addr = phys_addr;
    
	err = ioremap_page_range(addr, addr + size, phys_addr, prot);
	if (err) {
		vunmap((void *)addr);
		return NULL;
	}
#ifdef CONFIG_ARM64_UNIQUE_IOMAP     
    if(uiop)
    {
        uiop->addr=(void *)addr;
        goto subrange;
    }
#endif    
	return (void __iomem *)(offset + addr);
}

void __iomem *__ioremap(phys_addr_t phys_addr, size_t size, pgprot_t prot)
{
	return __ioremap_caller(phys_addr, size, prot,
				__builtin_return_address(0));
}
EXPORT_SYMBOL(__ioremap);

void __iounmap(volatile void __iomem *io_addr)
{
	unsigned long addr = (unsigned long)io_addr & PAGE_MASK;
#ifdef CONFIG_ARM64_UNIQUE_IOMAP     
    if(find_unique_iomap_vaddr(addr))return;
#endif    
	/*
	 * We could get an address outside vmalloc range in case
	 * of ioremap_cache() reusing a RAM mapping.
	 */
	if (VMALLOC_START <= addr && addr < VMALLOC_END)
		vunmap((void *)addr);
}
EXPORT_SYMBOL(__iounmap);

void __iomem *ioremap_cache(phys_addr_t phys_addr, size_t size)
{
	/* For normal memory we already have a cacheable mapping. */
	if (pfn_valid(__phys_to_pfn(phys_addr)))
		return (void __iomem *)__phys_to_virt(phys_addr);

	return __ioremap_caller(phys_addr, size, __pgprot(PROT_NORMAL),
				__builtin_return_address(0));
}
EXPORT_SYMBOL(ioremap_cache);

/*
 * Must be called after early_fixmap_init
 */
void __init early_ioremap_init(void)
{
	early_ioremap_setup();
}

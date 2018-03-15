/*
 * (c) 1997-2010 Netflix, Inc.  All content herein is protected by
 * U.S. copyright and other applicable intellectual property laws and
 * may not be copied without the express permission of Netflix, Inc.,
 * which reserves all rights.  Reuse of any of this content for any
 * purpose without the permission of Netflix, Inc. is strictly
 * prohibited.
 */
#include <asm/byteorder.h>
#include <linux/crypto.h>
#include <linux/scatterlist.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/syscalls.h>
#include <linux/nfsb.h>
#include "rsa.h"

static const char MAGIC[] = {'N','F','S','B'};

static const ssize_t BYTES_PER_SECTOR = 512;

uint8_t nfsb_hash_type(const struct nfsb_header *h)
{
	return h->hash_type;
}

ssize_t nfsb_fs_offset(const struct nfsb_header *h)
{
	return ntohl(h->fs_offset) * BYTES_PER_SECTOR;
}

ssize_t nfsb_fs_size(const struct nfsb_header *h)
{
	return ntohl(h->fs_size) * BYTES_PER_SECTOR;
}

ssize_t nfsb_hash_size(const struct nfsb_header *h)
{
	return ntohl(h->hash_size) * BYTES_PER_SECTOR;
}

uint32_t nfsb_data_blockcount(const struct nfsb_header *h)
{
	return ntohl(h->data_blocks);
}

uint32_t nfsb_data_blocksize(const struct nfsb_header *h)
{
	return ntohl(h->data_blocksize);
}

const char *nfsb_hash_algo(const struct nfsb_header *h)
{
	return h->hash_algo;
}

uint32_t nfsb_hash_blocksize(const struct nfsb_header *h)
{
	return ntohl(h->hash_blocksize);
}

const char *nfsb_verity_salt(const struct nfsb_header *h)
{
	return h->verity_salt;
}

const char *nfsb_verity_hash(const struct nfsb_header *h)
{
	return h->verity_hash;
}

int nfsb_verify(const struct nfsb_header *h, const uint8_t *key, const uint8_t *n)
{
	struct crypto_hash *tfm = NULL;
	struct hash_desc desc;
	uint8_t *digest = NULL;
	uint8_t *msg = NULL;
	struct scatterlist sg[1];
	int ret = -1;

	/* Initialize SHA-256. */
	tfm = crypto_alloc_hash("sha256", 0, 0);
	if (IS_ERR(tfm)) {
		printk(KERN_ERR "Failed to load transform for sha256: %ld\n",
			   PTR_ERR(tfm));
		goto failure;
	}
	desc.tfm = tfm;
	desc.flags = 0;

	/* SHA-256 the header, minus the signature memory. */
	digest = kmalloc(crypto_hash_digestsize(tfm), GFP_KERNEL);
	if (!digest) {
		printk(KERN_ERR "Failed to allocate digest memory.\n");
		goto failure;
	}
	ret = crypto_hash_init(&desc);
	if (ret) {
		printk(KERN_ERR "crypto_hash_init failure: %d\n", ret);
		goto failure;
	}
	sg_set_buf(sg, h, sizeof(struct nfsb_header) - sizeof(h->rsa_sig));
	ret = crypto_hash_update(&desc, sg, sizeof(struct nfsb_header) - sizeof(h->rsa_sig));
	if (ret) {
		printk(KERN_ERR "crypto_hash_update failure: %d\n", ret);
		goto failure;
	}
	ret = crypto_hash_final(&desc, digest);
	if (ret) {
		printk(KERN_ERR "crypto_hash_final failure: %d\n", ret);
		goto failure;
	}

	/* Reconstruct the SHA-256 from the header RSA-2048 signature. */
	msg = rsa_encryptdecrypt(h->rsa_sig, key, n);
	if (!msg) {
		printk(KERN_ERR "rsa_encryptdecrypt failure\n");
		goto failure;
	}

	/* Compare the digests. */
	if (!memcmp(&msg[RSA_LEN - SHA256_LEN], digest, SHA256_LEN))
		ret = 0;
	else
		ret = -1;

failure:
	kfree(msg);
	kfree(digest);
	crypto_free_hash(tfm);
	return ret;
}

ssize_t nfsb_read(int fd, struct nfsb_header *h)
{
	/* Once different versions are supported, this would need to change. */
	ssize_t r = 0, total = 0;
	while (total < sizeof(struct nfsb_header))
	{
		r = sys_read(fd, (char*)(h) + r, sizeof(struct nfsb_header) - r);
		if (r == -1) return -1;
		total += r;
	}
	/* Check magic. */
	if (memcmp(h->magic, MAGIC, sizeof(MAGIC)) != 0)
		return -1;
	/* Check version. */
	if (h->version != 3)
		return -1;

	return total;
}

// Called in kernel space for non-rootfs image
ssize_t nfsb_nonroot_read(struct file *fp, struct nfsb_header *h)
{
	/* Once different versions are supported, this would need to change. */
	ssize_t r = 0, total = 0;

	while (total < sizeof(struct nfsb_header))
	{
		r = fp->f_op->read(fp, (char*)(h) + r, sizeof(struct nfsb_header) - r, &fp->f_pos);
		if (r == -1) return -1;
		total += r;
	}

	/* Check magic. */
	if (memcmp(h->magic, MAGIC, sizeof(MAGIC)) != 0)
		return -1;

	/* Check version. */
	if (h->version != 3)
		return -1;

	return total;
}

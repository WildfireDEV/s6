/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation; or, when distributed
 * separately from the Linux kernel or incorporated into other
 * software packages, subject to the following license:
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this source file (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef __XEN_BLKIF__BACKEND__COMMON_H__
#define __XEN_BLKIF__BACKEND__COMMON_H__

#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/blkdev.h>
#include <linux/vmalloc.h>
#include <linux/wait.h>
#include <linux/io.h>
#include <linux/rbtree.h>
#include <asm/setup.h>
#include <asm/pgalloc.h>
#include <asm/hypervisor.h>
#include <xen/grant_table.h>
#include <xen/xenbus.h>
#include <xen/interface/io/ring.h>
#include <xen/interface/io/blkif.h>
#include <xen/interface/io/protocols.h>

#define DRV_PFX "xen-blkback:"
#define DPRINTK(fmt, args...)				\
	pr_debug(DRV_PFX "(%s:%d) " fmt ".\n",		\
		 __func__, __LINE__, ##args)


/* Not a real protocol.  Used to generate ring structs which contain
 * the elements common to all protocols only.  This way we get a
 * compiler-checkable way to use common struct elements, so we can
 * avoid using switch(protocol) in a number of places.  */
struct blkif_common_request {
	char dummy;
};
struct blkif_common_response {
	char dummy;
};

struct blkif_x86_32_request_rw {
	uint8_t        nr_segments;  /* number of segments                   */
	blkif_vdev_t   handle;       /* only for read/write requests         */
	uint64_t       id;           /* private guest value, echoed in resp  */
	blkif_sector_t sector_number;/* start sector idx on disk (r/w only)  */
	struct blkif_request_segment seg[BLKIF_MAX_SEGMENTS_PER_REQUEST];
} __attribute__((__packed__));

struct blkif_x86_32_request_discard {
	uint8_t        flag;         /* BLKIF_DISCARD_SECURE or zero         */
	blkif_vdev_t   _pad1;        /* was "handle" for read/write requests */
	uint64_t       id;           /* private guest value, echoed in resp  */
	blkif_sector_t sector_number;/* start sector idx on disk (r/w only)  */
	uint64_t       nr_sectors;
} __attribute__((__packed__));

struct blkif_x86_32_request_other {
	uint8_t        _pad1;
	blkif_vdev_t   _pad2;
	uint64_t       id;           /* private guest value, echoed in resp  */
} __attribute__((__packed__));

struct blkif_x86_32_request {
	uint8_t        operation;    /* BLKIF_OP_???                         */
	union {
		struct blkif_x86_32_request_rw rw;
		struct blkif_x86_32_request_discard discard;
		struct blkif_x86_32_request_other other;
	} u;
} __attribute__((__packed__));

/* i386 protocol version */
#pragma pack(push, 4)
struct blkif_x86_32_response {
	uint64_t        id;              /* copied from request */
	uint8_t         operation;       /* copied from request */
	int16_t         status;          /* BLKIF_RSP_???       */
};
#pragma pack(pop)
/* x86_64 protocol version */

struct blkif_x86_64_request_rw {
	uint8_t        nr_segments;  /* number of segments                   */
	blkif_vdev_t   handle;       /* only for read/write requests         */
	uint32_t       _pad1;        /* offsetof(blkif_reqest..,u.rw.id)==8  */
	uint64_t       id;
	blkif_sector_t sector_number;/* start sector idx on disk (r/w only)  */
	struct blkif_request_segment seg[BLKIF_MAX_SEGMENTS_PER_REQUEST];
} __attribute__((__packed__));

struct blkif_x86_64_request_discard {
	uint8_t        flag;         /* BLKIF_DISCARD_SECURE or zero         */
	blkif_vdev_t   _pad1;        /* was "handle" for read/write requests */
        uint32_t       _pad2;        /* offsetof(blkif_..,u.discard.id)==8   */
	uint64_t       id;
	blkif_sector_t sector_number;/* start sector idx on disk (r/w only)  */
	uint64_t       nr_sectors;
} __attribute__((__packed__));

struct blkif_x86_64_request_other {
	uint8_t        _pad1;
	blkif_vdev_t   _pad2;
	uint32_t       _pad3;        /* offsetof(blkif_..,u.discard.id)==8   */
	uint64_t       id;           /* private guest value, echoed in resp  */
} __attribute__((__packed__));

struct blkif_x86_64_request {
	uint8_t        operation;    /* BLKIF_OP_???                         */
	union {
		struct blkif_x86_64_request_rw rw;
		struct blkif_x86_64_request_discard discard;
		struct blkif_x86_64_request_other other;
	} u;
} __attribute__((__packed__));

struct blkif_x86_64_response {
	uint64_t       __attribute__((__aligned__(8))) id;
	uint8_t         operation;       /* copied from request */
	int16_t         status;          /* BLKIF_RSP_???       */
};

DEFINE_RING_TYPES(blkif_common, struct blkif_common_request,
		  struct blkif_common_response);
DEFINE_RING_TYPES(blkif_x86_32, struct blkif_x86_32_request,
		  struct blkif_x86_32_response);
DEFINE_RING_TYPES(blkif_x86_64, struct blkif_x86_64_request,
		  struct blkif_x86_64_response);

union blkif_back_rings {
	struct blkif_back_ring        native;
	struct blkif_common_back_ring common;
	struct blkif_x86_32_back_ring x86_32;
	struct blkif_x86_64_back_ring x86_64;
};

enum blkif_protocol {
	BLKIF_PROTOCOL_NATIVE = 1,
	BLKIF_PROTOCOL_X86_32 = 2,
	BLKIF_PROTOCOL_X86_64 = 3,
};

struct xen_vbd {
	/* What the domain refers to this vbd as. */
	blkif_vdev_t		handle;
	/* Non-zero -> read-only */
	unsigned char		readonly;
	/* VDISK_xxx */
	unsigned char		type;
	/* phys device that this vbd maps to. */
	u32			pdevice;
	struct block_device	*bdev;
	/* Cached size parameter. */
	sector_t		size;
	unsigned int		flush_support:1;
	unsigned int		discard_secure:1;
	unsigned int		feature_gnt_persistent:1;
	unsigned int		overflow_max_grants:1;
};

struct backend_info;

/* Number of available flags */
#define PERSISTENT_GNT_FLAGS_SIZE	2
/* This persistent grant is currently in use */
#define PERSISTENT_GNT_ACTIVE		0
/*
 * This persistent grant has been used, this flag is set when we remove the
 * PERSISTENT_GNT_ACTIVE, to know that this grant has been used recently.
 */
#define PERSISTENT_GNT_WAS_ACTIVE	1

/* Number of requests that we can fit in a ring */
#define XEN_BLKIF_REQS			32

struct persistent_gnt {
	struct page *page;
	grant_ref_t gnt;
	grant_handle_t handle;
	DECLARE_BITMAP(flags, PERSISTENT_GNT_FLAGS_SIZE);
	struct rb_node node;
	struct list_head remove_node;
};

struct xen_blkif {
	/* Unique identifier for this interface. */
	domid_t			domid;
	unsigned int		handle;
	/* Physical parameters of the comms window. */
	unsigned int		irq;
	/* Comms information. */
	enum blkif_protocol	blk_protocol;
	union blkif_back_rings	blk_rings;
	void			*blk_ring;
	/* The VBD attached to this interface. */
	struct xen_vbd		vbd;
	/* Back pointer to the backend_info. */
	struct backend_info	*be;
	/* Private fields. */
	spinlock_t		blk_ring_lock;
	atomic_t		refcnt;

	wait_queue_head_t	wq;
	/* for barrier (drain) requests */
	struct completion	drain_complete;
	atomic_t		drain;
	/* One thread per one blkif. */
	struct task_struct	*xenblkd;
	unsigned int		waiting_reqs;

	/* tree to store persistent grants */
	struct rb_root		persistent_gnts;
	unsigned int		persistent_gnt_c;
	atomic_t		persistent_gnt_in_use;
	unsigned long           next_lru;

	/* used by the kworker that offload work from the persistent purge */
	struct list_head	persistent_purge_list;
	struct work_struct	persistent_purge_work;

	/* buffer of free pages to map grant refs */
	spinlock_t		free_pages_lock;
	int			free_pages_num;
	struct list_head	free_pages;

	/* Allocation of pending_reqs */
	struct pending_req	*pending_reqs;
	/* List of all 'pending_req' available */
	struct list_head	pending_free;
	/* And its spinlock. */
	spinlock_t		pending_free_lock;
	wait_queue_head_t	pending_free_wq;

	/* statistics */
	unsigned long		st_print;
	unsigned long long			st_rd_req;
	unsigned long long			st_wr_req;
	unsigned long long			st_oo_req;
	unsigned long long			st_f_req;
	unsigned long long			st_ds_req;
	unsigned long long			st_rd_sect;
	unsigned long long			st_wr_sect;

	wait_queue_head_t	waiting_to_free;
};

/*
 * Each outstanding request that we've passed to the lower device layers has a
 * 'pending_req' allocated to it. Each buffer_head that completes decrements
 * the pendcnt towards zero. When it hits zero, the specified domain has a
 * response queued for it, with the saved 'id' passed back.
 */
struct pending_req {
	struct xen_blkif	*blkif;
	u64			id;
	int			nr_pages;
	atomic_t		pendcnt;
	unsigned short		operation;
	int			status;
	struct list_head	free_list;
	struct page		*pages[BLKIF_MAX_SEGMENTS_PER_REQUEST];
	struct persistent_gnt	*persistent_gnts[BLKIF_MAX_SEGMENTS_PER_REQUEST];
	grant_handle_t		grant_handles[BLKIF_MAX_SEGMENTS_PER_REQUEST];
};


#define vbd_sz(_v)	((_v)->bdev->bd_part ? \
			 (_v)->bdev->bd_part->nr_sects : \
			  get_capacity((_v)->bdev->bd_disk))

#define xen_blkif_get(_b) (atomic_inc(&(_b)->refcnt))
#define xen_blkif_put(_b)				\
	do {						\
		if (atomic_dec_and_test(&(_b)->refcnt))	\
			wake_up(&(_b)->waiting_to_free);\
	} while (0)

struct phys_req {
	unsigned short		dev;
	blkif_sector_t		nr_sects;
	struct block_device	*bdev;
	blkif_sector_t		sector_number;
};
int xen_blkif_interface_init(void);

int xen_blkif_xenbus_init(void);

irqreturn_t xen_blkif_be_int(int irq, void *dev_id);
int xen_blkif_schedule(void *arg);
int xen_blkif_purge_persistent(void *arg);

int xen_blkbk_flush_diskcache(struct xenbus_transaction xbt,
			      struct backend_info *be, int state);

int xen_blkbk_barrier(struct xenbus_transaction xbt,
		      struct backend_info *be, int state);
struct xenbus_device *xen_blkbk_xenbus(struct backend_info *be);

static inline void blkif_get_x86_32_req(struct blkif_request *dst,
					struct blkif_x86_32_request *src)
{
	int i, n = BLKIF_MAX_SEGMENTS_PER_REQUEST;
	dst->operation = src->operation;
	switch (src->operation) {
	case BLKIF_OP_READ:
	case BLKIF_OP_WRITE:
	case BLKIF_OP_WRITE_BARRIER:
	case BLKIF_OP_FLUSH_DISKCACHE:
		dst->u.rw.nr_segments = src->u.rw.nr_segments;
		dst->u.rw.handle = src->u.rw.handle;
		dst->u.rw.id = src->u.rw.id;
		dst->u.rw.sector_number = src->u.rw.sector_number;
		barrier();
		if (n > dst->u.rw.nr_segments)
			n = dst->u.rw.nr_segments;
		for (i = 0; i < n; i++)
			dst->u.rw.seg[i] = src->u.rw.seg[i];
		break;
	case BLKIF_OP_DISCARD:
		dst->u.discard.flag = src->u.discard.flag;
		dst->u.discard.id = src->u.discard.id;
		dst->u.discard.sector_number = src->u.discard.sector_number;
		dst->u.discard.nr_sectors = src->u.discard.nr_sectors;
		break;
	default:
		/*
		 * Don't know how to translate this op. Only get the
		 * ID so failure can be reported to the frontend.
		 */
		dst->u.other.id = src->u.other.id;
		break;
	}
}

static inline void blkif_get_x86_64_req(struct blkif_request *dst,
					struct blkif_x86_64_request *src)
{
	int i, n = BLKIF_MAX_SEGMENTS_PER_REQUEST;
	dst->operation = src->operation;
	switch (src->operation) {
	case BLKIF_OP_READ:
	case BLKIF_OP_WRITE:
	case BLKIF_OP_WRITE_BARRIER:
	case BLKIF_OP_FLUSH_DISKCACHE:
		dst->u.rw.nr_segments = src->u.rw.nr_segments;
		dst->u.rw.handle = src->u.rw.handle;
		dst->u.rw.id = src->u.rw.id;
		dst->u.rw.sector_number = src->u.rw.sector_number;
		barrier();
		if (n > dst->u.rw.nr_segments)
			n = dst->u.rw.nr_segments;
		for (i = 0; i < n; i++)
			dst->u.rw.seg[i] = src->u.rw.seg[i];
		break;
	case BLKIF_OP_DISCARD:
		dst->u.discard.flag = src->u.discard.flag;
		dst->u.discard.id = src->u.discard.id;
		dst->u.discard.sector_number = src->u.discard.sector_number;
		dst->u.discard.nr_sectors = src->u.discard.nr_sectors;
		break;
	default:
		/*
		 * Don't know how to translate this op. Only get the
		 * ID so failure can be reported to the frontend.
		 */
		dst->u.other.id = src->u.other.id;
		break;
	}
}

#endif /* __XEN_BLKIF__BACKEND__COMMON_H__ */

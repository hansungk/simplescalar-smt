/* mase-mem.c - sim-mase memory functions */

/* MASE was created by Eric Larson, Saugata Chatterjee, Dan Ernst, and
 * Todd M. Austin at the University of Michigan.
 */

/* SimpleScalar(TM) Tool Suite
 * Copyright (C) 1994-2001 by Todd M. Austin, Ph.D. and SimpleScalar, LLC.
 * All Rights Reserved. 
 * 
 * THIS IS A LEGAL DOCUMENT, BY USING SIMPLESCALAR,
 * YOU ARE AGREEING TO THESE TERMS AND CONDITIONS.
 * 
 * No portion of this work may be used by any commercial entity, or for any
 * commercial purpose, without the prior, written permission of SimpleScalar,
 * LLC (info@simplescalar.com). Nonprofit and noncommercial use is permitted
 * as described below.
 * 
 * 1. SimpleScalar is provided AS IS, with no warranty of any kind, express
 * or implied. The user of the program accepts full responsibility for the
 * application of the program and the use of any results.
 * 
 * 2. Nonprofit and noncommercial use is encouraged. SimpleScalar may be
 * downloaded, compiled, executed, copied, and modified solely for nonprofit,
 * educational, noncommercial research, and noncommercial scholarship
 * purposes provided that this notice in its entirety accompanies all copies.
 * Copies of the modified software can be delivered to persons who use it
 * solely for nonprofit, educational, noncommercial research, and
 * noncommercial scholarship purposes provided that this notice in its
 * entirety accompanies all copies.
 * 
 * 3. ALL COMMERCIAL USE, AND ALL USE BY FOR PROFIT ENTITIES, IS EXPRESSLY
 * PROHIBITED WITHOUT A LICENSE FROM SIMPLESCALAR, LLC (info@simplescalar.com).
 * 
 * 4. No nonprofit user may place any restrictions on the use of this software,
 * including as modified by the user, by any other authorized user.
 * 
 * 5. Noncommercial and nonprofit users may distribute copies of SimpleScalar
 * in compiled or executable form as set forth in Section 2, provided that
 * either: (A) it is accompanied by the corresponding machine-readable source
 * code, or (B) it is accompanied by a written offer, with no time limit, to
 * give anyone a machine-readable copy of the corresponding source code in
 * return for reimbursement of the cost of distribution. This written offer
 * must permit verbatim duplication by anyone, or (C) it is distributed by
 * someone who received only the executable form, and is accompanied by a
 * copy of the written offer of source code.
 * 
 * 6. SimpleScalar was developed by Todd M. Austin, Ph.D. The tool suite is
 * currently maintained by SimpleScalar LLC (info@simplescalar.com). US Mail:
 * 2395 Timbercrest Court, Ann Arbor, MI 48105.
 * 
 * Copyright (C) 2000-2001 by The Regents of The University of Michigan.
 * Copyright (C) 1994-2001 by Todd M. Austin, Ph.D. and SimpleScalar, LLC.
 */

#include "mase.h"

/* level 1 instruction cache, entry level instruction cache */
struct cache_t *cache_il1;

/* level 1 instruction cache */
struct cache_t *cache_il2;

/* level 1 data cache, entry level data cache */
struct cache_t *cache_dl1;

/* level 2 data cache */
struct cache_t *cache_dl2;

/* instruction TLB */
struct cache_t *itlb;

/* data TLB */
struct cache_t *dtlb;

#ifdef TEST_CB
void (*cb_fn) (unsigned int rid, unsigned int lat) = NULL;
int cb_rid;
#endif

/* mase cache access function  - a wrapper around cache_access */
enum mem_status                            
mase_cache_access(
	struct cache_t *cp,	/* cache to access */
	enum mem_cmd cmd,	/* access type, Read or Write */
	md_addr_t addr,		/* address of access */
	void *vp,		/* ptr to buffer for input/output */
	int nbytes,		/* number of bytes to access */
	tick_t now,		/* time of access */
	byte_t **udata,		/* for return of user data ptr */
	md_addr_t *repl_addr,	/* for address of replaced block */
	void (*callback_fn)	/* function to call when latency is known */
	  (unsigned int rid, unsigned int lat),
	unsigned int rid,	/* request id */
	unsigned int *lat)	/* latency of access in cycles (return value) */
{ 
#ifdef TEST_CB
  int random = rand() % 10;

  *lat = cache_access(cp, cmd, addr, vp, nbytes, now, udata, repl_addr);
  
  if (random == 0) return MEM_RETRY;
  if ((random == 5) && (cb_fn == NULL) && (callback_fn != NULL)) {
    cb_fn = callback_fn;
    cb_rid = rid;
    return MEM_UNKNOWN;
  }

  return MEM_KNOWN;
#else

  *lat = cache_access(cp, cmd, addr, vp, nbytes, now, udata, repl_addr);
  return MEM_KNOWN;

#endif
}

#ifdef TEST_CB
/* callback tester */
void
cb_tester()
{
  if (cb_fn == NULL) return;
  if (rand() % 10 == 0) {
    cb_fn(cb_rid, 5);
    cb_fn = NULL;
  }
}
#endif
   
/* memory access latency, assumed to not cross a page boundary */
unsigned int			/* total latency of access */
mem_access_latency(int blk_sz)		/* block size accessed */
{
  int chunks = (blk_sz + (mem_bus_width - 1)) / mem_bus_width;

  assert(chunks > 0);

  return (/* first chunk latency */	mem_lat[0] +
	 (/* remainder chunk latency */	mem_lat[1] * (chunks - 1)));
}

/*
 * cache miss handlers
 */

/* l1 data cache l1 block miss handler function */
unsigned int				/* latency of block access */
dl1_access_fn(enum mem_cmd cmd,		/* access cmd, Read or Write */
	      md_addr_t baddr,		/* block address to access */
	      int bsize,		/* size of block to access */
	      struct cache_blk_t *blk,	/* ptr to block in upper level */
	      tick_t now)		/* time of access */
{
  unsigned int lat;

  if (cache_dl2)
    {
      /* access next level of data cache hierarchy */
      lat = cache_access(cache_dl2, cmd, baddr, NULL, bsize,
			 /* now */now, /* pudata */NULL, /* repl addr */NULL);
      if (cmd == Read)
	return lat;
      else
	{
	  /* FIXME: unlimited write buffers */
	  return 0;
	}
    }
  else
    {
      /* access main memory */
      if (cmd == Read)
	return mem_access_latency(bsize);
      else
	{
	  /* FIXME: unlimited write buffers */
	  return 0;
	}
    }
}

/* l2 data cache block miss handler function */
unsigned int				/* latency of block access */
dl2_access_fn(enum mem_cmd cmd,		/* access cmd, Read or Write */
	      md_addr_t baddr,		/* block address to access */
	      int bsize,		/* size of block to access */
	      struct cache_blk_t *blk,	/* ptr to block in upper level */
	      tick_t now)		/* time of access */
{
  /* this is a miss to the lowest level, so access main memory */
  if (cmd == Read)
    return mem_access_latency(bsize);
  else
    {
      /* FIXME: unlimited write buffers */
      return 0;
    }
}

/* l1 inst cache l1 block miss handler function */
unsigned int				/* latency of block access */
il1_access_fn(enum mem_cmd cmd,		/* access cmd, Read or Write */
	      md_addr_t baddr,		/* block address to access */
	      int bsize,		/* size of block to access */
	      struct cache_blk_t *blk,	/* ptr to block in upper level */
	      tick_t now)		/* time of access */
{
  unsigned int lat;

if (cache_il2)
    {
      /* access next level of inst cache hierarchy */
      lat = cache_access(cache_il2, cmd, baddr, NULL, bsize,
			 /* now */now, /* pudata */NULL, /* repl addr */NULL);
      if (cmd == Read)
	return lat;
      else
	panic("writes to instruction memory not supported");
    }
  else
    {
      /* access main memory */
      if (cmd == Read)
	return mem_access_latency(bsize);
      else
	panic("writes to instruction memory not supported");
    }
}

/* l2 inst cache block miss handler function */
unsigned int				/* latency of block access */
il2_access_fn(enum mem_cmd cmd,		/* access cmd, Read or Write */
	      md_addr_t baddr,		/* block address to access */
	      int bsize,		/* size of block to access */
	      struct cache_blk_t *blk,	/* ptr to block in upper level */
	      tick_t now)		/* time of access */
{
  /* this is a miss to the lowest level, so access main memory */
  if (cmd == Read)
    return mem_access_latency(bsize);
  else
    panic("writes to instruction memory not supported");
}

/* TLB miss handlers */

/* inst cache block miss handler function */
unsigned int				/* latency of block access */
itlb_access_fn(enum mem_cmd cmd,	/* access cmd, Read or Write */
	       md_addr_t baddr,		/* block address to access */
	       int bsize,		/* size of block to access */
	       struct cache_blk_t *blk,	/* ptr to block in upper level */
	       tick_t now)		/* time of access */
{
  md_addr_t *phy_page_ptr = (md_addr_t *)blk->user_data;

  /* no real memory access, however, should have user data space attached */
  assert(phy_page_ptr);

  /* fake translation, for now... */
  *phy_page_ptr = 0;

  /* return tlb miss latency */
  return tlb_miss_lat;
}

/* data cache block miss handler function */
unsigned int				/* latency of block access */
dtlb_access_fn(enum mem_cmd cmd,	/* access cmd, Read or Write */
	       md_addr_t baddr,		/* block address to access */
	       int bsize,		/* size of block to access */
	       struct cache_blk_t *blk,	/* ptr to block in upper level */
	       tick_t now)		/* time of access */
{
  md_addr_t *phy_page_ptr = (md_addr_t *)blk->user_data;

  /* no real memory access, however, should have user data space attached */
  assert(phy_page_ptr);

  /* fake translation, for now... */
  *phy_page_ptr = 0;

  /* return tlb miss latency */
  return tlb_miss_lat;
}

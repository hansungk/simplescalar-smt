/* memory.c - flat memory space routines */

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

#include <stdio.h>
#include <stdlib.h>

#include "host.h"
#include "misc.h"
#include "machine.h"
#include "options.h"
#include "stats.h"
#include "memory.h"

/* create a flat memory space */
struct mem_t *
mem_create(char *name)			/* name of the memory space */
{
  struct mem_t *mem;

  mem = calloc(1, sizeof(struct mem_t));
  if (!mem)
    fatal("out of virtual memory");

  mem->name = mystrdup(name);
  return mem;
}

/* translate address ADDR in memory space MEM, returns pointer to host page */
byte_t *
mem_translate(struct mem_t *mem,	/* memory space to access */
	      md_addr_t addr)		/* virtual address to translate */
{
  struct mem_pte_t *pte, *prev;

  /* got here via a first level miss in the page tables */
  mem->ptab_misses++; mem->ptab_accesses++;

  /* locate accessed PTE */
  for (prev=NULL, pte=mem->ptab[MEM_PTAB_SET(addr)];
       pte != NULL;
       prev=pte, pte=pte->next)
    {
      if (pte->tag == MEM_PTAB_TAG(addr))
	{
	  /* move this PTE to head of the bucket list */
	  if (prev)
	    {
	      prev->next = pte->next;
	      pte->next = mem->ptab[MEM_PTAB_SET(addr)];
	      mem->ptab[MEM_PTAB_SET(addr)] = pte;
	    }
	  return pte->page;
	}
    }

  /* no translation found, return NULL */
  return NULL;
}

/* allocate a memory page */
void
mem_newpage(struct mem_t *mem,		/* memory space to allocate in */
	    md_addr_t addr)		/* virtual address to allocate */
{
  byte_t *page;
  struct mem_pte_t *pte;

  /* see misc.c for details on the getcore() function */
  page = getcore(MD_PAGE_SIZE);
  if (!page)
    fatal("out of virtual memory");

  /* generate a new PTE */
  pte = calloc(1, sizeof(struct mem_pte_t));
  if (!pte)
    fatal("out of virtual memory");
  pte->tag = MEM_PTAB_TAG(addr);
  pte->page = page;

  /* insert PTE into inverted hash table */
  pte->next = mem->ptab[MEM_PTAB_SET(addr)];
  mem->ptab[MEM_PTAB_SET(addr)] = pte;

  /* one more page allocated */
  mem->page_count++;
}

/* check for memory related faults */
enum md_fault_type
mem_check_fault(struct mem_t *mem,	/* memory */
	        enum mem_cmd cmd,	/* Read or Write */
	        md_addr_t addr,		/* target addresss */
	        int nbytes)		/* number of bytes of access */
{
  /* check alignments */
  if (/* check size */(nbytes & (nbytes-1)) != 0
      || /* check max size */nbytes > MD_PAGE_SIZE)
    return md_fault_access;

  if (/* check natural alignment */(addr & (nbytes-1)) != 0)
    return md_fault_alignment;

  return md_fault_none;
}

/* generic memory access function, it's safe because alignments and permissions
   are checked, handles any natural transfer sizes; note, faults out if nbytes
   is not a power-of-two or larger then MD_PAGE_SIZE */
enum md_fault_type
mem_access(struct mem_t *mem,		/* memory space to access */
	   enum mem_cmd cmd,		/* Read (from sim mem) or Write */
	   md_addr_t addr,		/* target address to access */
	   void *vp,			/* host memory address to access */
	   int nbytes)			/* number of bytes to access */
{
  enum md_fault_type fault;
  byte_t *p = vp;

  /* check for faults */
  fault = mem_check_fault(mem, cmd, addr, nbytes);
  if (fault != md_fault_none) return fault;

  /* perform the copy */
  if (cmd == Read) {

    while (nbytes-- > 0) {
      *((byte_t *)p) = MEM_READ_BYTE(mem, addr);
      p += sizeof(byte_t);
      addr += sizeof(byte_t);
    }
  }
  else {

    while (nbytes-- > 0) {
      MEM_WRITE_BYTE(mem, addr, *((byte_t *)p));
      p += sizeof(byte_t);
      addr += sizeof(byte_t);
    }
  }

  /* no fault... */
  return md_fault_none;
}

/* register memory system-specific statistics */
void
mem_reg_stats(struct mem_t *mem,	/* memory space to declare */
	      struct stat_sdb_t *sdb)	/* stats data base */
{
  char buf[512], buf1[512];

  sprintf(buf, "%s.page_count", mem->name);
  stat_reg_counter(sdb, buf, "total number of pages allocated",
		   &mem->page_count, mem->page_count, NULL);

  sprintf(buf, "%s.page_mem", mem->name);
  sprintf(buf1, "%s.page_count * %d / 1024", mem->name, MD_PAGE_SIZE);
  stat_reg_formula(sdb, buf, "total size of memory pages allocated",
		   buf1, "%11.0fk");

  sprintf(buf, "%s.ptab_misses", mem->name);
  stat_reg_counter(sdb, buf, "total first level page table misses",
		   &mem->ptab_misses, mem->ptab_misses, NULL);

  sprintf(buf, "%s.ptab_accesses", mem->name);
  stat_reg_counter(sdb, buf, "total page table accesses",
		   &mem->ptab_accesses, mem->ptab_accesses, NULL);

  sprintf(buf, "%s.ptab_miss_rate", mem->name);
  sprintf(buf1, "%s.ptab_misses / %s.ptab_accesses", mem->name, mem->name);
  stat_reg_formula(sdb, buf, "first level page table miss rate", buf1, NULL);
}

/* initialize memory system, call before loader.c */
void
mem_init(struct mem_t *mem)	/* memory space to initialize */
{
  int i;

  /* initialize the first level page table to all empty */
  for (i=0; i < MEM_PTAB_SIZE; i++)
    mem->ptab[i] = NULL;

  mem->page_count = 0;
  mem->ptab_misses = 0;
  mem->ptab_accesses = 0;
}

/* dump a block of memory, returns any faults encountered */
enum md_fault_type
mem_dump(struct mem_t *mem,		/* memory space to display */
	 md_addr_t addr,		/* target address to dump */
	 int len,			/* number bytes to dump */
	 FILE *stream)			/* output stream */
{
  int data;
  enum md_fault_type fault;

  if (!stream)
    stream = stderr;

  addr &= ~sizeof(word_t);
  len = (len + (sizeof(word_t) - 1)) & ~sizeof(word_t);
  while (len-- > 0)
    {
      fault = mem_access(mem, Read, addr, &data, sizeof(word_t));
      if (fault != md_fault_none)
	return fault;

      myfprintf(stream, "0x%08p: %08x\n", addr, data);
      addr += sizeof(word_t);
    }

  /* no faults... */
  return md_fault_none;
}

/* copy a '\0' terminated string to/from simulated memory space, returns
   the number of bytes copied, returns any fault encountered */
enum md_fault_type
mem_strcpy(mem_access_fn mem_fn,	/* user-specified memory accessor */
	   struct mem_t *mem,		/* memory space to access */
	   enum mem_cmd cmd,		/* Read (from sim mem) or Write */
	   md_addr_t addr,		/* target address to access */
	   char *s)
{
  int n = 0;
  char c;
  enum md_fault_type fault;

  switch (cmd)
    {
    case Read:
      /* copy until string terminator ('\0') is encountered */
      do {
	fault = mem_fn(mem, Read, addr++, &c, 1);
	if (fault != md_fault_none)
	  return fault;
	*s++ = c;
	n++;
      } while (c);
      break;

    case Write:
      /* copy until string terminator ('\0') is encountered */
      do {
	c = *s++;
	fault = mem_fn(mem, Write, addr++, &c, 1);
	if (fault != md_fault_none)
	  return fault;
	n++;
      } while (c);
      break;

    default:
      return md_fault_internal;
  }

  /* no faults... */
  return md_fault_none;
}

/* copy NBYTES to/from simulated memory space, returns any faults */
enum md_fault_type
mem_bcopy(mem_access_fn mem_fn,		/* user-specified memory accessor */
	  struct mem_t *mem,		/* memory space to access */
	  enum mem_cmd cmd,		/* Read (from sim mem) or Write */
	  md_addr_t addr,		/* target address to access */
	  void *vp,			/* host memory address to access */
	  int nbytes)
{
  byte_t *p = vp;
  enum md_fault_type fault;

  /* copy NBYTES bytes to/from simulator memory */
  while (nbytes-- > 0)
    {
      fault = mem_fn(mem, cmd, addr++, p++, 1);
      if (fault != md_fault_none)
	return fault;
    }

  /* no faults... */
  return md_fault_none;
}

/* copy NBYTES to/from simulated memory space, NBYTES must be a multiple
   of 4 bytes, this function is faster than mem_bcopy(), returns any
   faults encountered */
enum md_fault_type
mem_bcopy4(mem_access_fn mem_fn,	/* user-specified memory accessor */
	   struct mem_t *mem,		/* memory space to access */
	   enum mem_cmd cmd,		/* Read (from sim mem) or Write */
	   md_addr_t addr,		/* target address to access */
	   void *vp,			/* host memory address to access */
	   int nbytes)
{
  byte_t *p = vp;
  int words = nbytes >> 2;		/* note: nbytes % 2 == 0 is assumed */
  enum md_fault_type fault;

  while (words-- > 0)
    {
      fault = mem_fn(mem, cmd, addr, p, sizeof(word_t));
      if (fault != md_fault_none)
	return fault;

      addr += sizeof(word_t);
      p += sizeof(word_t);
    }

  /* no faults... */
  return md_fault_none;
}

/* zero out NBYTES of simulated memory, returns any faults encountered */
enum md_fault_type
mem_bzero(mem_access_fn mem_fn,		/* user-specified memory accessor */
	  struct mem_t *mem,		/* memory space to access */
	  md_addr_t addr,		/* target address to access */
	  int nbytes)
{
  byte_t c = 0;
  enum md_fault_type fault;

  /* zero out NBYTES of simulator memory */
  while (nbytes-- > 0)
    {
      fault = mem_fn(mem, Write, addr++, &c, 1);
      if (fault != md_fault_none)
	return fault;
    }

  /* no faults... */
  return md_fault_none;
}

/* mase-debug.c - sim-mase debugging functions */

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

/* default memory state accessor, used by DLite */
char *						/* err str, NULL for no err */
mase_mem_obj(struct mem_t *mem,			/* memory space to access */
	    int is_write,			/* access type */
	    md_addr_t addr,			/* address to access */
	    char *p,				/* input/output buffer */
	    int nbytes)				/* size of access */
{
  if (is_write) panic("This routine does not expect a write to memory");
    
  /* access memory */
  mem_access(mem, Read, addr, p, nbytes);

  /* no error */
  return NULL;
}

/* default register state accessor, used by DLite */
char *						/* err str, NULL for no err */
mase_reg_obj(struct regs_t *regs,		/* registers to access */
	    int is_write,			/* access type */
	    enum md_reg_type rt,		/* reg bank to probe */
	    int reg,				/* register number */
	    struct eval_value_t *val)		/* input, output */
{
  /* The only option in DLite to write a register is to write the PC,
   * but MASE does not support this feature since the debugger is at
   * commit. */
  if (is_write) return "continue at addr not supported in MASE";

  switch (rt) {

  case rt_gpr:
    if (reg < 0 || reg >= MD_NUM_IREGS)
      return "register number out of range";
    val->type = et_uint;
    val->value.as_uint = regs->regs_R[reg];
    break;
  case rt_fpr:
    if (reg < 0 || reg >= MD_NUM_FREGS)
      return "register number out of range";
    val->type = et_float;
#if defined(TARGET_PISA)
    val->value.as_float = regs->regs_F.f[reg];
#elif defined(TARGET_ALPHA)
    val->value.as_float = regs->regs_F.d[reg];
#endif
    break;
  case rt_lpr:
    if (reg < 0 || reg >= MD_NUM_FREGS)
      return "register number out of range";
    val->type = et_uint;
#if defined(TARGET_PISA)
    val->value.as_uint = regs->regs_F.l[reg];
#elif defined(TARGET_ALPHA)
    val->value.as_uint = regs->regs_F.q[reg];
#endif
    break;
  case rt_ctrl:
    if (reg < 0 || reg >= MD_NUM_CREGS)
      return "register number out of range";
    val->type = et_uint;
    switch (reg) {
#if defined(TARGET_PISA)
      case 0: val->value.as_uint = regs->regs_C.hi; break;
      case 1: val->value.as_uint = regs->regs_C.lo; break;
      case 2: val->value.as_uint = regs->regs_C.fcc; break;
#elif defined(TARGET_ALPHA)
      case 0: val->value.as_uint = regs->regs_C.fpcr; break;
      case 1: val->value.as_uint = regs->regs_C.uniq; break;
#endif
      default: fatal("Invalid control register\n");
    }
    break;
  case rt_PC:
    val->type = et_addr;
    val->value.as_addr = regs->regs_PC;
    break;
  case rt_NPC:
    val->type = et_addr;
    val->value.as_addr = regs->regs_NPC;
    break;
  default:
    panic("bogus register bank");
  }

  /* no error */
  return NULL;
}

/* default machine state accessor, used by DLite */
char *						/* err str, NULL for no err */
mase_mstate_obj(FILE *stream,			/* output stream */
	       char *cmd,			/* optional command string */
	       struct regs_t *regs,		/* registers to access */
	       struct mem_t *mem)		/* memory space to access */
{
  if (!stream) stream = stderr;

  if (!cmd || !strcmp(cmd, "help"))
    fprintf(stream,
"mstate commands:\n"
"\n"
"    mstate help   - show all machine-specific commands (this list)\n"
"    mstate stats  - dump all statistical variables\n"
"    mstate fetch  - dump contents of fetch stage\n"
"    mstate ifq    - dump contents of instruction fetch queue\n"
"    mstate isq    - dump contents of instruction state queue\n"
"    mstate rename - dump contents of the rename table\n"
"    mstate rob    - dump contents of the reorder buffer\n"
"    mstate lsq    - dump contents of the load/store queue\n"
"    mstate rs     - dump contents of the reservation stations\n"
"    mstate readyq - dump contents of ready instruction queue\n"
"    mstate eventq - dump contents of event queue\n"
"    mstate res    - dump current functional unit resource states\n"
"\n"
  );
  else if (!strcmp(cmd, "stats")) {
    /* just dump intermediate stats */
    sim_print_stats(stream);
  }
  else if (!strcmp(cmd, "fetch")) {
    /* dump fetch stage contents */
    fetch_dump(stream);
  }
  else if (!strcmp(cmd, "ifq")) {
    /* dump IFQ contents */
    ifq_dump(stream);
  }
  else if (!strcmp(cmd, "isq")) {
    /* dump ISQ contents */
    isq_dump(stream);
  }
  else if (!strcmp(cmd, "rename")) {
    /* dump rename table contents */
    rename_dump(stream);
  }
  else if (!strcmp(cmd, "rob")) {
    /* dump ROB contents */
    rob_dump(stream);
  }
  else if (!strcmp(cmd, "lsq")) {
    /* dump LSQ contents */
    lsq_dump(stream);
  }
  else if (!strcmp(cmd, "rs")) {
    /* dump RS contents */
    rs_dump(stream);
  }
  else if (!strcmp(cmd, "readyq")) {
    /* dump ready queue contents */
    readyq_dump(stream);
  }
  else if (!strcmp(cmd, "eventq")) {
    /* dump event queue contents */
    eventq_dump(stream);
  }
  else if (!strcmp(cmd, "res"))  {
    /* dump resource state */
    res_dump(fu_pool, stream);
  }
  else
    return "unknown mstate command";

  /* no error */
  return NULL;
}

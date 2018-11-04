/* mase.c - micro architecture simulation environment */

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
#include "mase-macros-oracle.h"

/* initialize the simulator */
void
sim_init(void)
{
  sim_num_refs = 0;

  /* initialize register files */
  regs_init(&regs);
  regs_init(&pu_regs);

  /* allocate and initialize memory space */
  mem = mem_create("mem");
  mem_init(mem);
}

/* load program into simulated state */
void
sim_load_prog(char *fname,		/* program to load */
              int argc, char **argv,	/* program arguments */
              char **envp)		/* program environment */
{
  /* load program text and data, set up environment, memory, and regs */
  ld_load_prog(fname, argc, argv, envp, &regs, mem, TRUE);

  /* transfer registers to pre-update state */
  pu_regs = regs;

  /* initialize pipetrace */
  if (ptrace_nelt == 2) {
    /* generate a pipeline trace */
    ptrace_open(/* fname */ ptrace_opts[0], /* range */ptrace_opts[1]);
  }
  else if (ptrace_nelt == 0) {
      /* no pipetracing */;
  }
  else
    fatal("bad pipetrace args, use: <fname|stdout|stderr> <range>");

  /* finish initialization of the simulation engine */
  /* NOTE: the order of the init functions does matter in some cases */
  fu_pool = res_create_pool("fu-pool", fu_config, FU_NUM_INDICES);
  relink_init(MAX_RE_LINKS);
  fe_init();
  checker_init();
  rename_init();
  eventq_init();
  readyq_init();
  mpq_init();
  rs_init();
  rob_init();
  lsq_init();

  /* initialize the DLite debugger */
  dlite_init(mase_reg_obj, mase_mem_obj, mase_mstate_obj);
}

/* un-initialize the simulator */
void
sim_uninit(void)
{
  if (ptrace_nelt > 0) ptrace_close();

  if (((float) checker_errors / (float) sim_num_insn * 100.0) >= chk_threshold) {
    warn("Checker errors exceeded threshold");
  }
}

/* start simulation */
void
sim_main(void)
{
  /* ignore any floating point exceptions, they may occur on mis-speculated
     execution paths */
  signal(SIGFPE, SIG_IGN);

  /* set up program entry state */
  pu_regs.regs_PC  = ld_prog_entry;
  pu_regs.regs_NPC = pu_regs.regs_PC + sizeof(md_inst_t);

  /* check for DLite debugger entry condition */
  if (dlite_check_break(pu_regs.regs_PC, /* no access */0, /* addr */0, 0, 0))
    dlite_main(0, pu_regs.regs_PC, sim_cycle, &regs, mem);

  /* fast forward simulator loop: performs functional simulation for
     fastfwd_count insts and then turns on performance (timing) simulation */
  if (fastfwd_count > 0) {

    /* Note: Many of these variables are here because the macros expect
     * them to be declared.  Some are not used in the simpler fast forward
     * mode. */
    int icount;			/* count of fastfwd instrs */
    md_inst_t inst;		/* actual instruction bits */
    enum md_opcode op;		/* decoded opcode enum */
    enum md_fault_type fault;	/* fault information */
    md_addr_t target_addr;	/* target address */
    md_addr_t mem_addr;		/* address of load/store */
    union val_union mem_value;	/* temp variable for spec mem access */
    int mem_size;		/* size of memory value */
    int is_write;		/* store? */
    int fastfwd_mode = 1;	/* set so some macros behave differently */

    fprintf(stderr, "sim: ** fast forwarding %d insts **\n", fastfwd_count);

    /* Execute the instructions.  The pre-update registers are used since the macros are
     * already written using this file and since everything is in-order and non-speculative,
     * only one register file is needed.
     */ 
    for (icount=0; icount < fastfwd_count; icount++) {

      /* maintain $r0 semantics */
      pu_regs.regs_R[MD_REG_ZERO] = 0;
#if defined(TARGET_ALPHA)
      pu_regs.regs_F.d[MD_REG_ZERO] = 0.0;
#endif /* TARGET_ALPHA */

      /* get the next instruction to execute */
      MD_FETCH_INST(inst, mem, pu_regs.regs_PC);

      /* set default values */
      mem_addr = 0;
      is_write = FALSE;
      fault = md_fault_none;

      /* decode the instruction */
      MD_SET_OPCODE(op, inst);

#define DECLARE_FAULT(FAULT) { fault = (FAULT); break; }
#define SYSCALL(INST) sys_syscall(&pu_regs, mem_access, mem, INST, TRUE)

      /* execute the instruction */
      switch (op) {

#define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,I1,I2,I3)		\
      case OP:							\
        SYMCAT(OP,_IMPL);						\
        break;
#define DEFLINK(OP,MSK,NAME,MASK,SHIFT)					\
      case OP:							\
        panic("attempted to execute a linking opcode");
#define CONNECT(OP)
#include "machine.def"
      default:
        panic("attempted to execute a bogus opcode");
      }
          
      /* check for faults */
      if (fault != md_fault_none)
        fatal("fault (%d) detected @ 0x%08p", fault, pu_regs.regs_PC);

      /* check for writes */
      if (MD_OP_FLAGS(op) & F_STORE) is_write = TRUE;

      /* check for DLite debugger entry condition */
      if (dlite_check_break(pu_regs.regs_NPC,
	                    is_write ? ACCESS_WRITE : ACCESS_READ,
	                    mem_addr, sim_num_insn, sim_num_insn)
      ) {
        dlite_main(pu_regs.regs_PC, pu_regs.regs_NPC, sim_num_insn, &pu_regs, mem);
      }

      /* go to the next instruction */
      pu_regs.regs_PC = pu_regs.regs_NPC;
      pu_regs.regs_NPC += sizeof(md_inst_t);
    }

    /* Now that fastforwarding is complete, make sure both register files are the same. */
    regs = pu_regs;
  }

  /* set up the PC registers in the fetch stage. */
  fetch_setup_PC(pu_regs.regs_PC);

  fprintf(stderr, "sim: ** starting performance simulation **\n");

  /* main simulator loop, NOTE: the pipe stages are traverse in reverse order
     to eliminate this/next state synchronization and relaxation problems */
  for (;;) {

    /* ROB/LSQ sanity checks */
    if (ROB_num < LSQ_num) panic("ROB_num < LSQ_num");
    if (((ROB_head + ROB_num) % ROB_size) != ROB_tail) panic("ROB_head/ROB_tail wedged");
    if (((LSQ_head + LSQ_num) % LSQ_size) != LSQ_tail) panic("LSQ_head/LSQ_tail wedged");

    /* check if pipetracing is still active */
    ptrace_check_active(pu_regs.regs_PC, sim_num_insn, sim_cycle);

    /* indicate new cycle in pipetrace */
    ptrace_newcycle(sim_cycle);

    /* commit entries from ROB/LSQ to architected register file */
    mase_commit();

    /* service function unit release events */
    mase_release_fu();

    /* service result completions, also readies dependent operations */
    /* ==> inserts operations into ready queue --> register deps resolved */
    mase_writeback();

    /* try to locate memory operations that are ready to execute */
    /* ==> inserts operations into ready queue --> mem deps resolved */
    lsq_refresh();

    /* issue operations ready to execute from a previous cycle */
    /* <== drains ready queue <-- ready operations commence execution */
    mase_issue();

    /* decode and dispatch new operations */
    /* ==> insert ops w/ no deps or all regs ready --> reg deps resolved */
    mase_dispatch();

    /* call instruction fetch unit */
    mase_fetch();

#ifdef TEST_CB
    cb_tester();
#endif

    /* update buffer occupancy stats */
    IFQ_count += IFQ_num;
    IFQ_fcount += ((IFQ_num == IFQ_size) ? 1 : 0);
    ISQ_count += ISQ_num;
    ISQ_fcount += ((ISQ_num == ISQ_size) ? 1 : 0);
    RS_count += RS_num;
    RS_fcount += ((RS_num == RS_size) ? 1 : 0);
    ROB_count += ROB_num;
    ROB_fcount += ((ROB_num == ROB_size) ? 1 : 0);
    LSQ_count += LSQ_num;
    LSQ_fcount += ((LSQ_num == LSQ_size) ? 1 : 0);

    /* go to next cycle */
    sim_cycle++;

    /* finish early? */
    if (max_insts && sim_num_insn >= max_insts) return;
    if (program_complete) return;
  }
}

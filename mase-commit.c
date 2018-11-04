/* mase-commit.c - sim-mase commit stage */

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

/*
 * MASE_COMMIT() - instruction retirement pipeline stage
 *
 * Commits the results of the oldest completed entries from the
 * ROB and LSQ to the architected reg file. Stores in the LSQ will commit
 * their store data to the data cache as well. */
void
mase_commit(void)
{
  int i;				/* loop traversal variable */
  int lat;				/* latency of store */
  int events;				/* summary of events */
  int is_write;				/* set if instr is store */
  int lsq_chk_error;			/* set if checker occurs for LSQ entry */
  int committed = 0;			/* number of instrs committed */
  int dlite_made_check = FALSE;		/* make sure debugger is run if stepping */ 
  md_addr_t mem_addr = 0;		/* address of memory operation */

  /* all values must be retired to the architected reg file in program order */
  while (ROB_num > 0 && committed < commit_width) {

    struct ROB_entry *re = &(ROB[ROB_head]);

    /* is ROB entry complete? */
    if (!re->completed) break;

    /* default values */
    events = 0;
    is_write = FALSE;
    lsq_chk_error = 0;

    /* loads/stores must retire LSQ entry as well */
    if (ROB[ROB_head].ea_comp) {

      /* check to see that ROB and LSQ are in sync */
      if (LSQ_num <= 0 || !LSQ[LSQ_head].in_LSQ)
	panic("ROB out of sync with LSQ");

      /* is LSQ entry complete? */
      if (!LSQ[LSQ_head].completed) break;

      /* grab the effective memory address */
      mem_addr = read_idep_list(&LSQ[LSQ_head], DEP_NAME(DTMP)).addr;

      /* check for a store - stores must retire their store
       * value to the cache at commit */
      if ((MD_OP_FLAGS(LSQ[LSQ_head].op) & (F_MEM|F_STORE))
	== (F_MEM|F_STORE))
      {
        struct res_template *fu;

        is_write = TRUE;

	/* try to get a store port (functional unit allocation) */
	fu = res_get(fu_pool, MD_OP_FUCLASS(LSQ[LSQ_head].op));

	if (fu) {

	  /* access the D-TLB */
	  if (dtlb) {
	    enum mem_status status = mase_cache_access(
	       dtlb, Read, (mem_addr & ~(LSQ[LSQ_head].mem_size-1)),
	       NULL, LSQ[LSQ_head].mem_size, sim_cycle,
	       NULL, NULL, NULL, 0, &lat);
            if (status == MEM_RETRY) break;
	    if (lat > 1) events |= PEV_TLBMISS;
	  }

	  /* access the data cache */
	  if (cache_dl1) {
	    enum mem_status status = mase_cache_access(
	      cache_dl1, Write, (mem_addr & ~(LSQ[LSQ_head].mem_size-1)),
	      NULL, LSQ[LSQ_head].mem_size, sim_cycle,
	      NULL, NULL, NULL, 0, &lat);
            if (status == MEM_RETRY) break;
	    if (lat > cache_dl1_lat) events |= PEV_CACHEMISS;
	  }

	  /* schedule functional unit release event */
	  if (fu->master->busy) panic("functional unit already in use");
	  fu->master->busy = fu->issuelat;
	}
	else {
	  /* no store ports left, cannot continue to commit insts */
	  break;
	}
      }

      /* verify the LSQ entry */
      lsq_chk_error = checker_check(&(LSQ[LSQ_head]), &(ISQ[ISQ_head]));

      /* remove the rename table entry */
      for (i=0; i<MAX_ODEPS; i++) {
        rename_remove_entry(LSQ[LSQ_head].odep_rename[i], LSQ[LSQ_head].odep_name[i]);
        LSQ[LSQ_head].odep_rename[i] = NULL;
      }

      /* invalidate load/store operation instance */
      LSQ[LSQ_head].tag++;
      sim_slip += (sim_cycle - LSQ[LSQ_head].slip);
   
      /* indicate to pipeline trace that this instruction retired */
      ptrace_newstage(LSQ[LSQ_head].ptrace_seq, PST_COMMIT, events);
      ptrace_endinst(LSQ[LSQ_head].ptrace_seq);

      /* commit head of LSQ */
      LSQ_head = (LSQ_head + 1) % LSQ_size;
      LSQ_num--;
    }

    /* update branch predictor if appropriate */
    if (pred && bpred_spec_update == spec_CT
        && (MD_OP_FLAGS(re->op) & F_CTRL))
    {
      bpred_update(pred,
	/* branch address */		re->PC,
	/* actual target address */	re->NPC,
	/* taken? */			re->NPC != (re->PC + sizeof(md_inst_t)),
	/* pred taken? */		re->pred_NPC != (re->PC + sizeof(md_inst_t)),
	/* correct pred? */		re->pred_NPC == re->NPC,
	/* opcode */			re->op,
	/* dir predictor update ptr */	&re->dir_update
      );
    }

    /* inject errors into the checker, if desired */
    if (inject_errors) {
      if (rand() % 10 == 0) re->fault = 43;
    }

    /* check and commit the instruction to architected state */
    if (checker_commit(re, lsq_chk_error)) events |= PEV_CHKERROR;

//    printf("\n%lx %lld %lx", re->IR, re->PC, re->odep_value[0]);
    enum md_opcode op;
    MD_SET_OPCODE(op, re->IR);

//    printf("\n%x %lx %d", re->IR, re->PC, op);

    fflush(stdout);
//    if (re->PC == 4832000340){
//	    printf("op: %d\n", re->op); while(1);}

    /* remove the rename table entry */
    for (i=0; i<MAX_ODEPS; i++) {
      rename_remove_entry(re->odep_rename[i], re->odep_name[i]);
      re->odep_rename[i] = NULL;
    }

    /* invalidate ROB operation instance */
    ROB[ROB_head].tag++;
    sim_slip += (sim_cycle - ROB[ROB_head].slip);

    /* print retirement trace if in verbose mode */
    if (verbose &&
        (sim_num_insn >= trigger_inst)  &&
        (!max_insts || (sim_num_insn <= max_insts)))
    {
      myfprintf(stderr, "COMMIT: %10n [xor: 0x%08x] @ 0x%08p: ",
                sim_num_insn, md_xor_regs(&regs), regs.regs_PC);
      md_print_insn(ROB[ROB_head].IR, regs.regs_PC, stderr);
      fprintf(stderr, "\n"); 
      if (verbose_regs) {
        md_print_iregs(regs.regs_R, stderr);
        md_print_fpregs(regs.regs_F, stderr);
        md_print_cregs(regs.regs_C, stderr);
        fprintf(stderr, "\n");
      }
    }

    /* indicate to pipeline trace that this instruction retired */
    ptrace_newstage(ROB[ROB_head].ptrace_seq, PST_COMMIT, events);
    ptrace_endinst(ROB[ROB_head].ptrace_seq);

    /* commit head entry of ROB */
    ROB_head = (ROB_head + 1) % ROB_size;
    ROB_num--;

    /* one more instruction committed to architected state */
    committed++;

    /* ensure that instruction has no odeps */
    for (i=0; i<MAX_ODEPS; i++) {
      if (re->odep_list[i]) panic ("retired instruction has odeps\n");
    }

    /* check for DLite debugger entry condition */
    dlite_made_check = TRUE;
    if (dlite_check_break(regs.regs_NPC,
        is_write ? ACCESS_WRITE : ACCESS_READ,
        mem_addr, sim_num_insn, sim_cycle)
    ) {
      dlite_main(regs.regs_PC, regs.regs_NPC, sim_cycle, &regs, mem);
    }
  }

  /* need to enter DLite at least once per cycle */
  if (!dlite_made_check) {
    if (dlite_check_break(0, 0, 0, sim_num_insn, sim_cycle))
      dlite_main(regs.regs_PC, 0, sim_cycle, &regs, mem);
  }
}

/*
 * ROB_RECOVER() - squash mispredicted microarchitecture state
 *
 * Recover processor microarchitecture state back to point at
 * ROB[recover_index]. */
void
rob_recover(int recover_index)		/* index of instruction invoking recovery */
{
  int i; 
  int ROB_index = ROB_tail;
  int LSQ_index = LSQ_tail;
  int ROB_prev_tail = ROB_tail;
  int LSQ_prev_tail = LSQ_tail;

  /* get the starting points */
  ROB_index = (ROB_index + (ROB_size-1)) % ROB_size;
  LSQ_index = (LSQ_index + (LSQ_size-1)) % LSQ_size;

  /* Recover from the tail of the ROB towards the head until the recovery index
   * is reached. This direction ensures that the LSQ can be synchronized with
   * the ROB */
  while (ROB_index != recover_index) {

    /* the ROB should not drain since the recovery instruction will remain */
    if (!ROB_num) panic("empty ROB");
	
    /* should meet up with the recovery index first */
    if (ROB_index == ROB_head) panic("ROB head and tail broken");

    /* is this operation an effective addr calc for a load or store? */
    if (ROB[ROB_index].ea_comp) {

      /* should be at least one load or store in the LSQ */
      if (!LSQ_num) panic("ROB and LSQ out of sync");

      /* recover any resources consumed by this load or store operation */
      for (i=0; i<MAX_ODEPS; i++) {
        rename_remove_entry(LSQ[LSQ_index].odep_rename[i], LSQ[LSQ_index].odep_name[i]);
        RELINK_FREE_LIST(LSQ[LSQ_index].odep_list[i]);
        LSQ[LSQ_index].odep_rename[i] = NULL;
        LSQ[LSQ_index].odep_list[i] = NULL;
      }
     
      /* squash this LSQ entry */
      LSQ[LSQ_index].tag++;

      /* indicate in pipetrace that this instruction was squashed */
      ptrace_endinst(LSQ[LSQ_index].ptrace_seq);

      /* go to next earlier LSQ slot */
      LSQ_prev_tail = LSQ_index;
      LSQ_index = (LSQ_index + (LSQ_size-1)) % LSQ_size;
      LSQ_num--;
    }

    /* recover any resources used by this ROB operation */
    for (i=0; i<MAX_ODEPS; i++) {
      rename_remove_entry(ROB[ROB_index].odep_rename[i], ROB[ROB_index].odep_name[i]);
      RELINK_FREE_LIST(ROB[ROB_index].odep_list[i]);
      ROB[ROB_index].odep_rename[i] = NULL;
      ROB[ROB_index].odep_list[i] = NULL;
    }
      
    /* squash this ROB entry */
    ROB[ROB_index].tag++;

    /* indicate in pipetrace that this instruction was squashed */
    ptrace_endinst(ROB[ROB_index].ptrace_seq);

    /* go to next earlier ROB slot */
    ROB_prev_tail = ROB_index;
    ROB_index = (ROB_index + (ROB_size-1)) % ROB_size;
    ROB_num--;
  }

  /* reset head and tail pointers to point of recovery */
  ROB_tail = ROB_prev_tail;
  LSQ_tail = LSQ_prev_tail;

  /* fix reservation station entries */
  RS_num = 0;

  ROB_index = ROB_tail;
  ROB_index = (ROB_index + (ROB_size-1)) % ROB_size;

  if(scheduler_replay) {
    while(ROB_index != ROB_head) {
      if(!(ROB[ROB_index].completed)) {
	RS_num++;
      }
      ROB_index = (ROB_index + (ROB_size-1)) % ROB_size;
    }
    if(!(ROB[ROB_head].completed)) {
      RS_num++;
    }
  } else {
    while(ROB_index != ROB_head) {
      if(!(ROB[ROB_index].issued)) {
	RS_num++;
      }
      ROB_index = (ROB_index + (ROB_size-1)) % ROB_size;
    }
    if(!(ROB[ROB_head].issued)) {
      RS_num++;
    }
  }

  if(RS_num > RS_size) {
    panic("Too many reservation stations after recovery");
  }
  
  /* FIXME: could reset functional units at squash time */
}

/* 
 * MASE_RELEASE_FU() - service all FU release events.
 *
 * This function is called once per cycle and is used to step the BUSY timers
 * to each FU in the FU resouce pool. As long as a FU's busy count is greater
 * than zero, it cannot be issued an operation. */
void
mase_release_fu(void)
{
  int i;

  /* walk all resource units, decrement busy counts by one */
  for (i=0; i<fu_pool->num_resources; i++) {

    /* resource is released when BUSY hits zero */
    if (fu_pool->resources[i].busy > 0)
      fu_pool->resources[i].busy--;
  }
}

/*
 * MASE_WRITEBACK() - instruction result writeback pipeline stage
 *
 * Process all of the instructions that have completed.  The output
 * dependency chains of the completing instructions are walked to determine
 * if any dependent instruction now has all of its register operands.
 * If so, the ready instruction is inserted into the ready queue. */
void
mase_writeback(void)
{
  int i;
  struct ROB_entry *re;

  /* service all completed events */
  while ((re = eventq_next_event())) {

    if (!OPERANDS_READY(re) || re->queued || !re->issued || re->completed) {
      panic("inst completed and ready, !issued, or completed");
    }

    /* For DTLB misses, place instruction back on ready queue for cache access. */
    if (re->dtlb_miss) {
      re->issued = FALSE;
      readyq_enqueue(re);
      continue;
    }

    /* mark operation as completed */
    re->completed = TRUE;

    /* release reservation station entry if we're using replay.
       if we're not, we do this when instruction is issued */
    if(scheduler_replay)
      if (!(re->in_LSQ)) RS_num--;

    /* does this operation reveal a mis-predicted branch? */
    if (re->recover_inst) {
      if (re->in_LSQ) panic("mis-predicted load or store?!?!?");
    
      /* recover processor state */
      checker_recover(re->isq_index);
      rob_recover(re - ROB);
      fe_recover(re->NPC);
      bpred_recover(pred, re->PC, re->stack_recover_idx);
    }

    /* if we speculatively update branch-predictor, do it here */
    if (pred
	&& bpred_spec_update == spec_WB
	&& !re->in_LSQ
	&& (MD_OP_FLAGS(re->op) & F_CTRL)
	&& (!ISQ[re->isq_index].spec_mode)
    ) {
      bpred_update(pred,
	/* branch address */	    re->PC,
	/* actual target address */ re->NPC,
	/* taken? */		    re->NPC != (re->PC + sizeof(md_inst_t)),
	/* pred taken? */	    re->pred_NPC != (re->PC + sizeof(md_inst_t)),
        /* correct pred? */	    re->pred_NPC == re->NPC,
        /* opcode */		    re->op,
        /* dir predictor update pointer */ &re->dir_update
      );
    }

    /* entered writeback stage - indicate in pipe trace */
    ptrace_newstage(re->ptrace_seq, PST_WRITEBACK,
		    re->recover_inst ? PEV_MPDETECT : 0);

    /* Broadcast results to consuming operations. This is more efficiently
     * accomplished by walking the output dependency chains of the
     * completed instruction. */
    for (i=0; i<MAX_ODEPS; i++) {

      struct RE_link *olink, *olink_next;

      /* mark output valid */
      re->odep_valid[i] = TRUE;

      /* walk output list -  queue up ready operations */
      for (olink=re->odep_list[i]; olink; olink=olink_next) {

        if (RELINK_VALID(olink)) {
          if (olink->re->idep_ready[olink->x.opnum])
            panic("output dependence already satisfied");

          /* input is now ready */
	  olink->re->idep_ready[olink->x.opnum] = TRUE;
	  olink->re->idep_value[olink->x.opnum] = re->odep_value[i];

	  /* Are all the register operands of target ready? */
	  if (OPERANDS_READY(olink->re)) {

	    /* All operands are ready. Add instruction to ready
             * queue unless it is a load operation.  Loads are
             * handled in lsq_refresh(). */
	    if (!olink->re->in_LSQ
	      || ((MD_OP_FLAGS(olink->re->op)&(F_MEM|F_STORE))
	      == (F_MEM|F_STORE))
            )
	      readyq_enqueue(olink->re);
	  }
	}

	/* grab link to next element prior to free */
	olink_next = olink->next;

	/* free dependence link element */
	RELINK_FREE(olink);
      }

      /* blow away the consuming op list */
      re->odep_list[i] = NULL;
    }
  }
}

/* mase-fe.c - sim-mase front-end */

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
#include "mase-decode.h"

/* GLOBAL VARIABLES */

/* Instruction Fetch Queue (Transfers instruction from FETCH to DISPATCH) */
struct fetch_rec *IFQ;
int IFQ_num;
int IFQ_tail, IFQ_head;

/* branch predictor */
struct bpred_t *pred;

/* rename table */
struct rename_entry *rename_table[MD_TOTAL_REGS];

/* rename free list */
static struct rename_entry *rename_free_list = NULL;

/* icache fetch blocking variables */
static int icache_block = FALSE;
static unsigned icache_delay = 0;

/* set if there is an unknown target */
static int unknown_target = FALSE;

/* speculation mode bit in dispatch */
static int dp_spec_mode = FALSE;

/* PC values in the fetch stage */
static md_addr_t fetch_PC;
static md_addr_t fetch_pred_NPC;

/* PC values in the dispatch stage */
static md_addr_t dp_PC;
static md_addr_t dp_pred_NPC;
static md_addr_t dp_oracle_NPC;

/* pipetrace variables */
static int last_inst_missed = FALSE;
static int last_inst_tmissed = FALSE;

/* pipetrace instruction sequence counter */
static unsigned int ptrace_seq = 0;

/* instruction sequence counter, used to assign unique id's to insts */
static unsigned int inst_seq = 0;

/* helper structures for keeping track of in-order issue */
#define RELINK_NULL_DATA { NULL, NULL, 0 }
static struct RE_link last_op = RELINK_NULL_DATA;
static struct RE_link RELINK_NULL = RELINK_NULL_DATA;

/* CALLBACK FUNCTION */

/* This callbck function is called by the memory system once the
 * latency of a memory operation is known.  This function unblocks
 * fetch and sets the delay to lat. Fetch will be unblocked after
 * lat cycles (unless something else happens in the interim). */
void
fetch_cb(unsigned int rid, unsigned int lat)
{
  icache_delay = lat;
  icache_block = FALSE;
}

/* RENAME TABLE FUNCTIONS */

/* initialize the rename table */
void
rename_init(void)
{
  int i;

  /* initialize rename table */
  for (i=0; i < MD_TOTAL_REGS; i++) rename_table[i] = NULL;

  /* create a rename_free_list to avoid calloc and free */
  rename_free_list = (struct rename_entry *)
    calloc(ISQ_size + 4, sizeof(struct rename_entry));
  for (i=0; i < ISQ_size + 3; i++) {
    rename_free_list[i].next = &(rename_free_list[i+1]);
  }
  rename_free_list[ISQ_size + 3].next = NULL;

}

/* get an entry from the rename_free_list */
struct rename_entry *
rename_get_entry(void)
{
  struct rename_entry *ent;

  if (rename_free_list == NULL) {
    ent = calloc(1, sizeof(struct rename_entry));
  }
  else {
    ent = rename_free_list;
    rename_free_list = rename_free_list->next;
  }

  return ent;
}

/* dump the contents of the rename table */
void
rename_dump(FILE *stream)
{
  int i;
  struct rename_entry *ent;

  fprintf(stream, "** rename table state **\n");

  for (i=0; i < MD_TOTAL_REGS; i++) {
    ent = rename_table[i];
    if (ent) fprintf(stream, "[%5s]: ", REG_NAME(i));
    while (ent) {
      fprintf(stream, "%s %2d",
        (ent->re->in_LSQ ? "LSQ" : "ROB"),
        (int)(ent->re - (ent->re->in_LSQ ? LSQ : ROB))
      );
      ent = ent->next;
      fprintf(stream, "%s", ent ? ", " : "\n");
    }
  }
}

/* removes a rename table entry */
void
rename_remove_entry(struct rename_entry *ent, int odep_name)
{
  if (ent == NULL) return;

  if (ent->prev == NULL)
    rename_table[odep_name] = ent->next;
  else
    ent->prev->next = ent->next;
  if (ent->next != NULL) ent->next->prev = ent->prev;

  ent->next = rename_free_list;
  rename_free_list = ent;
}

/* FRONT END FUNCTIONS */

/* initialize the front end */
void
fe_init(void)
{
  /* allocate the IFQ */
  IFQ = (struct fetch_rec *)calloc(IFQ_size, sizeof(struct fetch_rec));
  if (!IFQ) fatal("out of virtual memory");

  /* initialize IFQ */
  IFQ_num = 0;
  IFQ_tail = IFQ_head = 0;
  IFQ_count = 0;
  IFQ_fcount = 0;

  /* initialize flags */
  unknown_target = FALSE;
  dp_spec_mode = FALSE;
}

/* recover front end from misprediction */
void
fe_recover(md_addr_t recover_PC)
{
  /* Update the recovery count.  It is done here since all recoveries must
   * reset the front-end. */
  recovery_count++;

  /* clear flags */
  unknown_target = FALSE;
  dp_spec_mode = FALSE;

  /* if pipetracing, indicate squash of instructions in the IFQ */
  if (ptrace_active) {
    while (IFQ_num != 0) {
      /* squash the next instruction from the IFQ */
      ptrace_endinst(IFQ[IFQ_head].ptrace_seq);

      /* consume instruction from the IFQ */
      IFQ_head = (IFQ_head + 1) % IFQ_size;
      IFQ_num--;
    }
  }

  /* reset IFQ */
  IFQ_num = 0;
  IFQ_tail = 0;
  IFQ_head = 0;

  /* reset NPC values */
  fetch_pred_NPC = recover_PC;
  pu_regs.regs_NPC = recover_PC;
}

/* dump contents of fetch queue */
void
ifq_dump(FILE *stream)			/* output stream */
{
  int num, head;

  fprintf(stream, "** IFQ contents **\n");
  fprintf(stream, "IFQ_num: %d\n", IFQ_num);
  fprintf(stream, "IFQ_head: %d, IFQ_tail: %d\n",
	  IFQ_head, IFQ_tail);

  num = IFQ_num;
  head = IFQ_head;

  while (num) {
    fprintf(stream, "idx: %2d inst: `", head);
    md_print_insn(IFQ[head].IR, IFQ[head].PC, stream);
    fprintf(stream, "'\n");

    myfprintf(stream, "\tPC: 0x%08p, pred_NPC: 0x%08p\n",
      IFQ[head].PC, IFQ[head].pred_NPC);
    myfprintf(stream, "\tisq_index: %0d\n", IFQ[head].isq_index);

    head = (head + 1) & (IFQ_size - 1);
    num--;
  }
}

/*  DECODE AND DISPATCH STAGE - decode instructions and allocate ROB and LSQ resources */

/* Link ROB_entry onto the output chain number of whichever operation
 * will next create the architected register value idep_name.
 */
INLINE void
rob_link_idep(struct ROB_entry *re,		/* re entry to link */
	      int idep_num,			/* input dependence number */
	      int reg_name)			/* input register name */
{
  struct rename_entry *rt_ent;
  struct RE_link *re_link;
  enum val_type idep_type = GET_TYPE(reg_name);
  int idep_name = DEP_NAME(reg_name);

  re->idep_name[idep_num] = idep_name;

  /* check for special values for the register name */
  if (idep_name == DNA) {
    re->idep_ready[idep_num] = TRUE;
    return;
  }
  if (idep_name == DGPR(MD_REG_ZERO)) {
    re->idep_ready[idep_num] = TRUE;
    re->idep_value[idep_num].q = 0;
    return;
  }
#if defined(TARGET_ALPHA)
  if (idep_name == DFPR(MD_REG_ZERO)) {
    re->idep_ready[idep_num] = TRUE;
    re->idep_value[idep_num].d = 0.0;
    return;
  }
#endif

  /* locate creator of operand */
  rt_ent = rename_table[idep_name];

  /* any creator? */
  if (!rt_ent || !rt_ent->re) {

    /* no active creator, use value available in architected reg file */
#if defined(TARGET_PISA)
    if (REG_IS_FP(idep_name)) {
      switch (idep_type) {
      case vt_sfloat:
        re->idep_value[idep_num].f = regs.regs_F.f[FP_REG_INDEX(idep_name)];
        break;
      case vt_dfloat:
        re->idep_value[idep_num].d = regs.regs_F.d[FP_REG_INDEX(idep_name) >> 1];
        break;
      default:
	fatal("Invalid type in rob_link_idep");
      }
    }
    else if (idep_name == DFCC) {
      re->idep_value[idep_num].sw = regs.regs_C.fcc;
    }
    else if (idep_name == DHI) {
      re->idep_value[idep_num].sw = regs.regs_C.hi;
    }
    else if (idep_name == DLO) {
      re->idep_value[idep_num].sw = regs.regs_C.lo;
    }
    else { 	
      re->idep_value[idep_num].sw = regs.regs_R[INT_REG_INDEX(idep_name)];
    }
#elif defined(TARGET_ALPHA)
    if (REG_IS_FP(idep_name)) {
      re->idep_value[idep_num].d = regs.regs_F.d[FP_REG_INDEX(idep_name)];
    }
    else if (idep_name == DFPCR) {
      re->idep_value[idep_num].q = regs.regs_C.fpcr;
    }
    else if (idep_name == DUNIQ) {
      re->idep_value[idep_num].q = regs.regs_C.uniq;
    }
    else {
      re->idep_value[idep_num].q = regs.regs_R[INT_REG_INDEX(idep_name)];
    }
#endif
    re->idep_ready[idep_num] = TRUE;
  }
  else if (rt_ent->re->odep_valid[rt_ent->odep_num]) {	/* active creator - completed */

#if defined(TARGET_PISA)
    if (REG_IS_FP(idep_name)) {
      switch (idep_type) {
      case vt_sfloat:
        re->idep_value[idep_num].f = rt_ent->re->odep_value[rt_ent->odep_num].f;
	break;
      case vt_dfloat:
        re->idep_value[idep_num].d = rt_ent->re->odep_value[rt_ent->odep_num].d;
	break;
      default:
	fatal("Invalid type in rob_link_idep");
      }
    }
    else {
      re->idep_value[idep_num].sw = rt_ent->re->odep_value[rt_ent->odep_num].sw;
    }
#elif defined(TARGET_ALPHA)
    if (REG_IS_FP(idep_name)) 
      re->idep_value[idep_num].d = rt_ent->re->odep_value[rt_ent->odep_num].d;
    else
      re->idep_value[idep_num].q = rt_ent->re->odep_value[rt_ent->odep_num].q;
#endif

    re->idep_ready[idep_num] = TRUE;
  }
  else {		/* active creator - not completed */

    /* link onto creator's output list of dependent operands */
    RELINK_NEW(re_link, re);
    re_link->x.opnum = idep_num;
    re_link->next = rt_ent->re->odep_list[rt_ent->odep_num];
    rt_ent->re->odep_list[rt_ent->odep_num] = re_link;

    re->idep_ready[idep_num] = FALSE;
  }
}

/* Make re the creator of architected register odep_name. */
INLINE void
rob_install_odep(struct ROB_entry *re,	        /* creating ROB entry */
		 int odep_num,			/* output operand number */
		 int reg_name)			/* output register name */
{
  struct rename_entry *rt_ent;
  int odep_name = DEP_NAME(reg_name);

  /* initialize fields in the ROB entry */
  re->odep_name[odep_num] = odep_name;
  re->odep_list[odep_num] = NULL;

  /* check to see if there is a dependence */
#if defined(TARGET_PISA)
  if ((odep_name == DNA) || (odep_name == DGPR(MD_REG_ZERO))) {
#elif defined(TARGET_ALPHA)
  if ((odep_name == DNA) || (odep_name == DGPR(MD_REG_ZERO)) || (odep_name == DFPR(MD_REG_ZERO))) {
#endif
    re->odep_rename[odep_num] = NULL;
    re->odep_valid[odep_num] = TRUE;
    return;
  }

  /* Indicate this operation is latest creator of odep_name by adding
   * it to the head of the doubly-linked rename table entry list. */
  rt_ent = rename_get_entry();
  rt_ent->re = re;
  rt_ent->odep_num = odep_num;
  rt_ent->next = rename_table[odep_name];
  rt_ent->prev = NULL;
  if (rt_ent->next != NULL) rt_ent->next->prev = rt_ent;
  rename_table[odep_name] = rt_ent;

  /* store the relevant info in the ROB entry */
  re->odep_rename[odep_num] = rt_ent;
  re->odep_valid[odep_num] = FALSE;
}

/* Dispatch instructions from the IFQ: instructions are allocated ROB and
   LSQ (for load/stores) resources.  Input and output dependence chains
   are updated accordingly. */
void
mase_dispatch(void)
{
  enum md_opcode op;			/* decoded opcode enum */
  int br_taken;				/* if branch, is it taken? */
  int br_pred_taken;			/* if branch, is it predicted taken? */
  int n_dispatched;			/* total insts dispatched */
  int misfetch_only;			/* set to avoid branch mispredictions */
  struct ROB_entry *re;		        /* ROB entry being allocated */
  struct ROB_entry *lsq;		/* LSQ entry for ld/st's */
  int isq_index;			/* index of oracle data */
  md_inst_t inst;			/* actual instruction bits */
  struct bpred_update_t *dir_update_ptr;/* branch predictor dir update ptr */
  int stack_recover_idx;		/* bpred stack recovery index */	
  unsigned int pseq;			/* pipetrace sequence number */
  int mem_size;				/* size of the memory operation */
  int in1, in2, in3, out1, out2;	/* input/output register names */
  md_addr_t target_addr;		/* target address */
  enum dp_stall_t dp_stall_type;	/* decode stall type */

  /* initialize variables */
  n_dispatched = 0;
  misfetch_only = FALSE;

  while (
    /* instruction decode B/W left? */
    n_dispatched < (decode_width * fetch_speed) &&
    /* ROB not full? */
    ROB_num < ROB_size && 
    /* reservation station available? */
    RS_num < RS_size &&
    /* insts still available from fetch unit? */
    IFQ_num != 0 &&
    /* head inst not subject to IFQ delay */
    (IFQ[IFQ_head].timestamp + IFQ_delay) < sim_cycle &&
    /* block spec insts if include_spec == 0 */
    (include_spec || !dp_spec_mode)
  ) {

    /* If issuing in-order, block until last op issues. */
    if (inorder_issue
      && (last_op.re && RELINK_VALID(&last_op)
      && !OPERANDS_READY(last_op.re)))
    {
      break;
    }

    /* Get the next instruction from the IFQ, including some
     * decode information. */
    inst = IFQ[IFQ_head].IR;
    isq_index = IFQ[IFQ_head].isq_index;
    dp_PC = IFQ[IFQ_head].PC;
    dp_pred_NPC = IFQ[IFQ_head].pred_NPC;
    dir_update_ptr = &(IFQ[IFQ_head].dir_update);
    stack_recover_idx = IFQ[IFQ_head].stack_recover_idx;
    pseq = IFQ[IFQ_head].ptrace_seq;
    out1 = IFQ[IFQ_head].out1;
    out2 = IFQ[IFQ_head].out2;
    in1 = IFQ[IFQ_head].in1;
    in2 = IFQ[IFQ_head].in2;
    in3 = IFQ[IFQ_head].in3;
    target_addr = IFQ[IFQ_head].target_addr;
    mem_size = IFQ[IFQ_head].mem_size;

    /* decode the inst */
    MD_SET_OPCODE(op, inst);

    /* Determine if dispatch needs to be stalled due to a full LSQ. */
    if ((MD_OP_FLAGS(op) & F_MEM) && (LSQ_num == LSQ_size)) break;

    /* Grab the actual NPC from the oracle. It is used to determine if the
     * branch was correctly predicted or not. */
    if (isq_index != INVALID_ISQ)
      dp_oracle_NPC = ISQ[isq_index].NPC;
    else
      dp_oracle_NPC = dp_PC + sizeof(md_inst_t);

    /* determine if the branch was taken and if it was predicted taken */
    br_taken = (dp_oracle_NPC != (dp_PC + sizeof(md_inst_t)));
    br_pred_taken = (dp_pred_NPC != (dp_PC + sizeof(md_inst_t)));

    /* Check to see if the branch is predicted taken, but the predicted target doesn't
     * match the computed target (i.e., misfetch).  In this case, update the fetch PC
     * and do a fetch squash. */
    if (((MD_OP_FLAGS(op) & (F_CTRL|F_DIRJMP)) == (F_CTRL|F_DIRJMP))
        && (target_addr != dp_pred_NPC) && br_pred_taken)
    {
      /* update misfetch count */
      misfetch_count++;
      recovery_count++;

      /* If there is a misfetch, the target address is used as the NPC.  This is either
       * the correct NPC if the branch was supposed to be taken or a different incorrect
       * NPC if the branch was supposed to be not taken.  In the former case, the flag
       * misfetch_only is set to prevent a full-fledge branch misprediction.  In the
       * latter, there is a misfetch now and a branch misprediction later. */
      fetch_pred_NPC = target_addr;
      if (target_addr == dp_oracle_NPC) {
        misfetch_only = TRUE;
        misfetch_only_count++;
      }

      /* reset the IFQ */ 
      IFQ_num = 1;
      IFQ_tail = (IFQ_head + 1) % IFQ_size;

      /* reset the ISQ and oracle state */
      pu_regs.regs_NPC = dp_oracle_NPC;
      isq_reset(isq_index);

      /* the target is known - unblocks the front end */
      unknown_target = FALSE;
    }

    /* check for invalid instructions */
    if (isq_index != INVALID_ISQ)
    {
      /*
       * for load/stores:
       *   idep #0     - store operand (value that is store'ed)
       *   idep #1, #2 - eff addr computation inputs (addr of access)
       *
       * resulting ROB/LSQ operation pair:
       *   ROB (effective address computation operation):
       *     idep #1, #2 - eff addr computation inputs (addr of access)
       *   LSQ (memory access operation):
       *     idep #0     - operand input (value that is store'd)
       *     idep #1     - eff addr computation result (from ROB op)
       *
       * effective address computation is transfered via the reserved
       * name DTMP
       */

      /* allocate reservation station */
      RS_num++;

      /* fill in ROB entry */
      re = &ROB[ROB_tail];
      re->slip = sim_cycle - 1;
      re->IR = inst;
      re->op = op;
      re->isq_index = isq_index;
      re->PC = dp_PC;
      re->NPC = 0;
      re->pred_NPC = dp_pred_NPC;
      re->in_LSQ = FALSE;
      re->ea_comp = FALSE;
      re->mem_value.q = 0;
      re->mem_size = 0;
      re->match_in_LSQ = FALSE;
      re->fault = md_fault_none;
      re->dtlb_miss = FALSE;
      re->rob_index = 0;
      re->recover_inst = FALSE;
      re->dir_update = *dir_update_ptr;
      re->stack_recover_idx = stack_recover_idx;
      re->timing_dep = 0;
      /* re->tag is already set */
      re->seq = ++inst_seq;
      re->ptrace_seq = pseq;
      re->blind_recover = FALSE;
      re->queued = re->issued = re->completed = FALSE;
      re->schedule_wait_time = schedule_delay;

      /* split ld/st's into two operations: eff addr comp + mem access */
      if (MD_OP_FLAGS(op) & F_MEM) {

	/* convert ROB operation from ld/st to an add (eff addr comp) */
	re->op = MD_AGEN_OP;
	re->ea_comp = TRUE;

	/* fill in LSQ entry */
	lsq = &LSQ[LSQ_tail];
        lsq->slip = sim_cycle - 1;
	lsq->IR = inst;
	lsq->isq_index = isq_index;
	lsq->op = op;
	lsq->PC = dp_PC;
	lsq->NPC = 0;
	lsq->pred_NPC = dp_pred_NPC;
	lsq->in_LSQ = TRUE;
	lsq->ea_comp = FALSE;
	if ((MD_OP_FLAGS(op) & (F_MEM|F_LOAD)) == (F_MEM|F_LOAD))
	  lsq->mem_value = ISQ[isq_index].mem_value;
	else
	  lsq->mem_value.q = 0;
	lsq->mem_size = mem_size;
	lsq->match_in_LSQ = FALSE;
	lsq->fault = md_fault_none;
	lsq->dtlb_miss = FALSE;
        lsq->rob_index = re - ROB;
	lsq->recover_inst = FALSE;
	lsq->dir_update.pdir1 = lsq->dir_update.pdir2 = NULL;
	lsq->dir_update.pmeta = NULL;
	lsq->stack_recover_idx = 0;
        lsq->timing_dep = 0;
	/* lsq->tag is already set */
	lsq->seq = ++inst_seq;
	lsq->ptrace_seq = pseq + 1;
        lsq->blind_recover = FALSE;
	lsq->schedule_wait_time = 0;   /* has to wait for agen anyways... */
	lsq->queued = lsq->issued = lsq->completed = FALSE;

	/* pipetrace this uop */
	ptrace_newuop(lsq->ptrace_seq, "internal ld/st", lsq->PC, 0);
	ptrace_newstage(lsq->ptrace_seq, PST_DISPATCH, 0);

	/* link eff addr computation onto operand's output chains */
	rob_link_idep(re, 0, DNA);
	rob_link_idep(re, 1, in2);
	rob_link_idep(re, 2, in3);

	/* install output after inputs to prevent self reference */
	rob_install_odep(re, 0, DTMP);
	rob_install_odep(re, 1, DNA);

	/* link memory access onto output chain of eff addr operation */
	rob_link_idep(lsq, STORE_OP_INDEX   /* 0 */, in1);
	rob_link_idep(lsq, STORE_ADDR_INDEX /* 1 */, DTMP);
	rob_link_idep(lsq, 2, DNA);

	/* install output after inputs to prevent self reference */
	rob_install_odep(lsq, 0, out1);
	rob_install_odep(lsq, 1, out2);

	/* install operation in the ROB and LSQ */
	n_dispatched++;
	ROB_tail = (ROB_tail + 1) % ROB_size;
	ROB_num++;
	LSQ_tail = (LSQ_tail + 1) % LSQ_size;
	LSQ_num++;

	/* if eff addr computation ready, queue it on ready list */
	if (OPERANDS_READY(re)) readyq_enqueue(re);

	/* in-order issue may continue */
	RELINK_INIT(last_op, lsq);

	/* queue stores only, loads are queued by lsq_refresh() */
	if (((MD_OP_FLAGS(op) & (F_MEM|F_STORE)) == (F_MEM|F_STORE))
	    && OPERANDS_READY(lsq))
	{
	  readyq_enqueue(lsq);
	}
      }
      else { /* !(MD_OP_FLAGS(op) & F_MEM) */

	/* link onto producing operation */
	rob_link_idep(re, /* idep_ready[] index */0, in1);
	rob_link_idep(re, /* idep_ready[] index */1, in2);
	rob_link_idep(re, /* idep_ready[] index */2, in3);

	/* install output after inputs to prevent self reference */
	rob_install_odep(re, /* odep_list[] index */0, out1);
	rob_install_odep(re, /* odep_list[] index */1, out2);

	/* install operation in the ROB */
	n_dispatched++;
	ROB_tail = (ROB_tail + 1) % ROB_size;
	ROB_num++;

	/* queue op if all its reg operands are ready (no mem input) */
	if (OPERANDS_READY(re)) {
	  readyq_enqueue(re);
	  last_op = RELINK_NULL;
	}
	else {
	  RELINK_INIT(last_op, re);
	}
      }
    }
    else {
      /* this is invalid instruction, no need to update ROB/LSQ state */
      re = NULL;
    }

    /* If this is a branching instruction, update BTB. */
    if (pred && 
        !dp_spec_mode &&
        bpred_spec_update == spec_ID &&
        (MD_OP_FLAGS(op) & F_CTRL)
    ) {
      bpred_update(pred,
        /* branch address */        dp_PC,
        /* actual target address */ dp_oracle_NPC,
        /* taken? */                dp_oracle_NPC != (dp_PC + sizeof(md_inst_t)),
        /* pred taken? */           dp_pred_NPC != (dp_PC + sizeof(md_inst_t)),
        /* correct pred? */         dp_pred_NPC == dp_oracle_NPC,
        /* opcode */                op,
        /* predictor update ptr */  &re->dir_update
      );
    }

    /* is the trace generator trasitioning into mis-speculation mode? */
    if (dp_pred_NPC != dp_oracle_NPC && !misfetch_only) {
      if (recover_in_shadow || !dp_spec_mode) re->recover_inst = TRUE;
      dp_spec_mode = TRUE;
    }

    /* update pipetrace */
    ptrace_newstage(pseq, PST_DISPATCH,
		    (dp_pred_NPC != dp_oracle_NPC) ? PEV_MPOCCURED : 0);
    if (op == MD_NOP_OP) ptrace_endinst(pseq);

    /* consume instruction from IFQ queue */
    IFQ_head = (IFQ_head + 1) % IFQ_size;
    IFQ_num--;
  }

  /* update decode stall array */
  if (n_dispatched >= (decode_width * fetch_speed))
    dp_stall_type = dp_no_stall;
  else if ((IFQ_num == 0) || (IFQ[IFQ_head].timestamp + IFQ_delay) >= sim_cycle)
    dp_stall_type = dp_ifq_empty;
  else if (RS_num >= RS_size)
    dp_stall_type = dp_rs_full;
  else if (LSQ_num >= LSQ_size)
    dp_stall_type = dp_lsq_full;
  else if (ROB_num >= ROB_size)
    dp_stall_type = dp_rob_full;
  else
    dp_stall_type = dp_other;

  stat_add_sample(dp_stall, (int)dp_stall_type); 
}

/*
 *  INSTRUCTION FETCH STAGE - instruction fetch pipeline stage(s)
 */

/* Initialize the values of PC at the beginning of the simulation.  This
 * is called after fastwording and before the main simulator loop. */
void
fetch_setup_PC(md_addr_t PC)			/* initial PC value */
{
  fetch_pred_NPC = PC;
  pu_regs.regs_NPC = PC;
}

/* dump contents of fetch stage */
void
fetch_dump(FILE *stream)			/* output stream */
{
  if (!stream) stream = stderr;
    
  fprintf(stream, "** fetch stage state **\n");

  myfprintf(stream, "fetch_PC: 0x%08p, fetch_pred_NPC: 0x%08p\n",
	    fetch_PC, fetch_pred_NPC);

  if (icache_block)
    fprintf(stream, "fetch blocked due to icache miss (lat: unknown)\n");
  else if (icache_delay)
    fprintf(stream, "fetch blocked due to icache miss (lat: %d)\n", icache_delay);
}

/* fetches instructions */
void
mase_fetch(void)
{
  int i;			/* loop iteration variable */
  int lat;			/* latency of TLB/cache access */
  enum mem_status status;	/* status of TLB/cache access */
  int miss;			/* set if TLB/cache misses */
  md_inst_t inst;		/* instruction bits from memory */
  int stack_recover_idx;	/* bpred retstack recovery index */
  int branch_cnt;		/* number of taken branches */
  int isq_index;		/* index of ISQ entry */
  int max_branches;		/* max number of branches? */
  enum md_opcode op;		/* op, used for pre-decode */
  enum fetch_stall_t fetch_stall_type;  /* fetch stall type */

  /* initialize variables */
  branch_cnt = 0;
  max_branches = FALSE;

  /* check to see if we are blocked by outstanding memory accesses */
  if (icache_block) {
    stat_add_sample(fetch_stall, (int)fetch_cache_miss); 
    return;
  }
  if (icache_delay) {
    icache_delay--;
    stat_add_sample(fetch_stall, (int)fetch_cache_miss); 
    return;
  }

  for (i=0;
    /* decode width exceeded? */
    i < (decode_width * fetch_speed) &&
    /* IFQ full? */
    IFQ_num < IFQ_size &&
    /* unknown target encountered? */
    !unknown_target &&
    /* trap waiting to commit? */
    !is_trap &&
    /* syscall waiting to commit? */
    !is_syscall &&
    /* max number of branches? */
    !max_branches;
    i++)
  {

    /* fetch an instruction at the next predicted fetch address */
    fetch_PC = fetch_pred_NPC;

    /* Is this a bogus text address? */
    if (1 ||(ld_text_base <= fetch_PC
      && fetch_PC < (ld_text_base+ld_text_size)
      && !(fetch_PC & (sizeof(md_inst_t)-1))))
    {
      /* check to see if the address is on the same cache line */
      if ((i != 0) && 
	  !fetch_mult_lines && 
	  !IS_CACHE_FAST_HIT(cache_il1, IACOMPRESS(fetch_PC))
      ) {
        stat_add_sample(fetch_stall, (int)fetch_align); 
	return;
      }

      /* read instruction from memory */
      MD_FETCH_INST(inst, mem, fetch_PC);
      
      /* access the TLB */
      if (itlb) {
        status = mase_cache_access(itlb, Read, 
          IACOMPRESS(fetch_PC), NULL, ISCOMPRESS(sizeof(md_inst_t)), 
          sim_cycle, NULL, NULL, fetch_cb, 0, &lat);
        miss = (lat > 1) || (status != MEM_KNOWN);
        if ((lat > 1) || (status == MEM_UNKNOWN))
          last_inst_tmissed = TRUE;
      }
      else {
        status = MEM_KNOWN;
        miss = FALSE;
      }

      /* If there was a miss, return setting the issue delay if known, or the
       * issue block if unknown.  For retry, exit the function without setting
       * anything to allow for another attempt. */
      if (miss) {
        if (status == MEM_KNOWN) {
          icache_block = FALSE;
          icache_delay = lat - 1;
          stat_add_sample(fetch_stall, (int)fetch_cache_miss); 
          return;
        }
        else if (status == MEM_UNKNOWN) {
          icache_block = TRUE;
          stat_add_sample(fetch_stall, (int)fetch_cache_miss); 
          return;
        }
        else if (status == MEM_RETRY) {
          icache_block = FALSE;
          stat_add_sample(fetch_stall, (int)fetch_cache_miss); 
          return;
        }
      }

      /* hit the TLB, try accessing the cache */
      if (cache_il1) {
        status = mase_cache_access(cache_il1, Read, 
          IACOMPRESS(fetch_PC), NULL, ISCOMPRESS(sizeof(md_inst_t)),
          sim_cycle, NULL, NULL, fetch_cb, 0, &lat);
        miss = (lat > cache_il1_lat) || (status != MEM_KNOWN);
        if ((lat > cache_il1_lat) || (status == MEM_UNKNOWN))
          last_inst_missed = TRUE;
      }
      else {
        status = MEM_KNOWN;
        lat = cache_il1_lat;
        miss = FALSE;
      }

      /* If there was a miss, return setting the issue delay if known, or the
       * issue block if unknown.  For retry, exit the function without setting
       * anything to allow for another attempt. */
      if (miss) {
        if (status == MEM_KNOWN) {
          icache_block = FALSE;
          icache_delay = lat - 1;
          stat_add_sample(fetch_stall, (int)fetch_cache_miss); 
          return;
        }
        else if (status == MEM_UNKNOWN) {
          icache_block = TRUE;
          stat_add_sample(fetch_stall, (int)fetch_cache_miss); 
          return;
        }
        else if (status == MEM_RETRY) {
          icache_block = FALSE;
          stat_add_sample(fetch_stall, (int)fetch_cache_miss); 
          return;
        }
      }

      /* At this point, both the TLB and I-cache have hit so the oracle can
       * execute the instruction.  To improve the performance of the
       * simulator, some decode information is determined here and passed
       * onto the dispatch stage in the fetch data record. */
      isq_index = checker_exec(fetch_PC, &(IFQ[IFQ_tail]));
    }
    else {
      /* fetch PC is bogus, send an invalid instruction (NOP) down the pipeline */
      inst = MD_NOP_INST;
      isq_index = INVALID_ISQ;
      IFQ[IFQ_tail].in1 = DNA;
      IFQ[IFQ_tail].in2 = DNA;
      IFQ[IFQ_tail].in3 = DNA;
      IFQ[IFQ_tail].out1 = DNA;
      IFQ[IFQ_tail].out2 = DNA;
      IFQ[IFQ_tail].target_addr = fetch_PC + sizeof(md_inst_t);
      printf("BOGUS!!!");
    }

    /* pre-decode instruction */
    MD_SET_OPCODE(op, inst);

    /* determine the next value for fetch_pred_NPC */
    if (pred_perfect) {
      /* grab the actual NPC from the oracle */
      fetch_pred_NPC = pu_regs.regs_NPC;
      if (MD_OP_FLAGS(op) & F_CTRL) {
        if (pu_regs.regs_NPC != pu_regs.regs_PC + sizeof(md_inst_t)) branch_cnt++;
        if (branch_cnt >= fetch_speed) max_branches = TRUE;
      }
    }
    else if (!pred) {
      /* no predictor - default to predict not taken */
      fetch_pred_NPC = fetch_PC + sizeof(md_inst_t);
    }
    else  {
      /* use the BTB target if possible (assumes pre-decode bits) */
      if (MD_OP_FLAGS(op) & F_CTRL)

        /* Get the next predicted fetch address from the branch predictor.
         * NOTE: Returned value may be 1 if bpred can only predict taken
	 * without a proper target. */
        fetch_pred_NPC =
          bpred_lookup(pred,
                       /* branch address */ fetch_PC,
                       /* target address */ IFQ[IFQ_tail].target_addr,
                       /* opcode */         op,
                       /* call? */          MD_IS_CALL(op),
                       /* return? */        MD_IS_RETURN(op),
                       /* updt */           &(IFQ[IFQ_tail].dir_update),
                       /* RSB index */      &stack_recover_idx
          );
      else
        fetch_pred_NPC = 0;

      /* valid address returned from branch predictor? */
      if (!fetch_pred_NPC) {
        /* no predicted taken target, attempt not taken target */
        fetch_pred_NPC = fetch_PC + sizeof(md_inst_t);
      }
      else {
        /* Go with target unless it is 1.  If the target is 1, it
	 * means the branch predictor predicts a taken branch but
	 * cannot determine the target (such as an unencountered
         * unconditional indirect jump).  In this case, the fetch
         * stage is halted until the address is known. */
        if (fetch_pred_NPC == 1) unknown_target = TRUE;

        /* check max num of branches per fetch */
        branch_cnt++;
        if (branch_cnt >= fetch_speed) max_branches = TRUE;
      }
    }

    /* commit this instruction to the IFQ */
    IFQ[IFQ_tail].isq_index = isq_index;
    IFQ[IFQ_tail].IR = inst;
    IFQ[IFQ_tail].PC = fetch_PC;
    IFQ[IFQ_tail].pred_NPC = fetch_pred_NPC;
    IFQ[IFQ_tail].stack_recover_idx = stack_recover_idx;
    IFQ[IFQ_tail].ptrace_seq = ptrace_seq++;
    IFQ[IFQ_tail].timestamp = sim_cycle;

    /* update pipetrace */
    ptrace_newinst(IFQ[IFQ_tail].ptrace_seq,
	           inst, IFQ[IFQ_tail].PC,
	           0);
    ptrace_newstage(IFQ[IFQ_tail].ptrace_seq,
	            PST_IFETCH,
	            ((last_inst_missed ? PEV_CACHEMISS : 0)
	            | (last_inst_tmissed ? PEV_TLBMISS : 0)));
    last_inst_missed = FALSE;
    last_inst_tmissed = FALSE;

    /* allocate an additional ptrace_seq for internal ld/st uop */
    if (MD_OP_FLAGS(op) & F_MEM) ptrace_seq++;

    /* adjust IFQ */
    IFQ_tail = (IFQ_tail + 1) % IFQ_size;
    IFQ_num++;
  }

  /* update fetch stall array */
  if (i >= (decode_width * fetch_speed))
    fetch_stall_type = fetch_no_stall;
  else if (is_syscall || is_trap)
    fetch_stall_type = fetch_syscall;
  else if (unknown_target)
    fetch_stall_type = fetch_unknown;
  else if (max_branches)
    fetch_stall_type = fetch_branches;
  else if (IFQ_num >= IFQ_size)
    fetch_stall_type = fetch_ifq_full;
  else
    fetch_stall_type = fetch_other;

  stat_add_sample(fetch_stall, (int)fetch_stall_type); 
}

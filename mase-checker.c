/* mase-checker.c - sim-mase dynamic checker and oracle */

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
#include "mase-macros-oracle.h"

/* ISQ - instruction state queue
 * Stores oracle data and output register information. */
struct inst_state *ISQ;			/* queue of instruction state data (oracle) */
int ISQ_head;				/* head of ISQ */
int ISQ_tail;				/* tail of ISQ */
int ISQ_size;				/* size of ISQ */
int ISQ_num;				/* number of valid ISQ entries */

/* pre-update register file */
struct regs_t pu_regs;

/* architectural register file (updated at commit) */
struct regs_t regs;

/* architectural memory (updated at commit) */
struct mem_t *mem;

/* set if trap is encountered */
int is_trap;

/* set if syscall is encountered */
int is_syscall;

/* set if the checker is in spec mode */
int chk_spec_mode;

/* pre-update memory hash table */
#define STORE_HASH_SIZE		32
#define HASH_ADDR(ADDR)                                                 \
  ((((ADDR) >> 24)^((ADDR) >> 16)^((ADDR) >> 8)^(ADDR)) & (STORE_HASH_SIZE-1))
static struct pu_hash_ent *pu_htable[STORE_HASH_SIZE];

/* hash table bucket free list for pre-memory hash table */
static struct pu_hash_ent *pu_bucket_free_list = NULL; 

/* Reads or writes from pre_update memory.  For loads, if a memory location is not
 * in the pre-update memory, the architectural memory will supply the value. */ 
enum md_fault_type
pu_mem_access(struct mem_t *mem,        /* memory space to access */
              enum mem_cmd cmd,         /* Read or Write */
              md_addr_t addr,           /* target address to access */
              void *vp,                 /* host memory address to access */
              int nbytes                /* number of bytes to access */)
{
  enum md_fault_type fault;		/* fault */
  int index;				/* hash table index */
  md_addr_t tag;			/* hash table tag */
  int found_match = 0;			/* set if match is found */
  byte_t *dp;				/* data pointer */
  struct pu_hash_ent *ent, *new_ent;    /* linked list traversal pointers */

  /* Check for faults. */
  fault = mem_check_fault(mem, cmd, addr, nbytes);
  if (fault != md_fault_none) return fault;

  index = HASH_ADDR(addr & ~7);
  tag = addr & ~7;

  /* Is it a load? */
  if (cmd == Read) {

    /* Look for match in pre-update hash table. */
    for (ent = pu_htable[index]; ent; ent = ent->next) {
      if (ent->line_tag == tag) {
	found_match = 1;
	break;
      }
    }

    if (found_match) {
      /* Use pre-update data if there is a match */
      dp = (byte_t *)(&ent->data) + (addr - tag);
      memcpy(vp, dp, nbytes);
    }
    else {
      /* Use arch mem otherwise. */
      mem_access(mem, Read, addr, vp, nbytes);
    }
  }
  else {  /* cmd == Write */

    /* Search list for matching line tag. */
    for (ent = pu_htable[index]; ent; ent = ent->next) {
      if (ent->line_tag == tag) {
	found_match = 1;
	break;
      }
    }
      
    /* If the entry has the same isq_index, we use the same entry
     * for the store.  NOTE: Other parts of MASE do not support
     * multiple stores on the same instruction, hence a warning. */
    if (found_match && (ent->isq_index == ISQ_tail)) {
      warn("Mutiple stores for the same instruction.");
      ent = new_ent;
    }
    else {

      /* get a pre-update hash table entry */
      if (!pu_bucket_free_list) {
        pu_bucket_free_list = calloc(1, sizeof(struct pu_hash_ent));
        if (!pu_bucket_free_list) fatal("out of virtual memory");
      }
      new_ent = pu_bucket_free_list;
      pu_bucket_free_list = pu_bucket_free_list->next; 

      /* update fields and insert into hash table */
      if (found_match)
        new_ent->data = ent->data;
      else 
	mem_access(mem, Read, tag, &(new_ent->data), 8);
      new_ent->isq_index = ISQ_tail;
      new_ent->line_tag = tag;
      new_ent->next = pu_htable[index];
      pu_htable[index] = new_ent;
    }
      
    /* store the memory value */
    dp = (byte_t *)&new_ent->data + (addr - tag);
    memcpy(dp, vp, nbytes);

    /* update the ISQ entry */
    ISQ[ISQ_tail].is_write = 1;
  }
  return md_fault_none;
}

/* release line in hash table - used during committing and squashing */
static void
release_htable_line(int isq_index)
{
  md_addr_t addr;
  int index;
  md_addr_t tag;
  struct pu_hash_ent *prev, *ent;

  addr = ISQ[isq_index].mem_addr;
  index = HASH_ADDR(addr & ~7);
  tag = addr & ~7;

  prev = NULL;
  ent = pu_htable[index];
  while (ent) {
    if (ent->isq_index == isq_index) { 
      if (ent->line_tag != tag) fatal("tag mismatch in release_htable_line");

      if (prev != NULL)
	prev->next = ent->next;
      else
	pu_htable[index] = ent->next;
	  
      ent->next = pu_bucket_free_list;
      pu_bucket_free_list = ent;  

      if (prev != NULL)
        ent = prev->next;
      else
	ent = pu_htable[index];
    }
    else {
      prev = ent;
      ent = ent->next;
    }
  }
}

/* Saves register writing information in the next available ISQ entry
 * This is called when the oracle is being executed during the fetch
 * stage.  It is called for each output dependency associated with
 * the instruction. */
static void
save_reg_state(struct reg_state_t *reg_state, int reg_name)
{
  /* Get the dependence name and type of the register.  The variable
   * reg_name embeds the type and name of the register.  The following
   * macros extract the name and type from the name.  reg_name cannot
   * be used directly since a register may hold two different types
   * and will have two different names.  The dependence name will be
   * the same in this situation since the dependencies need to be
   * enforced regardless of type. */
  int dep_name = DEP_NAME(reg_name);
  enum val_type type = GET_TYPE(reg_name);

  /* store the info into the reg_state structure */
  reg_state->dep_name = dep_name;
  reg_state->reg_type = type;

  if (dep_name == DNA) return;

  /* transfer the value into the reg_state structure */
#if defined(TARGET_PISA)
  if (REG_IS_FP(dep_name)) {
    switch (type) {
    case vt_sfloat:
      reg_state->reg_value.f = FPR_F(FP_REG_INDEX(dep_name));
      break;
    case vt_dfloat:
      reg_state->reg_value.d = FPR_D(FP_REG_INDEX(dep_name));
      break;
    default:
      fatal("Invalid type");
    }  
  }
  else if (dep_name == DFCC) {
    reg_state->reg_value.sw = FCC;
  }
  else if (dep_name == DHI) {
    reg_state->reg_value.sw = HI;
  }
  else if (dep_name == DLO) {
    reg_state->reg_value.sw = LO;
  }
  else {
    reg_state->reg_value.sw = GPR(INT_REG_INDEX(dep_name));
  }
#elif defined(TARGET_ALPHA)
  if (REG_IS_FP(dep_name)) {
    reg_state->reg_value.d = FPR(FP_REG_INDEX(dep_name));
  }
  else if (dep_name == DFPCR) {
    reg_state->reg_value.q = FPCR;
  }
  else if (dep_name == DUNIQ) {
    reg_state->reg_value.q = UNIQ;
  }
  else {
    reg_state->reg_value.q = GPR(INT_REG_INDEX(dep_name));
  }
#endif
}

/* Commits register writing information to architectural state.  This
 * is called when the checker commits an instruction and is called for
 * each output dependency associated with the instruction.
 */
static void
commit_reg_state(struct reg_state_t *reg_state)
{
  /* extract info from reg_state structure */
  int dep_name = reg_state->dep_name;
  enum val_type reg_type = reg_state->reg_type;

  if (dep_name == DNA) return;

  /* commit the value to the register */
#if defined(TARGET_PISA)
  if (REG_IS_FP(dep_name)) {
    switch (reg_type) {
    case vt_sfloat:
      regs.regs_F.f[FP_REG_INDEX(dep_name)] = reg_state->reg_value.f;
      break;
    case vt_dfloat:
      regs.regs_F.d[FP_REG_INDEX(dep_name) >> 1] = reg_state->reg_value.d;
      break;
    default:
      fatal("Invalid type");
    }
  }
  else if (dep_name == DFCC) {
    regs.regs_C.fcc = reg_state->reg_value.sw;
  }
  else if (dep_name == DHI) {
    regs.regs_C.hi = reg_state->reg_value.sw;
  }
  else if (dep_name == DLO) {
    regs.regs_C.lo = reg_state->reg_value.sw;
  }
  else {
    regs.regs_R[INT_REG_INDEX(dep_name)] = reg_state->reg_value.sw;
  }
#elif defined(TARGET_ALPHA)
  if (REG_IS_FP(dep_name)) {
    regs.regs_F.d[FP_REG_INDEX(dep_name)] = reg_state->reg_value.d;
  }
  else if (dep_name == DFPCR) {
    regs.regs_C.fpcr = reg_state->reg_value.q;
  }
  else if (dep_name == DUNIQ) {
    regs.regs_C.uniq = reg_state->reg_value.q;
  }
  else {
    regs.regs_R[INT_REG_INDEX(dep_name)] = reg_state->reg_value.q;
  }
#endif

  /* clear the entry */
  reg_state->dep_name = DNA;
}

/* removes and frees all of the ISQ state after the rollback point */
void
isq_reset(int rollback_index)
{
  int isq_index;		/* ISQ traversal index */
  int removed = 0;			

  /* Eliminate pre-update state entries after the rollback point.  In addition,
   * remove all of the corresponing memory entries from the pre-update hash
   * table. */
  for (isq_index = (rollback_index + 1) % ISQ_size;
       isq_index != ISQ_tail;
       isq_index = (isq_index + 1) % ISQ_size
  ) {
    ISQ[isq_index].valid = 0;
    if (ISQ[isq_index].is_trap) is_trap = 0;
    if (ISQ[isq_index].is_syscall) is_syscall = 0;
    if (ISQ[isq_index].is_write)
      release_htable_line(isq_index);
    removed++;
  }

  /* set the tail to the entry after the rollback instruction */
  ISQ_tail = (rollback_index + 1) % ISQ_size;
  ISQ_num = ISQ_num - removed;
}

/* print an ISQ memory value */
static void
isq_print_mem_value(FILE *stream, int size, union val_union u)
{
  switch (size) {
    case 1: myfprintf(stream, "0x%p", u.b); return;
    case 2: myfprintf(stream, "0x%p", u.h); return;
    case 4: myfprintf(stream, "0x%p", u.w); return;
    case 8: myfprintf(stream, "0x%p", u.q); return;
    default: fatal("invlaid size in isq_print_mem_value");
  }
}

/* print an ISQ register value */
static void
isq_print_reg_value(FILE *stream, struct reg_state_t *reg_state)
{
  switch (reg_state->reg_type) {
    case vt_none: myfprintf(stream, "none"); return;
    case vt_byte: myfprintf(stream, "0x%p", reg_state->reg_value.b); return;
    case vt_sbyte: myfprintf(stream, "0x%p", reg_state->reg_value.sb); return;
    case vt_half: myfprintf(stream, "0x%p", reg_state->reg_value.h); return;
    case vt_shalf: myfprintf(stream, "0x%p", reg_state->reg_value.sh); return;
    case vt_word: myfprintf(stream, "0x%p", reg_state->reg_value.w); return;
    case vt_sword: myfprintf(stream, "0x%p", reg_state->reg_value.sw); return;
    case vt_qword: myfprintf(stream, "0x%p", reg_state->reg_value.q); return;
    case vt_sqword: myfprintf(stream, "0x%p", reg_state->reg_value.sq); return;
    case vt_sfloat: myfprintf(stream, "%f", reg_state->reg_value.f); return;
    case vt_dfloat: myfprintf(stream, "%f", reg_state->reg_value.d); return;
    case vt_addr: myfprintf(stream, "0x%p", reg_state->reg_value.addr); return;
    default: fatal("invlaid size in isq_print_reg_value");
  }
}

/* dump contents of the ISQ */
void
isq_dump(FILE *stream)
{
  int num, head, i;
  fprintf(stream, "** ISQ contents **\n");
  fprintf(stream, "ISQ_num: %d\n", ISQ_num);
  fprintf(stream, "ISQ_head: %d, ISQ_tail: %d\n", ISQ_head, ISQ_tail);

  num = ISQ_num;
  head = ISQ_head;
  while (num) {
    fprintf(stream, "idx: %2d inst: `", head);
    md_print_insn(ISQ[head].IR, ISQ[head].PC, stream);
    fprintf(stream, "'\n");

    myfprintf(stream, "\tPC: 0x%08p, NPC: 0x%08p\n", ISQ[head].PC, ISQ[head].NPC);
    fprintf(stream, "\ttrap: %d, syscall: %d, spec_mode: %d, fault: %d\n",
      ISQ[head].is_trap, ISQ[head].is_syscall, ISQ[head].spec_mode, ISQ[head].fault);

    if (MD_OP_FLAGS(ISQ[head].op) & F_MEM) {
      myfprintf(stream, "\tmem_addr: 0x%08p, mem_size: %d\n",
      ISQ[head].mem_addr, ISQ[head].mem_size);
      myfprintf(stream, "\tmem_value: ");
      isq_print_mem_value(stream, ISQ[head].mem_size, ISQ[head].mem_value);
      fprintf(stream, ", is_write: %d\n", ISQ[head].is_write);
    }

    for (i = 0; i < MAX_ODEPS; i++) {
      fprintf(stream, "\treg_state %d -- ", i);
      if (ISQ[head].reg_state[i].dep_name == DNA) {
        fprintf(stream, "reg_dep_name: DNA\n");
      }
      else {
        fprintf(stream, "reg_dep_name: %d, reg_value: ", ISQ[head].reg_state[i].dep_name);
        isq_print_reg_value(stream, &(ISQ[head].reg_state[i]));
        fprintf(stream, "\n");
      }
    }

    head = (head + 1) % ISQ_size;
    num--;
  }
}

/* initialize checker */
void
checker_init()
{
  ISQ_size = IFQ_size + ROB_size;
  ISQ = calloc(ISQ_size, sizeof(struct inst_state));
  if (!ISQ) fatal("out of virtual memory");

  ISQ_head = ISQ_tail = 0;
  ISQ_num = 0;
  ISQ_count = 0;
  ISQ_fcount = 0;

  is_trap = 0;
  is_syscall = 0;
}

/* Execute the instruction and create ISQ entry, returns index of ISQ entry. */
int
checker_exec(md_addr_t fetch_PC, struct fetch_rec *IFQ)
{
  const int fastfwd_mode = 0;		/* used in memory macros */
  md_inst_t inst;			/* actual instruction bits */
  enum md_opcode op;			/* decoded opcode enum */
  enum md_fault_type fault;		/* fault information */
  md_addr_t target_addr;		/* target address */
  md_addr_t mem_addr;			/* address of load/store */
  union val_union mem_value;		/* memory value */
  int mem_size;				/* size of memory value */
  int out1, out2;			/* output dependencies */
  int in1, in2, in3;			/* input dependencies */
  int isq_entry;			/* entry for the instruction state */
  int i;				/* loop iteration variable */

  /* check for illegal entry conditions */
  if (is_trap) fatal("Cannot call checker_exec until trap is committed.\n");
  if (is_syscall) fatal("Cannot call checker_exec until syscall is committed.\n");

  /* initialize variables */
  fault = md_fault_none;
  mem_addr = 0;
  mem_size = 0;

  /* Move the pu_regs.regs_NPC into pu_regs.regs_PC.  If it differs from the fetch PC, it
   * must be an instruction on a mis-speculated path, so turn on checker spec
   * mode. */
  pu_regs.regs_PC = pu_regs.regs_NPC;
  if (!chk_spec_mode) {
    chk_spec_mode = (fetch_PC != pu_regs.regs_PC);
  }

  /* In any case, use the fetch_PC since we must execute instructions on the 
   * mis-speculated path anyway. */
  pu_regs.regs_PC = fetch_PC;

  /* compute default next PC */
  pu_regs.regs_NPC = pu_regs.regs_PC + sizeof(md_inst_t);

  /* fetch and decode the instruction */
  MD_FETCH_INST(inst, mem, pu_regs.regs_PC);
  MD_SET_OPCODE(op, inst);

  /* Update stats.  The "total" stats are updated here while the "committed" stats
   * are updated in the commit stage.  */
  sim_total_insn++;
  if (MD_OP_FLAGS(op) & F_CTRL) sim_total_branches++;
  if (MD_OP_FLAGS(op) & F_MEM) {
    sim_total_refs++;
    if (!(MD_OP_FLAGS(op) & F_STORE)) {
      sim_total_loads++;
    }
  }
  if (!chk_spec_mode) sim_oracle_insn++;

  /* maintain $r0 semantics */
  pu_regs.regs_R[MD_REG_ZERO] = 0;
#if defined(TARGET_ALPHA)
  pu_regs.regs_F.d[MD_REG_ZERO] = 0.0;
#endif

  /* assume instruction is not a store */
  ISQ[ISQ_tail].is_write = 0;

  /* Execution of syscalls are delayed until the instruction commits.  As a result,
   * instructions are prevented from entering the oracle (or the fetch stage)
   * until the syscall has commited. */
#define DECLARE_FAULT(FAULT) { fault = (FAULT); break; }
#define SYSCALL(INST) is_syscall = 1

  /* execute the instruction */
  switch (op) {
#define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,I1,I2,I3)  \
  case OP:                                                    \
    out1 = O1; out2 = O2; in1 = I1; in2 = I2; in3 = I3;	      \
    SYMCAT(OP,_IMPL);                                         \
    break;
#define DEFLINK(OP,MSK,NAME,MASK,SHIFT)                       \
  case OP:                                                    \
    panic("attempted to execute a linking opcode");
#define CONNECT(OP)
#include "machine.def"
  default:
    return INVALID_ISQ;
  }

  /* print instruction trace */
  if (verbose && 
      (sim_oracle_insn >= trigger_inst)  &&
      (!max_insts || (sim_oracle_insn <= max_insts)))
  {
    if (chk_spec_mode)
      myfprintf(stderr, "   ORACLE: %10s [xor: 0x%08x] @ 0x%08p: ",
                "spec", md_xor_regs(&pu_regs), pu_regs.regs_PC);
    else
      myfprintf(stderr, "   ORACLE: %10n [xor: 0x%08x] @ 0x%08p: ",
                sim_oracle_insn, md_xor_regs(&pu_regs), pu_regs.regs_PC);
    md_print_insn(inst, pu_regs.regs_PC, stderr);
    fprintf(stderr, "\n");
    if (verbose_regs) {
      md_print_iregs(pu_regs.regs_R, stderr);
      md_print_fpregs(pu_regs.regs_F, stderr);
      md_print_cregs(pu_regs.regs_C, stderr);
      fprintf(stderr, "\n");
    }
  }

  /* check for traps */
  is_trap = (MD_OP_FLAGS(op) & F_TRAP);

  /* check for a full ISQ */
  if ((ISQ_head == ISQ_tail) && (ISQ[ISQ_head].valid))
    fatal("full ISQ");

  /* update the latest ISQ entry */
  ISQ[ISQ_tail].valid = 1;
  ISQ[ISQ_tail].IR = inst;
  ISQ[ISQ_tail].op = op;
  ISQ[ISQ_tail].PC = pu_regs.regs_PC;
  ISQ[ISQ_tail].NPC = pu_regs.regs_NPC;
  ISQ[ISQ_tail].is_trap = is_trap;
  ISQ[ISQ_tail].is_syscall = is_syscall;
  ISQ[ISQ_tail].spec_mode = chk_spec_mode;
  ISQ[ISQ_tail].fault = fault;
  ISQ[ISQ_tail].mem_addr = mem_addr;
  ISQ[ISQ_tail].mem_size = mem_size;
  ISQ[ISQ_tail].mem_value = mem_value;

  /* save register state */
  save_reg_state(&(ISQ[ISQ_tail].reg_state[0]), out1);
  save_reg_state(&(ISQ[ISQ_tail].reg_state[1]), out2);

  /* save decode information in the fetch data record */
  IFQ->in1 = in1;
  IFQ->in2 = in2;
  IFQ->in3 = in3;
  IFQ->out1 = out1;
  IFQ->out2 = out2;
  IFQ->target_addr = target_addr;
  IFQ->mem_size = mem_size;

  /* advance the ISQ tail */
  isq_entry = ISQ_tail;
  ISQ_tail = (ISQ_tail + 1) % ISQ_size;
  ISQ_num++;

  /* update any "spec" stats tracked by PC */
  for (i=0; i<spec_pcstat_nelt; i++) {

    counter_t newval;
    int delta;

    /* check if any tracked stats changed */
    newval = STATVAL(spec_pcstat_stats[i]);
    delta = newval - spec_pcstat_lastvals[i];
    if (delta != 0) {
      stat_add_samples(spec_pcstat_sdists[i], pu_regs.regs_PC, delta);
      spec_pcstat_lastvals[i] = newval;
    }
  }

  return isq_entry;
}

/* Recover the checker to the instruction indicated by the rollback index.
 * Pre-updated state is changed to reflect the state of the machine up to
 * (and including) the rollback instruction.
 * 
 * All ISQ entries after the rollback instruction are removed.  This function
 * is called when a branch misprediction occurs and can be called for other
 * mispredictions (such as address and value mispredictions). */
void
checker_recover(int rollback_index)
{
  int isq_index;		/* index for ISQ traversal */
  int i;			/* loop iteration variable */
  int found_match = 0;		/* set if a match is found */

  /* start with the non-speculative register file */
  pu_regs = regs;

  /* walk the list of pre-update state entries to the rollback point */
  for (isq_index = ISQ_head; !found_match; isq_index = (isq_index + 1) % ISQ_size) {

    if (isq_index == rollback_index) found_match = 1;
    if (ISQ[isq_index].valid == 0) continue;

    /* update the register file */
    for (i = 0; i < MAX_ODEPS; i++) {
      struct reg_state_t *reg_state = &(ISQ[isq_index].reg_state[i]);

      if (reg_state->dep_name == DNA) continue;

#if defined(TARGET_PISA)
      if (REG_IS_FP(reg_state->dep_name)) {
	switch (reg_state->reg_type) {
	case vt_sfloat:
          SET_FPR_F(FP_REG_INDEX(reg_state->dep_name), reg_state->reg_value.f);
	  break;
	case vt_dfloat:
          SET_FPR_D(FP_REG_INDEX(reg_state->dep_name), reg_state->reg_value.d);
	  break;
        default:
          fatal("Invalid type");
	}
      }
      else if (reg_state->dep_name == DFCC) {
        SET_FCC(reg_state->reg_value.sw);
      }
      else if (reg_state->dep_name == DHI) {
        SET_HI(reg_state->reg_value.sw);
      }
      else if (reg_state->dep_name == DLO) {
        SET_LO(reg_state->reg_value.sw);
      }
      else {
        SET_GPR(INT_REG_INDEX(reg_state->dep_name), reg_state->reg_value.sw);
      }
#elif defined(TARGET_ALPHA)
      if (REG_IS_FP(reg_state->dep_name)) {
        SET_FPR(FP_REG_INDEX(reg_state->dep_name), reg_state->reg_value.d);
      }
      else if (reg_state->dep_name == DFPCR) {
        SET_FPCR(reg_state->reg_value.q);
      }
      else if (reg_state->dep_name == DUNIQ) {
        SET_UNIQ(reg_state->reg_value.q);
      }
      else {
        SET_GPR(INT_REG_INDEX(reg_state->dep_name), reg_state->reg_value.q);
      }
#endif
    }
  }

  /* reset the ISQ  - this function will clean up pre-update memory */
  isq_reset(rollback_index);
    
  /* Set the spec mode bit equal to the spec mode bit of the rollback
   * instruction. If the spec_mode bit is set, there is an earlier
   * isntruction that has not yet executed (and subsequently not
   * recovered) that caused the entry into spec_mode. */
  chk_spec_mode = ISQ[rollback_index].spec_mode;
}

/* Process a timing dependent instruction. Writes the re value into the isq. */
static void
process_timing_dep(struct ROB_entry *re, struct inst_state *isq)
{
  int i, j;			    /* loop iteration variables */

  isq->NPC = re->NPC;
  isq->fault = re->fault;

  /* For stores, check the memory value. */
  if (re->in_LSQ && isq->is_write) {
    switch (re->mem_size) {
    case 1:
      isq->mem_value.b = re->mem_value.b;
      break;
    case 2:
      isq->mem_value.h = re->mem_value.h;
      break;
    case 4:
      isq->mem_value.w = re->mem_value.w;
      break;
    case 8:
      isq->mem_value.q = re->mem_value.q;
      break;
    default:
      fatal("invalid memory size in isq entry");
      break;
    }
  }

  /* Check register values */
  for (i = 0; i < MAX_ODEPS; i++) {

#if defined(TARGET_PISA)
    if ((re->odep_name[i] == DNA) || (re->odep_name[i] == DGPR(MD_REG_ZERO)))
       continue;
#elif defined(TARGET_ALPHA)
    if ((re->odep_name[i] == DNA) || (re->odep_name[i] == DGPR(MD_REG_ZERO)) ||
        (re->odep_name[i] == DFPR(MD_REG_ZERO))) continue;
#endif

    for (j = 0; j < MAX_ODEPS; j++) {
      if (re->ea_comp) {
        if (re->odep_name[i] == DTMP) {
	  isq->mem_addr = re->odep_value[i].addr;
        }
      }
      else if (re->odep_name[i] == isq->reg_state[j].dep_name) {
	switch (isq->reg_state[j].reg_type) {
	case vt_byte:
	  isq->reg_state[j].reg_value.b  = re->odep_value[i].b;
	  break;
	case vt_sbyte:
	  isq->reg_state[j].reg_value.sb  = re->odep_value[i].sb;
	  break;
	case vt_half:
	  isq->reg_state[j].reg_value.h  = re->odep_value[i].h;
	  break;
	case vt_shalf:
	  isq->reg_state[j].reg_value.sh  = re->odep_value[i].sh;
	  break;
	case vt_word:
	  isq->reg_state[j].reg_value.w  = re->odep_value[i].w;
	  break;
	case vt_sword:
	  isq->reg_state[j].reg_value.sw  = re->odep_value[i].sw;
	  break;
	case vt_qword:
	  isq->reg_state[j].reg_value.q  = re->odep_value[i].q;
	  break;
	case vt_sqword:
	  isq->reg_state[j].reg_value.sq  = re->odep_value[i].sq;
	  break;
	case vt_sfloat:
	  isq->reg_state[j].reg_value.f  = re->odep_value[i].f;
	  break;
	case vt_dfloat:
	  isq->reg_state[j].reg_value.d  = re->odep_value[i].d;
	  break;
	case vt_addr:
	  isq->reg_state[j].reg_value.addr  = re->odep_value[i].addr;
	  break;
	default:
	  fatal("Invalid type to check");
	}
      }
    }
  }
}

/* Prints all errors associated with an instruction. */
static void
checker_print_errors(struct ROB_entry *re, struct inst_state *isq)
{
  int i, j;			    /* loop iteration variables */

  /* verify NPC */
  if (re->NPC != isq->NPC) {
    myfprintf(stderr,
      "CHECKER ERROR! re NPC 0x%08p differs from oracle NPC 0x%08p (cycle %d)\n",
      re->NPC, isq->NPC, sim_cycle
    );
  }

  /* verify faults */
  if (re->fault != isq->fault) {
    myfprintf(stderr,
      "CHECKER ERROR! re fault %d differs from oracle fault %d at 0x%08p (cycle %d)\n",
      re->fault, isq->fault, isq->PC, sim_cycle
    );
  }

  /* For stores, check the memory value. */
  if (re->in_LSQ && isq->is_write) {
    switch (isq->mem_size) {
    case 1:
      if (re->mem_value.b != isq->mem_value.b) {
        myfprintf(stderr,
          "CHECKER ERROR! Memory mismatch at PC 0x%08p (1 byte) (cycle %d)\n",
          isq->PC, sim_cycle
        );
      }
      break;
    case 2:
      if (re->mem_value.h != isq->mem_value.h) {
        myfprintf(stderr,
          "CHECKER ERROR! Memory mismatch at PC 0x%08p (2 bytes) (cycle %d)\n",
          isq->PC, sim_cycle
        );
      } 
      break;
    case 4:
      if (re->mem_value.w != isq->mem_value.w) {
        myfprintf(stderr,
          "CHECKER ERROR! Memory mismatch at PC 0x%08p (4 bytes) (cycle %d)\n",
          isq->PC, sim_cycle
        );
      } 
      break;
    case 8:
      if (re->mem_value.q != isq->mem_value.q) {
        myfprintf(stderr,
          "CHECKER ERROR! Memory mismatch at PC 0x%08p (8 bytes) (cycle %d)\n",
          isq->PC, sim_cycle
        );
      } 
      break;
    default:
      fatal("invalid memory size in isq entry");
      break;
    }
  }

  /* Check register values */
  for (i = 0; i < MAX_ODEPS; i++) {

    int found_match = 0;

#if defined(TARGET_PISA)
    if ((re->odep_name[i] == DNA) || (re->odep_name[i] == DGPR(MD_REG_ZERO)))
       continue;
#elif defined(TARGET_ALPHA)
    if ((re->odep_name[i] == DNA) || (re->odep_name[i] == DGPR(MD_REG_ZERO)) ||
        (re->odep_name[i] == DFPR(MD_REG_ZERO))) continue;
#endif

    for (j = 0; j < MAX_ODEPS; j++) {
      if (re->ea_comp) {
        if (re->odep_name[i] == DTMP) {
	  found_match = 1;
          if (re->odep_value[i].addr != isq->mem_addr) {
            fprintf(stderr, "CHECKER ERROR! memory address differs at ");
            myfprintf(stderr,
              "PC 0x%08p, re: 0x%08p oracle: 0x%08p (cycle %d)\n",
              isq->PC, re->odep_value[i].addr,
	      isq->mem_addr, sim_cycle
            );
          }
        }
      }
      else if (re->odep_name[i] == isq->reg_state[j].dep_name) {
	found_match = 1;

	switch (isq->reg_state[j].reg_type) {
	case vt_byte:
	  if (re->odep_value[i].b != isq->reg_state[j].reg_value.b) {
            fprintf(stderr, "CHECKER ERROR! register %d differs at PC ", re->odep_name[i]);
            myfprintf(stderr, "0x%08p, re: 0x%08p oracle: 0x%08p (cycle %d)\n", 
		isq->PC, re->odep_value[i].b, isq->reg_state[j].reg_value.b, sim_cycle);
	  }
	  break;
	case vt_sbyte:
	  if (re->odep_value[i].sb != isq->reg_state[j].reg_value.sb) {
            fprintf(stderr, "CHECKER ERROR! register %d differs at PC ", re->odep_name[i]);
            myfprintf(stderr, "0x%08p, re: 0x%08p oracle: 0x%08p (cycle %d)\n", 
		isq->PC, re->odep_value[i].sb, isq->reg_state[j].reg_value.sb, sim_cycle);
	  }
	  break;
	case vt_half:
	  if (re->odep_value[i].h != isq->reg_state[j].reg_value.h) {
            fprintf(stderr, "CHECKER ERROR! register %d differs at PC ", re->odep_name[i]);
            myfprintf(stderr, "0x%08p, re: 0x%08p oracle: 0x%08p (cycle %d)\n", 
		isq->PC, re->odep_value[i].h, isq->reg_state[j].reg_value.h, sim_cycle);
	  }
	  break;
	case vt_shalf:
	  if (re->odep_value[i].sh != isq->reg_state[j].reg_value.sh) {
            fprintf(stderr, "CHECKER ERROR! register %d differs at PC ", re->odep_name[i]);
            myfprintf(stderr, "0x%08p, re: 0x%08p oracle: 0x%08p (cycle %d)\n", 
		isq->PC, re->odep_value[i].sh, isq->reg_state[j].reg_value.sh, sim_cycle);
	  }
	  break;
	case vt_word:
	  if (re->odep_value[i].w != isq->reg_state[j].reg_value.w) {
            fprintf(stderr, "CHECKER ERROR! register %d differs at PC ", re->odep_name[i]);
            myfprintf(stderr, "0x%08p, re: 0x%08p oracle: 0x%08p (cycle %d)\n", 
		isq->PC, re->odep_value[i].w, isq->reg_state[j].reg_value.w, sim_cycle);
	  }
	  break;
	case vt_sword:
	  if (re->odep_value[i].sw != isq->reg_state[j].reg_value.sw) {
            fprintf(stderr, "CHECKER ERROR! register %d differs at PC ", re->odep_name[i]);
            myfprintf(stderr, "0x%08p, re: 0x%08p oracle: 0x%08p (cycle %d)\n", 
		isq->PC, re->odep_value[i].sw, isq->reg_state[j].reg_value.sw, sim_cycle);
	  }
	  break;
	case vt_qword:
	  if (re->odep_value[i].q != isq->reg_state[j].reg_value.q) {
            fprintf(stderr, "CHECKER ERROR! register %d differs at PC ", re->odep_name[i]);
            myfprintf(stderr, "0x%08p, re: 0x%08p oracle: 0x%08p (cycle %d)\n", 
		isq->PC, re->odep_value[i].q, isq->reg_state[j].reg_value.q, sim_cycle);
	  }
	  break;
	case vt_sqword:
	  if (re->odep_value[i].sq != isq->reg_state[j].reg_value.sq) {
            fprintf(stderr, "CHECKER ERROR! register %d differs at PC ", re->odep_name[i]);
            myfprintf(stderr, "0x%08p, re: 0x%08p oracle: 0x%08p (cycle %d)\n", 
		isq->PC, re->odep_value[i].sq, isq->reg_state[j].reg_value.sq, sim_cycle);
	  }
	  break;
	case vt_sfloat:
	  if (re->odep_value[i].f != isq->reg_state[j].reg_value.f) {
            fprintf(stderr, "CHECKER ERROR! register %d differs at PC ", re->odep_name[i]);
            myfprintf(stderr, "0x%08p, re: 0x%f oracle: 0x%f (cycle %d)\n", 
		isq->PC, re->odep_value[i].f, isq->reg_state[j].reg_value.f, sim_cycle);
	  }
	  break;
	case vt_dfloat:
	  if (re->odep_value[i].d != isq->reg_state[j].reg_value.d) {
            fprintf(stderr, "CHECKER ERROR! register %d differs at PC ", re->odep_name[i]);
            myfprintf(stderr, "0x%08p, re: 0x%f oracle: 0x%f (cycle %d)\n", 
		isq->PC, re->odep_value[i].d, isq->reg_state[j].reg_value.d, sim_cycle);
	  }
	  break;
	case vt_addr:
	  if (re->odep_value[i].addr != isq->reg_state[j].reg_value.addr) {
            fprintf(stderr, "CHECKER ERROR! register %d differs at PC ", re->odep_name[i]);
            myfprintf(stderr, "0x%08p, re: 0x%08p oracle: 0x%08p (cycle %d)\n", 
		isq->PC, re->odep_value[i].addr, isq->reg_state[j].reg_value.addr, sim_cycle);
	  }
	  break;
	default:
	  fatal("Invalid type to check");
	}
      }
    }

    /* check to see that the register was checked at least once */
    if (!found_match) {
      fprintf(stderr, "CHECKER ERROR! oracle does not write register %d at ", re->odep_name[i]);
      myfprintf(stderr, "PC 0x%08p (cycle %d)\n", isq->PC, sim_cycle);
    }
  }

  fprintf(stderr, "Failing instruction: ");
  md_print_insn(re->IR, re->PC, stderr);
  fprintf(stderr, "\n");
}

/* Check to see if the core results match the oracle results.  
 * Returns 1 if an error was found, 0 if there was no error.  */
int
checker_check(struct ROB_entry *re, struct inst_state *isq)
{
  int found_error = 0;              /* set if an error is detected */
  int i, j;			    /* loop iteration variables */

  /* check for fatal errors */
  if (re->isq_index != ISQ_head) fatal("ISQ and ROB out of sync");
  if (re->PC != isq->PC) fatal("ISQ and ROB out of sync"); 
//  if (isq->spec_mode) found_error = 1;
//  isq->spec_mode = 0;
  if (isq->spec_mode) fatal("trying to commit in spec mode");


  /* verify NPC */
  if (re->NPC != isq->NPC) found_error = 1;

  /* verify faults */
  if (re->fault != isq->fault) found_error = 1;

  /* For stores, check the memory value. */
  if (re->in_LSQ && isq->is_write) {
    switch (isq->mem_size) {
    case 1:
      if (re->mem_value.b != isq->mem_value.b) found_error = 1;
      break;
    case 2:
      if (re->mem_value.h != isq->mem_value.h) found_error = 1;
      break;
    case 4:
      if (re->mem_value.w != isq->mem_value.w) found_error = 1;
      break;
    case 8:
      if (re->mem_value.q != isq->mem_value.q) found_error = 1;
      break;
    default:
      fatal("invalid memory size in isq entry");
      break;
    }
  }

  /* Check register values */
  for (i = 0; i < MAX_ODEPS; i++) {

    int found_match = 0;

#if defined(TARGET_PISA)
    if ((re->odep_name[i] == DNA) || (re->odep_name[i] == DGPR(MD_REG_ZERO)))
       continue;
#elif defined(TARGET_ALPHA)
    if ((re->odep_name[i] == DNA) || (re->odep_name[i] == DGPR(MD_REG_ZERO)) ||
        (re->odep_name[i] == DFPR(MD_REG_ZERO))) continue;
#endif

    for (j = 0; j < MAX_ODEPS; j++) {
      if (re->ea_comp) {
        if (re->odep_name[i] == DTMP) {
	  found_match = 1;
          if (re->odep_value[i].addr != isq->mem_addr)
            found_error = 1;
        }
      }
      else if (re->odep_name[i] == isq->reg_state[j].dep_name) {
	found_match = 1;

	switch (isq->reg_state[j].reg_type) {
	case vt_byte:
	  if (re->odep_value[i].b != isq->reg_state[j].reg_value.b)
	    found_error = 1;
	  break;
	case vt_sbyte:
	  if (re->odep_value[i].sb != isq->reg_state[j].reg_value.sb)
	    found_error = 1;
	  break;
	case vt_half:
	  if (re->odep_value[i].h != isq->reg_state[j].reg_value.h)
	    found_error = 1;
	  break;
	case vt_shalf:
	  if (re->odep_value[i].sh != isq->reg_state[j].reg_value.sh)
	    found_error = 1;
	  break;
	case vt_word:
	  if (re->odep_value[i].w != isq->reg_state[j].reg_value.w)
	    found_error = 1;
	  break;
	case vt_sword:
	  if (re->odep_value[i].sw != isq->reg_state[j].reg_value.sw)
	    found_error = 1;
	  break;
	case vt_qword:
	  if (re->odep_value[i].q != isq->reg_state[j].reg_value.q)
	    found_error = 1;
	  break;
	case vt_sqword:
	  if (re->odep_value[i].sq != isq->reg_state[j].reg_value.sq)
	    found_error = 1;
	  break;
	case vt_sfloat:
	  if (re->odep_value[i].f != isq->reg_state[j].reg_value.f)
	    found_error = 1;
	  break;
	case vt_dfloat:
	  if (re->odep_value[i].d != isq->reg_state[j].reg_value.d)
	    found_error = 1;
	  break;
	case vt_addr:
	  if (re->odep_value[i].addr != isq->reg_state[j].reg_value.addr)
	    found_error = 1;
	  break;
	default:
	  fatal("Invalid type to check");
	}
      }
    }

    /* check to see that the register was checked at least once */
    if (!found_match) found_error = 1;
  }

  /* check for timing dependent instructions and if errors should be printed */
  if (re->timing_dep && found_error) process_timing_dep(re,isq);
  else if (print_chk_err && found_error) checker_print_errors(re, isq);
  return found_error;
}

/* First, checker_check is called to see if the core results match the
 * oracle results.  In either case, the instruction is commited using
 * the oracle data.  If a mismatched occurs, the pipeline is flushed
 * and restarted with the subsequent instruction.  Returns 1 if an
 * error was found, 0 if there was no error.  If this is an effective
 * address instruction, the LSQ entry must have been verified earlier
 * using checker_commit.  The result of this check is passed in via
 * lsq_check_error. */
int
checker_commit(struct ROB_entry *re, int lsq_chk_error)
{
  int found_error;                  /* set if an error is detected */
  int i;			    /* loop iteration variable */

  /* check the result of the instruction */
  found_error = checker_check(re, &(ISQ[ISQ_head]));
  if (lsq_chk_error) found_error = 1;

  /* check for non-speculative faults */
  if (ISQ[ISQ_head].fault != md_fault_none)
    fatal("non-spec fault (%d) detected @ 0x%08p", ISQ[ISQ_head].fault, ISQ[ISQ_head].PC);

  /* update stats */
  sim_num_insn++;
  if (MD_OP_FLAGS(ISQ[ISQ_head].op) & F_CTRL) sim_num_branches++;
  if (MD_OP_FLAGS(ISQ[ISQ_head].op) & F_MEM) {
    sim_num_refs++;
    if (!(ISQ[ISQ_head].is_write)) {
      sim_num_loads++;
    }
  }
  if (re->timing_dep) timing_dep_insn++;

  /* update any stats tracked by PC */
  for (i=0; i<pcstat_nelt; i++) {

    counter_t newval;
    int delta;

    /* check if any tracked stats changed */
    newval = STATVAL(pcstat_stats[i]);
    delta = newval - pcstat_lastvals[i];
    if (delta != 0) {
      stat_add_samples(pcstat_sdists[i], pu_regs.regs_PC, delta);
      pcstat_lastvals[i] = newval;
    }
  }

  /* Update the architectural values of PC and NPC.  This needs to be done
   * before processing a syscall. */
  regs.regs_PC = ISQ[ISQ_head].PC;
  regs.regs_NPC = ISQ[ISQ_head].NPC;

  /* update state using the checker data regardless of correctness */
  if (ISQ[ISQ_head].is_syscall) {

    /* syscall: should be the only instruction in the ISQ as both
     * register files need updating */
    if (ISQ_tail != ((ISQ_head + 1) % ISQ_size))
      fatal("syscall is not the only instruction in ISQ");
    sys_syscall(&regs, mem_access, mem, re->IR, TRUE);
    pu_regs = regs;
    is_syscall = 0;
  }
  else if (ISQ[ISQ_head].is_write) {
    mem_access(mem, Write, ISQ[ISQ_head].mem_addr, &(ISQ[ISQ_head].mem_value), ISQ[ISQ_head].mem_size);
    release_htable_line(ISQ_head);
  }
  else {
    commit_reg_state(&(ISQ[ISQ_head].reg_state[0]));
    commit_reg_state(&(ISQ[ISQ_head].reg_state[1]));
  }

  /* clear trap flag, if appropriate */
  if (ISQ[ISQ_head].is_trap) is_trap = 0;

  if (found_error) {
    if (!re->timing_dep) checker_errors++;
    if (stop_on_chk_err) fatal("Aborting simulation due to checker error!");

    /* recover and restart with the next isntruction */
    checker_recover(ISQ_head);
    rob_recover(&(ROB[ROB_head]) - ROB);
    fe_recover(ISQ[ISQ_head].NPC);    
  }

  /* free the head entry */
  ISQ[ISQ_head].valid = 0;
  ISQ_head = (ISQ_head + 1) % ISQ_size;
  ISQ_num--;

  return found_error;
}

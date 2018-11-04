/* mase-exec.c - sim-mase exec stage */

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
#include "mase-macros-exec.h"

/* Reorder Buffer Unit (ROB) - Organized as a circular queue. */
struct ROB_entry *ROB;		/* Reorder buffer */
int ROB_head, ROB_tail;		/* ROB head and tail pointers */
int ROB_num;			/* num entries currently in ROB */

int RS_num; /* num reservation stations currently occupied */

/* Load/Store Queue (LSQ) - Holds loads and stores in program order. */
struct ROB_entry *LSQ;          /* Load Store Queue */
int LSQ_head, LSQ_tail;         /* LSQ head and tail pointers */
int LSQ_num;                    /* num entries currently in LSQ */

/* RE link free list - grab RE_LINKs from here when needed */
struct RE_link *relink_free_list;

/* request id - for memory accesses */
unsigned int req_id = 0;

/* Pending event queue, sorted from soonest to latest event (in time).
 * NOTE: RE_LINK nodes are used so that it need not be updated during
 * squash events. */
static struct RE_link *event_queue;

/* latency misprediction queue */
static struct RE_link *mp_queue;

/* The ready instruction queue indicates which instruction have all of their
 * register and memory dependencies satisfied.  Instruction will issue when
 * resources are available.
 *
 * The ready queue is fully constructed each cycle before any operation is
 * issued from it -- this ensures that instruction issue priorities are
 * properly observed. NOTE: RE_LINK nodes are used for the so that it need
 * not be updated during squash events. */
static struct RE_link *ready_queue;

/* The mem_req_table stores pending memory requests that require callback.*/
struct mem_req {
  unsigned int rid;		/* request id */
  INST_TAG_TYPE tag;		/* tag (used to see if entry got squashed) */
  struct ROB_entry *re;	/* pointer to LSQ entry; NULL if unused */
};
struct mem_req *mem_req_table;
#define MEMREQ_VALID(ENTRY) ((ENTRY.re != NULL) && (ENTRY.tag == ENTRY.re->tag))

/* The sinfo_table is used to determine if loads are ready. */
struct store_info {
  md_addr_t addr;	/* address of store */
  int index;		/* LSQ index of store */
  int sta_unknown;	/* set if sta is unknown */
  int std_unknown;	/* set if std is unknown */
};
struct store_info *sinfo_table;
 
/* callback function */
void
load_cb(unsigned int rid, unsigned int lat)
{
  int i;
  
  /* scan the mem req table for the proper entry */
  for (i = 0; i <= LSQ_size; i++) {
    if (mem_req_table[i].rid == rid) break;
  }
  if (i == LSQ_size) fatal("cannot find entry in mem req table");
  if (mem_req_table[i].re == NULL) fatal("no RE entry for memory req");
  if (!(MEMREQ_VALID(mem_req_table[i]))) return;

  /* queue the writeback event */
  eventq_queue_event(mem_req_table[i].re, sim_cycle + lat);

  /* free the entry */
  mem_req_table[i].re = NULL;
}

/* micro-functional execution functions */

/* Read an value from the idep list that correspons to reg_name. */
union val_union
read_idep_list(struct ROB_entry *re, int reg_name)
{
  int i;		/* loop traversal variable */
  union val_union zero;

  zero.q = 0;

  for (i = 0; i < MAX_IDEPS; i++) {
    if (reg_name == re->idep_name[i]) {
      if (!(re->idep_ready[i])) fatal("Trying to read input that is not ready");
      return (re->idep_value[i]);
    }
  }

  /* It is OK in these situations for the value not to be found. */
  if (re->ea_comp) return zero;
  if (re->in_LSQ)  return zero;
  if (re->op == MD_NOP_OP) return zero;

  fatal("Could not find int register in input list");
  return zero;
}
 
/* Set the value of an odep value entry. */
void
set_odep_list(struct ROB_entry *re, int reg_name, union val_union reg_value, enum val_type reg_type)
{
  int i;		/* loop traversal variable */

  for (i = 0; i < MAX_ODEPS; i++) {
    if (reg_name == re->odep_name[i]) {
      switch (reg_type) {
      case vt_byte:
        re->odep_value[i].b = reg_value.b;
	break;
      case vt_sbyte:
        re->odep_value[i].sb = reg_value.sb;
	break;
      case vt_half:
        re->odep_value[i].h = reg_value.h;
	break;
      case vt_shalf:
        re->odep_value[i].sh = reg_value.sh;
	break;
      case vt_word:
        re->odep_value[i].w = reg_value.w;
	break;
      case vt_sword:
        re->odep_value[i].sw = reg_value.sw;
	break;
      case vt_qword:
        re->odep_value[i].q = reg_value.q;
	break;
      case vt_sqword:
        re->odep_value[i].sq = reg_value.sq;
	break;
      case vt_sfloat:
        re->odep_value[i].f = reg_value.f;
	break;
      case vt_dfloat:
        re->odep_value[i].d = reg_value.d;
	break;
      case vt_addr:
        re->odep_value[i].addr = reg_value.addr;
	break;
      default:
	fatal("Invalid type for set_odep_list");
      }
    }
  }
}
 
/* Read a value from memory and place in mem_value.  The value may be
 * obtained from the LSQ if match_in_LSQ is set.  In this case, the value
 * stored in the RE entry.  Otherwise, it is read from memory. */
enum md_fault_type
read_load_data(struct ROB_entry *re, void *mem_value, int size)
{
  enum md_fault_type fault;
  md_addr_t mem_addr = read_idep_list(re, DEP_NAME(DTMP)).addr;
  
  /* Check for faults. */
  fault = mem_check_fault(mem, Read, mem_addr, size);
  if (fault != md_fault_none) return fault;

  /* Check for a match in the LSQ. */
  if (re->match_in_LSQ) {
    switch (size) {
      case 1: *((byte_t *)mem_value) = re->mem_value.b; break;
      case 2: *((half_t *)mem_value) = re->mem_value.h; break;
      case 4: *((word_t *)mem_value) = re->mem_value.w; break;
      case 8: *((qword_t *)mem_value) = re->mem_value.q; break;
      default: fatal("Invalid size");
    }
  }
  else {
    mem_access(mem, Read, mem_addr, mem_value, size);
  }

  return md_fault_none;
}

/* Transfers the value in data to mem_value. */
enum md_fault_type
read_store_data(struct ROB_entry *re, union val_union *mem_value, qword_t data, int size)
{
  enum md_fault_type fault;
  md_addr_t mem_addr = read_idep_list(re, DEP_NAME(DTMP)).addr;

  /* Check for faults. */
  fault = mem_check_fault(mem, Write, mem_addr, size);
  if (fault != md_fault_none) return fault;

  /* Record the value written to memory. */
  switch(size) {
    case 1: mem_value->b = (byte_t)data; break; 
    case 2: mem_value->h = (half_t)data; break; 
    case 4: mem_value->w = (word_t)data; break; 
    case 8: mem_value->q =         data; break; 
    default: fatal("Invalid size");
  }

  return md_fault_none;
}

/* Transfers memory value in src into memory value of dest.  Returns 1
 * if operation was successful, and 0 otherwise. */
int
transfer_store_data(struct ROB_entry *src, struct ROB_entry *dest)
{
  /* Use the source store operand.  The store might not have
   * executed yet so mem_value might not be up to date. */
  union val_union src_value = src->idep_value[STORE_OP_INDEX];
  int src_size = src->mem_size;

  if (dest->mem_size == src_size) {
    switch (dest->mem_size) {
      case 1: dest->mem_value.b = src_value.b; break;
      case 2: dest->mem_value.h = src_value.h; break;
      case 4: dest->mem_value.w = src_value.w; break;
      case 8: dest->mem_value.q = src_value.q; break;
      default: fatal("Unsupported memory size");
    }
    return 1;
  }
  else if (dest->mem_size < src_size) {
      
    switch (dest->mem_size) {

    case 1: 
      dest->mem_value.b = (src_size == 2) ? (byte_t) src_value.h :
                          (src_size == 4) ? (byte_t) src_value.w :
                                            (byte_t) src_value.q;
      break;
    case 2:
      dest->mem_value.h = (src_size == 4) ? (half_t) src_value.w :
                                            (half_t) src_value.q;
      break;
    case 4:
      dest->mem_value.w = (word_t) src_value.q;
      break;
    default:
      fatal("Unsupported memory size");
    }
    return 1;
  }
  else {
    return 0;
  }
}

/* executes the instruction */
void execute_inst(struct ROB_entry *re)
{
  md_inst_t inst = re->IR;   /* machine.def needs inst to be defined */
  enum md_opcode op;
  union val_union mem_value;
  union val_union reg_value;

  /* assume there is no branch */
  re->NPC = re->PC + sizeof(md_inst_t);

  /* execution of syscalls are delayed until commit */
#define DECLARE_FAULT(FAULT) { re->fault = (FAULT); break; }
#define SYSCALL(INST) ;

  /* execute the instruction */
  MD_SET_OPCODE(op, inst);
  switch (op) {
#define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,I1,I2,I3)  	 	 \
  case OP:							 \
    SYMCAT(OP,_IMPL);						 \
    break;
#define DEFLINK(OP,MSK,NAME,MASK,SHIFT) 				 \
  case OP:							 \
     panic("attempted to execute a linking opcode");
#define CONNECT(OP)
#include "machine.def"
  default:
    fatal ("Invalid operation");
  }
}

/* WARNING:
 * Use the following function with care.  You must guarantee
 * that the instruction is allowed to reset. For instance,
 * a reservation station must be re-allocated if the
 * (non-memory) instruction has completed. */

/* reset the instruction, as if it hadn't been executed */
void
reset_inst(struct ROB_entry *re)
{
  int i;

  re->queued = FALSE;
  re->issued = FALSE;
  re->completed = FALSE;
  re->match_in_LSQ = FALSE;

  for (i = 0; i < MAX_ODEPS; i++) {

#if defined(TARGET_PISA)
    if ((re->odep_name[i] != DNA) &&
        (re->odep_name[i] != DGPR(MD_REG_ZERO)))
    {
#elif defined(TARGET_ALPHA)
    if ((re->odep_name[i] != DNA) &&
        (re->odep_name[i] != DGPR(MD_REG_ZERO)) &&
	(re->odep_name[i] != DFPR(MD_REG_ZERO)))
    {
#endif
      re->odep_valid[i] = FALSE;
    }
  }
  re->tag++;
}

/* allocate and initialize ROB */
void
rob_init(void)
{
  ROB = calloc(ROB_size, sizeof(struct ROB_entry));
  if (!ROB) fatal("out of virtual memory");
    
  ROB_num = 0;
  ROB_head = ROB_tail = 0;
  ROB_count = 0;
  ROB_fcount = 0;
}

/* initialize RS variables */
void
rs_init(void)
{
  RS_num = 0;
  RS_count = 0;
  RS_fcount = 0;
}

/* dump the contents of an ROB entry */
void
rob_dumpent(struct ROB_entry *re,		/* ptr to ROB entry */
	    int index,				/* entry index */
	    FILE *stream,			/* output stream */
	    int header)				/* print header? */
{
  if (!stream) stream = stderr;

  if (header)
    fprintf(stream, "idx: %2d inst: `", index);
  else
    fprintf(stream, "\tinst: `");
  md_print_insn(re->IR, re->PC, stream);
  fprintf(stream, "'");
  if (re->ea_comp)
    fprintf(stream, " (ea_comp)\n");
  else
    fprintf(stream, "\n");

  myfprintf(stream, "\tPC: 0x%08p, pred_NPC: 0x%08p\n",
	    re->PC, re->pred_NPC);

  if (re->in_LSQ)
    fprintf(stream, "\tisq_index: %d, rob_index: %d\n",
	    re->isq_index, re->rob_index);
  else
    fprintf(stream, "\tisq_index: %d\n", re->isq_index);

  fprintf(stream, "\ttag: %u, seq: %u, ptrace_seq: %u\n",
	  re->tag, re->seq, re->ptrace_seq);
  fprintf(stream, "\trecover_inst: %s, timing_dep: %s\n",
	  re->recover_inst ? "T" : "F",
	  re->timing_dep ? "T" : "F");

  if (re->in_LSQ) {
    fprintf(stream, "\tmatch_in_LSQ: %s\n",
	    re->match_in_LSQ ? "T" : "F");
    if (use_blind_spec) {
      fprintf(stream, "\tblind_recover: %s\n",
  	      re->blind_recover ? "T" : "F");
    }
  }

  fprintf(stream, "\tstatus: ");
  if (re->completed) fprintf(stream, "COMPLETED\n");
  else if (re->issued) fprintf(stream, "ISSUED\n");
  else if (re->queued) fprintf(stream, "READY (Scheduler delay: %d)\n",
      re->schedule_wait_time);
  else fprintf(stream, "NOT READY\n");

}

/* dump the contents of the entire ROB */
void
rob_dump(FILE *stream)
{
  int num, head;
  struct ROB_entry *re;

  fprintf(stream, "** ROB state **\n");
  fprintf(stream, "ROB_head: %d, ROB_tail: %d\n", ROB_head, ROB_tail);
  fprintf(stream, "ROB_num: %d\n", ROB_num);

  num = ROB_num;
  head = ROB_head;
  while (num) {
    re = &ROB[head];
    rob_dumpent(re, re - ROB, stream, /* header */TRUE);
    head = (head + 1) % ROB_size;
    num--;
  }
}

/* dump the contents of the reservation stations */
void
rs_dump(FILE *stream)
{
  int num, head;
  struct ROB_entry *re;

  fprintf(stream, "** RS state **\n");
  fprintf(stream, "RS_num: %d\n", RS_num);

  num = ROB_num;
  head = ROB_head;
  while (num) {
    re = &ROB[head];
    if (!re->completed)
      rob_dumpent(re, re - ROB, stream, /* header */TRUE);
    head = (head + 1) % ROB_size;
    num--;
  }
}

/* allocate and initialize the LSQ */
void
lsq_init(void)
{
  LSQ = calloc(LSQ_size, sizeof(struct ROB_entry));
  if (!LSQ) fatal("out of virtual memory");

  LSQ_num = 0;
  LSQ_head = LSQ_tail = 0;
  LSQ_count = 0;
  LSQ_fcount = 0;

  sinfo_table = calloc(LSQ_size, sizeof(struct store_info));
  if (!sinfo_table) fatal("out of virtual memory");

  mem_req_table = calloc(LSQ_size, sizeof(struct mem_req));
  if (!mem_req_table) fatal("out of virtual memory");
}

/* dump the contents of the entire LSQ */
void
lsq_dump(FILE *stream)
{
  int num, head;
  struct ROB_entry *re;

  if (!stream) stream = stderr;

  fprintf(stream, "** LSQ state **\n");
  fprintf(stream, "LSQ_head: %d, LSQ_tail: %d\n", LSQ_head, LSQ_tail);
  fprintf(stream, "LSQ_num: %d\n", LSQ_num);

  num = LSQ_num;
  head = LSQ_head;
  while (num) {
    re = &LSQ[head];
    rob_dumpent(re, re - LSQ, stream, /* header */TRUE);
    head = (head + 1) % LSQ_size;
    num--;
  }
}

/* initialize the free RE_LINK pool */
void
relink_init(int nlinks)
{
  int i;
  struct RE_link *link;

  relink_free_list = NULL;
  for (i=0; i<nlinks; i++) {
    link = calloc(1, sizeof(struct RE_link));
    if (!link) fatal("out of virtual memory");
    link->next = relink_free_list;
    relink_free_list = link;
  }
}

/* initialize the event queue */
void
eventq_init(void)
{
  event_queue = NULL;
}

/* dump the contents of the event queue */
void
eventq_dump(FILE *stream)
{
  struct RE_link *ev;

  if (!stream) stream = stderr;

  fprintf(stream, "** event queue state **\n");

  for (ev = event_queue; ev != NULL; ev = ev->next) {

    if (RELINK_VALID(ev)) {
      struct ROB_entry *re = RELINK_RE(ev);
      fprintf(stream, "idx: %2d when: %.0f\n",
	      (int)(re - (re->in_LSQ ? LSQ : ROB)), (double)ev->x.when);
      rob_dumpent(re, re - (re->in_LSQ ? LSQ : ROB),
	          stream, FALSE);
    }
  }
}

/* Insert an event for RE into the event queue. Event queue is sorted from
 * earliest to latest event. Event and associated side-effects will be
 * apparent at the start of cycle "when" */
void
eventq_queue_event(struct ROB_entry *re, tick_t when)
{
  struct RE_link *prev, *ev, *new_ev;

  if (re->completed) panic("event completed");
    
  if (when <= sim_cycle) panic("event occurred in the past");

  /* get a free event record */
  RELINK_NEW(new_ev, re);
  new_ev->x.when = when;

  /* locate insertion point */
  for (prev=NULL, ev=event_queue;
       ev && ev->x.when < when;
       prev=ev, ev=ev->next);

  if (prev) {
    /* insert middle or end */
    new_ev->next = prev->next;
    prev->next = new_ev;
  }
  else {
    /* insert at beginning */
    new_ev->next = event_queue;
    event_queue = new_ev;
  }
}


/* Return the next event that has already occurred. Returns NULL when no
 * remaining events or all remaining events are in the future. */
struct ROB_entry *
eventq_next_event(void)
{
  struct RE_link *ev;

  if (event_queue && event_queue->x.when <= sim_cycle) {

    /* unlink and return first event on priority list */
    ev = event_queue;
    event_queue = event_queue->next;

    if (RELINK_VALID(ev)) {
      struct ROB_entry *re = RELINK_RE(ev);

      /* reclaim event record */
      RELINK_FREE(ev);

      /* event is valid, return ROB entry */
      return re;
    }
    else {
      /* reclaim event record */
      RELINK_FREE(ev);

      /* receiving inst was squashed, return next event */
      return eventq_next_event();
    }
  }
  else {
    /* no event or no event is ready */
    return NULL;
  }
}

/* functions to provide the latency misprediction queue */

/* initialize the mp_queue */
void
mpq_init(void)
{
  mp_queue = NULL;
}

/* dump the contents of the mp_queue */
void
mpq_dump(FILE *stream)
{
  struct RE_link *ev;

  if (!stream) stream = stderr;

  fprintf(stream, "** mp_queue state **\n");

  for (ev = mp_queue; ev != NULL; ev = ev->next) {

    if (RELINK_VALID(ev)) {
      struct ROB_entry *re = RELINK_RE(ev);
      fprintf(stream, "idx: %2d: @ %.0f\n",
	      (int)(re - (re->in_LSQ ? LSQ : ROB)), (double)ev->x.when);
      rob_dumpent(re, re - (re->in_LSQ ? LSQ : ROB),
	          stream, /* !header */FALSE);
    }
  }
}

/* add an event to the mp_queue */
void
mpq_queue_event(struct ROB_entry *re, tick_t when)
{
  struct RE_link *prev, *ev, *new_ev;

  if (re->completed) panic("event completed");
    
  if (when <= sim_cycle) panic("event occurred in the past");

  /* get a free event record */
  RELINK_NEW(new_ev, re);
  new_ev->x.when = when;

  /* locate insertion point */
  for (prev=NULL, ev=mp_queue;
       ev && ev->next != NULL;
       prev=ev, ev=ev->next);

  if (prev) {
    /* insert middle or end */
    new_ev->next = prev->next;
    prev->next = new_ev;
  }
  else {
    /* insert at beginning */
    new_ev->next = mp_queue;
    mp_queue = new_ev;
  }
}

/* Return the next event that has already occurred. Returns NULL when no
 * remaining events or all remaining events are in the future. */
struct ROB_entry *
mpq_next_event(void)
{
  struct RE_link *ev;

  if (mp_queue && mp_queue->x.when <= sim_cycle) {

    /* unlink and return first event on priority list */
    ev = mp_queue;
    mp_queue = mp_queue->next;

    if (RELINK_VALID(ev)) {
      struct ROB_entry *re = RELINK_RE(ev);

      /* reclaim event record */
      RELINK_FREE(ev);

      /* event is valid, return ROB entry */
      return re;
    }
    else {
      /* reclaim event record */
      RELINK_FREE(ev);

      /* receiving inst was squashed, return next event */
      return mpq_next_event();
    }
  }
  else {
    /* no event or no event is ready */
    return NULL;
  }
}

/* initialize the ready queue */
void
readyq_init(void)
{
  ready_queue = NULL;
}

/* dump the contents of the ready queue */
void
readyq_dump(FILE *stream)
{
  struct RE_link *link;

  if (!stream) stream = stderr;

  fprintf(stream, "** ready queue state **\n");

  for (link = ready_queue; link != NULL; link = link->next) {

    /* is entry still valid? */
    if (RELINK_VALID(link)) {
      struct ROB_entry *re = RELINK_RE(link);
      rob_dumpent(re, re - (re->in_LSQ ? LSQ : ROB),
	          stream, TRUE);
    }
  }
}

/* Insert ready node into the ready list using ready instruction scheduling
 * policy. Currently, the following scheduling policy is enforced:
 *
 * 1. Memory, long latency operations, and branches are automatically placed
 * the head of the queue.
 *
 * 2. All other instructions are inserted into the queue in program order.
 *
 * This policy works well because branches pass through the machine quicker
 * which works to reduce branch misprediction latencies, and very long latency
 * instructions (such loads and multiplies) get priority since they are very
 * likely on the program's critical path. */
void
readyq_enqueue(struct ROB_entry *re)		/* RE to enqueue */
{
  struct RE_link *prev, *node, *new_node;

  /* node is now queued */
  if (re->queued) panic("node is already queued");
  re->queued = TRUE;

  /* get a free ready list node */
  RELINK_NEW(new_node, re);
  new_node->x.seq = re->seq;

  /* locate insertion point */
  if (re->in_LSQ || MD_OP_FLAGS(re->op) & (F_LONGLAT|F_CTRL)) {
      /* insert loads/stores and long latency ops at the head of the queue */
      prev = NULL;
      node = ready_queue;
  }
  else {
    /* otherwise insert in program order (earliest seq first) */
    for (prev=NULL, node=ready_queue;
      node && node->x.seq < re->seq;
      prev=node, node=node->next);
  }

  if (prev) {
    /* insert middle or end */
    new_node->next = prev->next;
    prev->next = new_node;
  }
  else {
    /* insert at beginning */
    new_node->next = ready_queue;
    ready_queue = new_node;
  }
}

/*
 * LSQ_REFRESH() - memory access dependence checker/scheduler
 *
 * This function locates ready instructions whose memory dependencies have
 * been satisfied. This is accomplished by walking the LSQ for loads, looking
 * for blocking memory dependency condition (e.g., earlier store with an
 * unknown address).
 */
void
lsq_refresh(void)
{
  int i, j;		/* loop iteration variables */
  int index;		/* index of LSQ entry */
  int sinfo_count = 0;	/* number of elements in sinfo_table */
  int prevent_queue;	/* set if a load shouldn't be placed into the ready queue */
  md_addr_t load_addr;	/* address of the load */
  int blind_sindex; 	/* LSQ index of a store that potentially can cause a
		  	 * blind speculation miss. */

  /* Scan entire queue for ready loads.  Start at the oldest instruction
   * (head) until we reach the tail. */
  for (i=0, index=LSQ_head;
       i < LSQ_num;
       i++, index=(index + 1) % LSQ_size)
  {
    /* check for stores */
    if ((MD_OP_FLAGS(LSQ[index].op) & (F_MEM|F_STORE)) == (F_MEM|F_STORE)) {

      /* grab the address from the oracle */
      sinfo_table[sinfo_count].addr = ISQ[LSQ[index].isq_index].mem_addr;
      sinfo_table[sinfo_count].index = index;

      /* set the flags appropriately */
      if (!STORE_ADDR_READY(&LSQ[index])) {
        sinfo_table[sinfo_count].sta_unknown = 1;
        sinfo_table[sinfo_count].std_unknown = 0;
      }
      else if (!OPERANDS_READY(&LSQ[index])) {
        sinfo_table[sinfo_count].sta_unknown = 0;
        sinfo_table[sinfo_count].std_unknown = 1;
      }
      else { /* STORE_ADDR_READY() && OPERANDS_READY() */
        sinfo_table[sinfo_count].sta_unknown = 0;
        sinfo_table[sinfo_count].std_unknown = 0;
      }
      sinfo_count++;
    }

    /* check for loads */
    if (((MD_OP_FLAGS(LSQ[index].op) & (F_MEM|F_LOAD)) == (F_MEM|F_LOAD))
      && !LSQ[index].queued
      && !LSQ[index].issued
      && !LSQ[index].completed
      && OPERANDS_READY(&LSQ[index])
    ) {
      
      /* default values */
      prevent_queue = 0;
      load_addr = read_idep_list(&(LSQ[index]), DEP_NAME(DTMP)).addr;
      blind_sindex = -1;

      /* scan the store info table (from newest to oldest) to see if the
       * load should be queued or not */
      for (j = sinfo_count - 1;  j >= 0; j--) {

        /* if an STA unknown is encountered, block in normal operation */
        if (sinfo_table[j].sta_unknown && !use_blind_spec && !perfect_disambig) {
          prevent_queue = 1;
          break;
        }

        /* check to see if the address matches */
        if (sinfo_table[j].addr == load_addr) {

          /* If the STA is unknown, we must be in blind speculation mode or
           * using perfect disambiugtion; otherwise we don't get to this point.
	   * If both are set, use_blind_spec takes precedence. */

	  /* For blind speculation, the LSQ index of the store is stored
           * in the event of a miss.  This index is only set once in order to
           * guarantee that the most recent store causes the recovery.  Since
           * the address is unknown, the loop must continue to see if there is
           * a match with an older entry where the address is known.  If the
           * STD is unknown in such a situation, queuing would be blocked and no
           * blind speculation miss as a result. */
          if (sinfo_table[j].sta_unknown && use_blind_spec) {
            if (blind_sindex == -1) blind_sindex = sinfo_table[j].index;
	  }

	  /* For perfect disambiguation, the instruction is blocked because the
	   * address matches but is still unknown. */
          else if (sinfo_table[j].sta_unknown && perfect_disambig) {
	    prevent_queue = 1;
	    break;
	  }
	  else if (sinfo_table[j].sta_unknown) {
	    fatal("Failure to block an STA unknown.\n");
          }
          else if (sinfo_table[j].std_unknown) {	/* STD unknown - load is blocked */
            prevent_queue = 1;
            break;
          }
          else {	/* STA and STD both are known */
	    int transfer_ok;

	    /* Attempt to forward the data from the store into the load.  If the
	     * transfer could not take place due to a partial store, then the load
	     * must wait until the store has completed. The match_in_LSQ bit is set
	     * to ensure the proper operation of the instruction when it executes. */
	    transfer_ok = transfer_store_data(&(LSQ[sinfo_table[j].index]), &(LSQ[index]));
	    if (transfer_ok) {
              LSQ[index].match_in_LSQ = 1;
	    }
	    else {
	      prevent_queue = 1;
	    }
	    break;
          }
        }
      }
 
      /* queue the instruction unless it is prohibitied */
      if (!prevent_queue) {

        /* update the store's LSQ entry if there is a blind speculation miss */
        if (blind_sindex != -1) {
	  LSQ[blind_sindex].blind_recover = TRUE;
	  LSQ[index].blind_recover = TRUE;
	}
        readyq_enqueue(&LSQ[index]);
      }
    }
  }
}

/*
 * MASE_ISSUE() - issue instructions to functional units
 *
 * Attempt to issue all operations in the ready queue. Instructions in the ready
 * queue have all register and memory dependencies satisfied. This function checks
 * to see if a function unit is available in this cycle to commence execution of
 * the operation. If all goes well, the function unit is allocated, a writeback
 * event is scheduled, and the instruction begins execution. */
void
mase_issue(void)
{
  int i;				/* loop traversal variable */
  int index;				/* LSQ traversal variable */
  int prevent_issue;			/* set if issue should be prevented */
  int prevent_queue;			/* set if instr shouldn't be added to event queue */
  int events;				/* keeps track of events */
  int lat;				/* latency of operation */
  enum mem_status status;		/* status of TLB/cache access */
  int n_issued;				/* number of ops issued */
  struct RE_link *node, *next_node;	/* linked list traversal ptrs */
  struct res_template *fu;		/* functional unit */
  struct ROB_entry *rob_e;


  /* Copy, then blow away the ready queue. NOTE: The ready queue is
   * always totally reclaimed each cycle; instructions that are not
   * issued are explicitly reinserted into the ready queue.  This
   * management strategy ensures that the ready queue is always
   * properly sorted. */
  node = ready_queue;
  ready_queue = NULL;

  /* check for delays to scheduling */
  if(scheduler_replay) {
    int replay_flag = 0;
    while((rob_e = mpq_next_event())) {
      /* we had a latency mispredict - penalize */
      /*    if(!selective_squash) {   */
      /* everybody hurts - sometimes */
      struct ROB_entry *re;
      replay_flag = 1;
      for(i=0;i < ROB_size; i++) {
	re = &(ROB[i]);
	if(!re->issued && !re->completed) {
	  re->schedule_wait_time = schedule_delay;
	}
      }
      /*    } else {  */
    }
    if(replay_flag) num_replays++;
  }
  
  /* Visit all ready instructions (i.e., insts whose register input
   * dependencies have been satisfied). Stop issue when no more instructions
   * are available or issue bandwidth is exhausted. */
  for (n_issued=0;
       node && n_issued < issue_width;
       node = next_node)
  {
    next_node = node->next;
    prevent_issue = FALSE;
    prevent_queue = FALSE;
    events = 0;

    if (RELINK_VALID(node)) {
      struct ROB_entry *re = RELINK_RE(node);

      if (!OPERANDS_READY(re) || !re->queued
          || re->issued || re->completed)
        panic("issued inst !ready, issued, or completed");

      /* instruction hasn't made it it execute yet - skip it and try again next cycle */
      if(re->schedule_wait_time > 0) {
	re->queued = FALSE;
	readyq_enqueue(re);
        RELINK_FREE(node);
	continue;
      }

      /* re is now un-queued */
      re->queued = FALSE;

      /* check for stores */
      if (re->in_LSQ && ((MD_OP_FLAGS(re->op) & (F_MEM|F_STORE)) == (F_MEM|F_STORE))) {
	      
        /* Stores complete in effectively zero time. Result is written
         * into the LSQ. The actual store into the memory system occurs
         * when the instruction is retired. (See mase_commit()) */
        re->issued = TRUE;
        re->completed = TRUE;
        if ((re->odep_name[0] != DNA) || (re->odep_name[1] != DNA)) panic("store creates result");
        if (re->recover_inst) panic("mis-predicted store");

        /* entered execute stage - indicate in pipe trace */
        ptrace_newstage(re->ptrace_seq, PST_WRITEBACK,
          (re->blind_recover) ? PEV_BSPECMISS : 0);

        /* one more instruction issued */
        n_issued++;

        /* check to see if there is a blind misspeculation */
        if (re->blind_recover && ((re - LSQ) != LSQ_tail)) {
	  struct ROB_entry *load_re;
	  int recovery_needed = FALSE;

	  /* scan the LSQ to see if recovery is necessary */
          for (index = (re - LSQ + 1) % LSQ_size;
               index != LSQ_tail;
               index = (index + 1) % LSQ_size)
          {
	    /* TODO: Could be an illegal use of oracle data here. */
	    if (((MD_OP_FLAGS(LSQ[index].op) & (F_MEM|F_STORE)) == (F_MEM|F_STORE))) {
	      if (ISQ[LSQ[index].isq_index].mem_addr == read_idep_list(re, DEP_NAME(DTMP)).addr)
		break;
	    }
	    else if (((MD_OP_FLAGS(LSQ[index].op) & (F_MEM|F_LOAD)) == (F_MEM|F_LOAD))) {

	      if (!LSQ[index].blind_recover) continue;
	      if (ISQ[LSQ[index].isq_index].mem_addr == read_idep_list(re, DEP_NAME(DTMP)).addr) {

		int transfer_ok;
	        load_re = &(LSQ[index]);

	        /* Transfer the store data to the load. If the transfer was successful,
	         * the instruction will need to be re-executed if has been already
	         * issued.  The re-execution is necessary in order to get the correct
	         * value into the destination register of the load.  If the transfer
	         * was not successful, there is a partial store.  The instruction is
	         * reset and must wait until the store finishes. */
	        transfer_ok = transfer_store_data(re, load_re);
	        if (transfer_ok) {
	          load_re->match_in_LSQ = TRUE;
	          if (load_re->issued) execute_inst(load_re);
	        }
	        else {
	          reset_inst(load_re);
	        }

	        if (load_re->completed) {
	  	  recovery_needed = TRUE;
		  break;
	        }
	      }
	    }
	  }

	  /* Schedule a recovery if the instruction was completed. Dependent
	   * instructions could have wrong values. */
	   recovery_needed=1;
	  if (recovery_needed) {
            int isq_index = load_re->isq_index;
            checker_recover(isq_index);
            rob_recover(load_re->rob_index);
            fe_recover(ISQ[isq_index].NPC);
            blind_spec_all_flushes++;
            if (ISQ[isq_index].spec_mode == 0) blind_spec_flushes++;
          }
	}
      }
      else if (MD_OP_FUCLASS(re->op) == FUClass_NA) { 
        /* The instruction does not need a functional unit, so the
         * instruction is issued. */
        re->issued = TRUE;

        /* schedule a result event using latency of 1 */
        eventq_queue_event(re, sim_cycle + 1);

        /* entered execute stage - indicate in pipe trace */
        ptrace_newstage(re->ptrace_seq, PST_EXECUTE,
		        re->ea_comp ? PEV_AGEN : 0);

        /* one more instruction issued */
        n_issued++;
      }
      else {
        
        /* try to obtain a functional unit */
	fu = res_get(fu_pool, MD_OP_FUCLASS(re->op));

        /* If there is no functional unit - can't issue */
	if (!fu) {
          prevent_issue = TRUE;
        }

	/* check for loads so they access the cache */
        else if (re->in_LSQ && 
          ((MD_OP_FLAGS(re->op) & (F_MEM|F_LOAD)) == (F_MEM|F_LOAD)))
	{
	  /* For load instructions, we need to determine the cache access latency.
	   * The cache access functions return astatus and/or a latency. Here
	   * are the statuses:
	   * 
	   * MEM_RETRY: The memory system cannot handle the request. The request
	   * is placed back onto the ready queue.  If the request matched in the
	   * LSQ, the cache is simply not updated.
	   *
	   * MEM_UNKNOWN: The memory system is unable to immediately return the
	   * latency.  A mem_req_table entry is created that the callback function
	   * uses to add it to the event queue when the latency is determined.  No
	   * entry is created if the requested matched in the LSQ since the latency
	   * is automatically one.
	   *
	   * MEM_KNOWN (with latency): The memory system returns a latency of the
	   * operation.  This latency is used to schedule the writeback event unless
	   * the request matched in the LSQ where the latency is 1.
	   */

	  md_addr_t mem_addr = read_idep_list(re, DEP_NAME(DTMP)).addr;
	  int valid_addr = MD_VALID_ADDR(mem_addr);
	  lat = 0;

	  /* access the DTLB */
	  if (dtlb && valid_addr && !re->dtlb_miss) {

	    status = mase_cache_access(dtlb,
	      Read, (mem_addr & ~(re->mem_size-1)), NULL, re->mem_size,
	      sim_cycle, NULL, NULL, load_cb, req_id, &lat
            );
	    if ((status == MEM_RETRY) && !re->match_in_LSQ) {
              prevent_issue = TRUE;
            }
            else {
	      if (status == MEM_UNKNOWN) {
	        if (!re->match_in_LSQ) {
                  for (i = 0; i <= LSQ_size; i++) {
                    if (!(MEMREQ_VALID(mem_req_table[i]))) break;
                  } 
                  if (i == LSQ_size) fatal("mem req table is full");
                  mem_req_table[i].rid = req_id;
                  mem_req_table[i].tag = re->tag;
                  mem_req_table[i].re = re;
                  prevent_queue = TRUE;
	        }
                req_id++;
              }
              re->dtlb_miss = (lat > 1) || (status == MEM_UNKNOWN);
	      if (re->dtlb_miss) events |= PEV_TLBMISS;
	    }
	  }
          else {
            re->dtlb_miss = FALSE;
          }

	  /* access the data cache */
	  if (cache_dl1 && valid_addr && !prevent_issue && !re->dtlb_miss) {

	    status = mase_cache_access(cache_dl1,
	      Read, (mem_addr & ~(re->mem_size-1)), NULL, re->mem_size,
	      sim_cycle, NULL, NULL, load_cb, req_id, &lat
            );
	    if ((status == MEM_RETRY) && !re->match_in_LSQ) {
              prevent_issue = TRUE;
            }
            else {
	      if (status == MEM_UNKNOWN) {
	        if (!re->match_in_LSQ) {
                  for (i = 0; i <= LSQ_size; i++) {
                    if (!(MEMREQ_VALID(mem_req_table[i]))) break;
                  } 
                  if (i == LSQ_size) fatal("mem req table is full");
                  mem_req_table[i].rid = req_id;
                  mem_req_table[i].tag = re->tag;
                  mem_req_table[i].re = re;
                  prevent_queue = TRUE;
	        }
                req_id++;
	      }
	      if ((lat > cache_dl1_lat) || (status == MEM_UNKNOWN)) events |= PEV_CACHEMISS;
	    }
          }
	  else {
	    lat = cache_dl1 ? cache_dl1_lat : fu->oplat;
	  }

	  /* The latency is automatically 1 if there is a match in the LSQ. */
          if (re->match_in_LSQ) lat = 1;

	  /* if there is a d1 cache miss it is a latency misprediction */
	  /* TODO: This does not work when status == MEM_UNKNOWN */
	  if (scheduler_replay && (lat > cache_dl1_lat)) {
	    mpq_queue_event(re,sim_cycle + cache_dl1_lat);
	  }
        }
        else {
	  /* not load - use deterministic functional unit latency */
          lat = fu->oplat; 
        }

        if (!prevent_issue) {
	  /* schedule functional unit release event */
	  if (fu->master->busy) panic("functional unit already in use");
	  fu->master->busy = fu->issuelat;

          /* add event to queue */
          if (!prevent_queue)
	    eventq_queue_event(re, sim_cycle + lat);

	  /* entered execute stage - indicate in pipe trace */
	  ptrace_newstage(re->ptrace_seq, PST_EXECUTE,
			  ((re->ea_comp ? PEV_AGEN : 0)
			  | events));

	  /* one more instruction issued */
	  re->issued = TRUE;
	  n_issued++;
        }
        else {
          /* cannot issue - put operation back onto ready queue */
          readyq_enqueue(re);
        }
      } 

      /* execute the instruction if it issued 
         and deallocate its reservation station, unless we're using replay */
      if (re->issued) {
     	execute_inst(re);
	if((!scheduler_replay) & !(re->in_LSQ)) {
	  RS_num--;
	}
      }
    }

    /* Reclaim ready list entry. This is done whether or not the instruction
     * issued. If an instruction did not issue, it is reinserted into the
     * ready queue with a brand new RELINK structure. */
    RELINK_FREE(node);
  }

  /* Put any instruction not issued back into the ready queue. Go through
   * normal channels to ensure instruction stay ordered correctly. */
  for (; node; node = next_node) {
    next_node = node->next;

    if (RELINK_VALID(node)) {
      struct ROB_entry *re = RELINK_RE(node);

      /* node is now un-queued */
      re->queued = FALSE;

      /* put operation back onto the ready list */
      readyq_enqueue(re);
    }

    /* Reclaim ready list entry. The instruction was reinserted into
     * the ready queue with a brand new RELINK structure. */
    RELINK_FREE(node);
  }

  /* instructions make progress through the scheduler */
  for(i=0;i < ROB_size;i++) {
    struct ROB_entry *rob_ent = &(ROB[i]);
    if(rob_ent->schedule_wait_time > 0) {
      rob_ent->schedule_wait_time--;
    }
  }
}

/* mase-exec.h - sim-mase exec stage */

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

/* read a value from the idep list that correspons to dep_name */
union val_union
read_idep_list(struct ROB_entry *re, int dep_name);

/* set the value of an odep value entry */
void
set_odep_list(struct ROB_entry *re, int reg_name, union val_union reg_value, enum val_type reg_type);

/* read value of load */
enum md_fault_type
read_load_data(struct ROB_entry *re, void *mem_value, int size);

/* read value of store */
enum md_fault_type
read_store_data(struct ROB_entry *re, union val_union *mem_value, qword_t data, int size);

/* Reorder buffer (ROB) */
extern struct ROB_entry *ROB;		/* Reorder buffer */
extern int ROB_head, ROB_tail;		/* ROB head and tail pointers */
extern int ROB_num;			/* num entries currently in ROB */

extern int RS_num; /* num reservation stations currently occupied */

/*
 * Load Store Queue (LSQ) - Holds loads and stores in program order.
 *
 * Status of load/store access:
 *
 *   issued: address computation complete, memory access in progress
 *   completed: memory access has completed, stored value available
 *   squashed: memory access was squashed, ignore this entry
 *
 * Loads may execute when:
 *   1. Register operands are ready.
 *   2. Memory operands are ready. 
 *   3. No earlier unresolved store.
 *
 * Loads are serviced by:
 *   1. Previous store at same address in LSQ (hit latency)
 *   2. Data cache (hit latency + miss latency)
 *
 * Stores may execute when:
 *   1. Register operands are ready.
 *
 * Stores are serviced by:
 *   1. Depositing store value into the LSQ.
 *   2. Writing store value to the store buffer (plus tag check) at commit.
 *   3. Writing store buffer entry to data cache when cache is free.
 *
 * The LSQ can bypass a store value to a load in the same cycle the store
 * executes (using a bypass network). Thus stores complete in effective
 * zero time after their effective address is known.
 */
extern struct ROB_entry *LSQ;           /* Load Store Queue */
extern int LSQ_head, LSQ_tail;          /* LSQ head and tail pointers */
extern int LSQ_num;                     /* num entries currently in LSQ */

/*
 * Input dependencies for stores in the LSQ:
 *   idep #0 - operand input (value that is stored)
 *   idep #1 - effective address input (address of store operation)
 */
#define STORE_OP_INDEX                  0
#define STORE_ADDR_INDEX                1
#define STORE_OP_READY(RE)              ((RE)->idep_ready[STORE_OP_INDEX])
#define STORE_ADDR_READY(RE)            ((RE)->idep_ready[STORE_ADDR_INDEX])

/* RE link free list - grab RE_LINKs from here, when needed */
extern struct RE_link *relink_free_list;

/* Maximum number of cases of unknown store addresses and store data. */
#define MAX_STA_UNKNOWNS		128
#define MAX_STD_UNKNOWNS		64

/* request id - for memory accesses */
extern unsigned int req_id;

/* allocate and initialize ROB */
void
rob_init(void);

void
rs_init(void);

/* dump the contents of an ROB entry */
void
rob_dumpent(struct ROB_entry *re,	/* ptr to ROB entry */
	    int index,			/* entry index */
	    FILE *stream,		/* output stream */
	    int header);		/* print header? */

/* dump the contents of the entire ROB */
void
rob_dump(FILE *stream);

/* dump the contents of the reservation stations */
void
rs_dump(FILE *stream);

/* allocate and initialize the LSQ */
void
lsq_init(void);

/* dump the contents of the entire LSQ */
void
lsq_dump(FILE *stream);

/* initialize the free RE_LINK pool */
void
relink_init(int nlinks);		/* total number of RE_LINK available */

/* initialize the event queue */
void
eventq_init(void);

/* dump the contents of the event queue */
void
eventq_dump(FILE *stream);

/* Insert an event for RE into the event queue. Event queue is sorted from
 * earliest to latest event. Event and associated side-effects will be
 * apparent at the start of cycle "when" */
void
eventq_queue_event(struct ROB_entry *re, tick_t when);

/* Return the next event that has already occurred. Returns NULL when no
 * remaining events or all remaining events are in the future. */
struct ROB_entry *
eventq_next_event(void);

/* initialize the ready queue */
void
readyq_init(void);

/* dump the contents of the ready queue */
void
readyq_dump(FILE *stream);			/* output stream */

/* Insert ready node into the ready list using ready instruction scheduling
 * policy. Currently, the following scheduling policy is enforced:
 *
 * First: memory operand, long latency operands, and branch instructions
 *
 * Then: all other instructions, oldest instructions first
 *
 * This policy works well because branches pass through the machine quicker
 * which works to reduce branch misprediction latencies, and very long latency
 * instructions (such loads and multiplies) get priority since they are very
 * likely on the program's critical path. */
void
readyq_enqueue(struct ROB_entry *re);		/* RE to enqueue */

/* initialize the mp_queue */
void
mpq_init(void);

/* dump the contents of the mp_queue */
void
mpq_dump(FILE *stream);

/* Insert an event for RE into the mp_queue. mp_queue is sorted from
 * earliest to latest event. Event and associated side-effects will be
 * apparent at the start of cycle "when" */
void
mpq_queue_event(struct ROB_entry *re, tick_t when);

/* Return the next event that has already occurred. Returns NULL when no
 * remaining events or all remaining events are in the future. */
struct ROB_entry *
mpq_next_event(void);



/* This function locates ready instructions whose memory dependencies have
 * been satisfied. This is accomplished by walking the LSQ for loads, looking
 * for blocking memory dependency condition (e.g., earlier store with an
 * unknown address).
 */
void
lsq_refresh(void);

/* Attempt to issue all operations in the ready queue. Instructions in the ready
 * queue have all register dependencies satisfied. This function must then
 * 1) ensure the instructions memory dependencies have been satisfied (see
 * lsq_refresh() for details on this process) and 2) a function unit is available
 * in this cycle to commence execution of the operation. If all goes well, the
 * the function unit is allocated, a writeback event is scheduled, and the
 * instruction begins execution. */
void
mase_issue(void);

/* mase-checker.h - sim-mase dynamic checker and oracle */

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

/* ISQ - instruction state queue
 * Stores oracle data and output register information. */
extern struct inst_state *ISQ;                 /* queue of instruction state data (oracle) */
extern int ISQ_head;                           /* head of ISQ */
extern int ISQ_tail;                           /* tail of ISQ */
extern int ISQ_size;                           /* size of ISQ */
extern int ISQ_num;                            /* number of elements in ISQ */
#define INVALID_ISQ	-1

/* pre-update register file */
extern struct regs_t pu_regs;

/* architectural register file (updated at commit) */
extern struct regs_t regs;

/* architectural memory (updated at commit) */
extern struct mem_t *mem;

/* set if trap is encountered */
extern int is_trap;

/* set if syscall is encountered */
extern int is_syscall;

/* Reads or writes from pre_update memory.  For loads, if a memory location is not
 * in the pre-update memory, the architectural memory will supply the value. */
enum md_fault_type
pu_mem_access(struct mem_t *mem,           /* memory space to access */
              enum mem_cmd cmd,            /* Read or Write */
              md_addr_t addr,              /* target address to access */
              void *vp,                    /* host memory address to access */
              int nbytes                   /* number of bytes to access */);

/* removes and frees all of the ISQ state after the rollback point */
void
isq_reset(int rollback_index);

/* dump contents of the ISQ */
void
isq_dump(FILE *stream);

/* initialize checker */
void
checker_init();

/* Execute the instruction and create ISQ entry, returns index of ISQ entry. */
int
checker_exec(md_addr_t fetch_PC, struct fetch_rec *IFQ);

/* Recover the checker to the instruction indicated by the rollback index.
 * Pre-updated state is changed to reflect the state of the machine up to
 * (and including) the rollback instruction.
 * 
 * All ISQ entries after the rollback instruction are removed.  This function
 * is called when a branch misprediction occurs and can be called for other
 * mispredictions (such as address and value mispredictions). */
void
checker_recover(int rollback_index);

/* Check to see if the core results match the oracle results.  
 * Returns 1 if an error was found, 0 if there was no error.  */
int
checker_check(struct ROB_entry *re, struct inst_state *isq);

void
checker_correct(struct ROB_entry *re, int );

static void
process_timing_dep(struct ROB_entry *re, struct inst_state *isq);


/* First, checker_check is called to see if the core results match the
 * oracle results.  In either case, the instruction is commited using
 * the oracle data.  If a mismatched occurs, the pipeline is flushed
 * and restarted with the subsequent instruction.  Returns 1 if an
 * error was found, 0 if there was no error.  If this is an effective
 * address instruction, the LSQ entry must have been verified earlier
 * using checker_commit.  The result of this check is passed in via
 * lsq_check_error. */
int
checker_commit(struct ROB_entry *re, int lsq_chk_error);

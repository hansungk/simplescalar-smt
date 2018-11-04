/* mase-fe.h - sim-mase front-end */

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

/* IFQ (instruction fetch queue) */
extern struct fetch_rec *IFQ;
extern int IFQ_num;
extern int IFQ_tail, IFQ_head;

/* branch predictor */
extern struct bpred_t *pred;

/* rename table */
struct rename_entry *rename_table[MD_TOTAL_REGS];
#define IS_RENAME_HEAD(REN, NAME) (rename_table[NAME] == REN)

/* initialize the rename table */
void
rename_init(void);

/* get an entry from the rename_free_list */
struct rename_entry *
rename_get_entry(void);

/* dump the contents of the rename table */
void
rename_dump(FILE *stream);

/* removes the rename table entry */
void
rename_remove_entry(struct rename_entry *ent, int odep_name);

/* initialize the front end */
void
fe_init(void);

/* recovers the front end from a misprediction */
void
fe_recover(md_addr_t recover_PC);

/* dump contents of fetch queue */
void
ifq_dump(FILE *stream);			/* output stream */

/* Dispatch instructions from the IFQ: instructions are allocated ROB and
   LSQ (for load/stores) resources.  Input and output dependence chains
   are updated accordingly. */
void
mase_dispatch(void);

/* Initialize the values of PC at the beginning of the simulation.  This
 * is called after fastwording and before the main simulator loop. */
void
fetch_setup_PC(md_addr_t PC);		/* initial PC value */

/* dump contents of fetch stage */
void
fetch_dump(FILE *stream);		/* output stream */

/* fetches instructions */
void
mase_fetch(void);

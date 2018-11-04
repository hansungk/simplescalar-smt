/* mase-structs.h - sim-mase data structure definitions */

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

/* total input dependencies possible */
#define MAX_IDEPS               3

/* total output dependencies possible */
#define MAX_ODEPS               2

/* Describes the type of data */
enum val_type {
  vt_none,
  vt_byte,
  vt_sbyte,
  vt_half,
  vt_shalf,
  vt_word,
  vt_sword,
  vt_qword,
  vt_sqword,
  vt_sfloat,
  vt_dfloat,
  vt_addr
};

/* Union to support data types of different sizes */
union val_union {
    byte_t b;
    sbyte_t sb;
    half_t h;
    shalf_t sh;
    word_t w;
    sword_t sw;
    qword_t q;
    sqword_t sq;
    sfloat_t f;
    dfloat_t d;
    md_addr_t addr;
};

/* Pre-update memory hash table entry */
struct pu_hash_ent {
  struct pu_hash_ent *next;    /* ptr. to next hash table bucket */
  md_addr_t line_tag;          /* virtual address tag of spec. state */
  qword_t data;                /* memory value buffer */
  int isq_index;	       /* index of ISQ entry */
};

/* Structure that stores register data */
struct reg_state_t {
  int dep_name;			/* name of destination register */
  union val_union reg_value;	/* value of register */
  enum val_type reg_type;	/* type of register */
};

/* Structure that stores instruction (oracle) state */
struct inst_state {
  int valid;				/* ISQ entry is valid */
  md_inst_t IR;                         /* instruction data */
  enum md_opcode op;			/* op of instruction (used for commit stats) */
  md_addr_t PC;				/* PC of instruction */
  md_addr_t NPC;			/* correct NPC of instruction */
  int is_trap;				/* set if trap */
  int is_syscall;			/* set if syscall */
  int spec_mode;			/* set if on (branch) mispeculated path */
  enum md_fault_type fault;		/* fault, if any */
  md_addr_t mem_addr;			/* effective address (memory ops only) */
  int mem_size;				/* memory size (memory ops only) */
  union val_union mem_value;		/* memory data (stores only) */
  int is_write;				/* set if write (store) */
  struct reg_state_t reg_state[MAX_ODEPS]; /* array of output dependencies and values */
};

/* A record that transports info from fetch to dispatch. Contains some information
 * about the instruction including decode information from the oracle. */
struct fetch_rec {
  md_inst_t IR;                         /* instruction data */
  int isq_index;			/* index of ISQ entry */
  md_addr_t PC;				/* current PC */
  md_addr_t pred_NPC;           	/* predicted NPC */
  struct bpred_update_t dir_update;     /* bpred direction update info */
  int stack_recover_idx;                /* branch predictor RSB index */
  unsigned int ptrace_seq;              /* print trace sequence id */
  tick_t timestamp;			/* time when inst entered queue */
  int in1, in2, in3, out1, out2;	/* input/output register names */
  md_addr_t target_addr;		/* target address (branch instructions only) */
  int mem_size;				/* size of memory operation */
};

/* A rename table entry vector maps a logical register to a creator in the ROB and
 * a specific output operand. */
struct rename_entry {
  struct ROB_entry *re;                 /* creator's ROB entry */
  int odep_num;                         /* specific output operand */
  struct rename_entry *prev;            /* prev creator */
  struct rename_entry *next;            /* next creator */
};

/******* ROB_entry data structure *******/

/* inst tag type, used to tag an operation instance in the ROB */
typedef unsigned int INST_TAG_TYPE;

/* inst sequence type, used to order instructions in the ready list, if
   this rolls over the ready list order temporarily will get messed up,
   but execution will continue and complete correctly */
typedef unsigned int INST_SEQ_TYPE;

/* non-zero if all register operands are ready, update with MAX_IDEPS */
#define OPERANDS_READY(RE)                                              \
  ((RE)->idep_ready[0] && (RE)->idep_ready[1] && (RE)->idep_ready[2])

/* a reorder buffer (ROB) entry. The ROB is an order circular queue, in which
   instructions are inserted in fetch (program) order, results are stored in
   the ROB buffers, and later when an ROB entry is the oldest entry in the
   machines, it and its instruction's value is retired to the architectural
   register file in program order. NOTE: The ROB and LSQ share the same
   structure, this is useful because loads and stores are split into two
   operations: an effective address add and a load/store, the add is inserted
   into the ROB and the load/store inserted into the LSQ. */
struct ROB_entry {
  md_inst_t IR;                 	/* instruction bits */
  enum md_opcode op;                    /* decoded instruction opcode */

  md_addr_t PC;				/* PC of instruction */
  md_addr_t NPC;			/* correct value of NPC */
  md_addr_t pred_NPC;       		/* predicted value of NPC */

  int in_LSQ;                           /* non-zero if op is in LSQ */
  int isq_index;			/* index to oracle data */
  int rob_index;			/* corresponding ea comp entry for LSQ entries */

  int slip;				/* cycle info */

  INST_TAG_TYPE tag;                    /* ROB slot tag, increment to squash operation */
  INST_SEQ_TYPE seq;                    /* inst sequence num, used to sort ready list */
  unsigned int ptrace_seq;              /* pipetrace sequence number */

  int dtlb_miss;			/* set if processing a DTLB miss */
  int timing_dep;			/* set if timing dependent computation */
  int ea_comp;                          /* non-zero if op is an addr comp */
  int match_in_LSQ;			/* set if store forwarding took place */
  int recover_inst;                     /* start of mis-speculation? */
  int stack_recover_idx;                /* non-speculative TOS for RSB pred */
  struct bpred_update_t dir_update;     /* bpred direction update info */
  int schedule_wait_time;               /* time remaining before can execute */

  /* instruction status */
  int queued;                           /* operands ready and queued */
  int issued;                           /* operation is/was executing */
  int completed;                        /* operation has completed execution */

  /* blind speculation recovery information */ 
  int blind_recover;                    /* set if blind speculation recovery is needed */

  /* memory values */
  union val_union mem_value;		/* output data of store / input data for load */
  int mem_size;                         /* size of memory value (known at decode) */

  /* output fault */
  enum md_fault_type fault;		/* fault, if any */

  /* output register values and dependency list */ 
  /* The output dependency list are used to limit the number of
     associative searches into the ROB when instructions complete
     and need to wake up dependent insts */
  int odep_name[MAX_ODEPS];                    /* output names (DNA=unused) */
  struct rename_entry *odep_rename[MAX_ODEPS]; /* output rename table entry */
  struct RE_link *odep_list[MAX_ODEPS];        /* chains to consuming operations */
  union val_union odep_value[MAX_ODEPS];       /* output value */
  int odep_valid[MAX_ODEPS];                   /* output value is valid */

  /* input dependent links */
  /* The output chains rooted above use these fields to mark input
   * operands as ready.  These only track register dependencies. (see
   * lsq_refressh() for details on enforcing memory dependencies) */
  int idep_name[MAX_IDEPS];              /* input names (DNA=unused) */
  int idep_ready[MAX_IDEPS];             /* input operand ready? */
  union val_union idep_value[MAX_IDEPS]; /* input value */
};

/******* RE_link data structure *******/

/* A link to a reorder buffer entry link.  It links elements of a ROB
 * entry list and is used in the ready queue, event queue, and the
 * output dependency lists. 
 * 
 * Each RE_link node contains a pointer to the ROB entry it references
 * along with a tag. The RE_LINK is only valid if the tag matches the
 * ROB entry's tag. This strategy allows entries in the ROB to be
 * squashed and reused without updating the lists that point to it.
 * This significantly improves the performance of squash events.
 * 
 * Here is an example.  An instruction is added to the event queue.
 * When it is added, assume that the instruction's ROB entry has a
 * tag of 5.  When the event queue entry is created, the tag will be
 * assigned 5 and re will point to the ROB entry.  Before the event
 * completes, assume a misprediction occurs and squashes this
 * instruction.  In the function rob_recover, the instruction is
 * squashed by incrementing the tag.  Now, the tag in the ROB entry
 * will have the value of 6.  When the event finishes, the tags are
 * compared.  Since the event queue has a tag of 5 and the re has
 * a tag of 6, the event is not valid and correctly ignored.  An
 * alternative slower approach would be to scan the event queue during
 * the recovery and manually remove all invalid elements.  This is
 * much slower, considering this would also have to be done for the
 * ready queue and the output dependency lists for each valid
 * instruction.
 */
struct RE_link {
  struct RE_link *next;                 /* next entry in list */
  struct ROB_entry *re;                 /* referenced ROB entry */
  INST_TAG_TYPE tag;                    /* inst instance sequence number */
  union {
    tick_t when;                        /* time stamp of entry (for eventq) */
    INST_SEQ_TYPE seq;                  /* inst sequence */
    int opnum;                          /* input/output operand number */
  } x;
};

/* total RE links allocated at program start */
#define MAX_RE_LINKS                    4096

/* initialize an RE link */
#define RELINK_INIT(REL, RE)                                            \
  ((REL).next = NULL, (REL).re = (RE), (REL).tag = (RE)->tag)

/* non-zero if RE link is NULL */
#define RELINK_IS_NULL(LINK)            ((LINK)->re == NULL)

/* an RE_link is valid if the tag in the RE_link entry
 * matches the tag in the corresponding ROB entry */
#define RELINK_VALID(LINK)              ((LINK)->tag == (LINK)->re->tag)

/* returns the pointer to the ROB entry */
#define RELINK_RE(LINK)                 ((LINK)->re)

/* get a new RE link record */
#define RELINK_NEW(DST, RE)                                             \
  { struct RE_link *n_link;                                             \
    if (!relink_free_list)                                              \
      panic("out of re links");                                         \
    n_link = relink_free_list;                                          \
    relink_free_list = relink_free_list->next;                          \
    n_link->next = NULL;                                                \
    n_link->re = (RE); n_link->tag = n_link->re->tag;                   \
    (DST) = n_link;                                                     \
  }

/* frees an individual RE link record */
#define RELINK_FREE(LINK)                                               \
  {  struct RE_link *f_link = (LINK);                                   \
     f_link->re = NULL; f_link->tag = 0;                                \
     f_link->next = relink_free_list;                                   \
     relink_free_list = f_link;                                         \
  }

/* frees an entire RE link list */
#define RELINK_FREE_LIST(LINK)                                          \
  {  struct RE_link *fl_link, *fl_link_next;                            \
     for (fl_link=(LINK); fl_link; fl_link=fl_link_next)                \
       {                                                                \
         fl_link_next = fl_link->next;                                  \
         RELINK_FREE(fl_link);                                          \
       }                                                                \
  }

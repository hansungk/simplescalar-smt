/* mase-opts.h - sim-mase options and statistics */

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

/* --- SIMULATOR OPTIONS --- */

/* --- instruction limit --- */

/* maximum number of inst's to execute */
extern unsigned int max_insts;

/* --- trace options --- */

/* number of insts skipped before timing starts */
extern int fastfwd_count;

/* pipeline trace range and output filename */
extern int ptrace_nelt;
extern char *ptrace_opts[2];

/* --- fetch options --- */

/* speed of front-end of machine relative to execution core */
extern int fetch_speed;

/* fetch instructions from multiple cache lines in same cycle */
extern int fetch_mult_lines;

/* re-introducr optimisitic misfetch recovery for compatability */
extern int mf_compat;

/* --- IFQ options --- */

/* instruction fetch queue size (in insts) */
extern int IFQ_size;

/* instruction fetch queue delay (in cycles) */
extern int IFQ_delay;

/* --- branch predictor options --- */

/* branch predictor type {nottaken|taken|perfect|bimod|2lev} */
extern char *pred_type;

/* perfect prediction enabled */
extern int pred_perfect;

/* bimodal predictor config (<table_size>) */
extern int bimod_nelt;
extern int bimod_config[1];

/* 2-level predictor config (<l1size> <l2size> <hist_size> <xor>) */
extern int twolev_nelt;
extern int twolev_config[4];

/* combining predictor config (<meta_table_size> */
extern int comb_nelt;
extern int comb_config[1];

/* return address stack (RAS) size */
extern int ras_size;

/* BTB predictor config (<num_sets> <associativity>) */
extern int btb_nelt;
extern int btb_config[2];

/* speculative bpred-update enabled */
enum bpred_update_stage { spec_ID, spec_WB, spec_CT };
extern char *bpred_spec_opt;
extern enum bpred_update_stage bpred_spec_update;

/* disallow branch misprediction recoveries in the shadow of a misprediction */
extern int recover_in_shadow;

/* --- machine width options --- */

/* instruction decode B/W (insts/cycle) */
extern int decode_width;

/* instruction issue B/W (insts/cycle) */
extern int issue_width;

/* instruction commit B/W (insts/cycle) */
extern int commit_width;

/* --- issue options --- */

/* run pipeline with in-order issue */
extern int inorder_issue;

/* issue instructions down control mispeculated paths */
extern int include_spec;

/* --- register scheduler options --- */

/* register update unit (ROB) size */
extern int ROB_size;

/* number of reservation stations */
extern int RS_size;

/* extra pipe stages for the scheduler/regfile */
extern int schedule_delay;

extern int scheduler_replay;

/* --- memory scheduler options --- */

/* load/store queue (LSQ) size */
extern int LSQ_size;

/* perfect memory disambiguation */   
extern int perfect_disambig;

/* use blind speculation for loads */
extern int use_blind_spec;

/* --- cache options --- */

/* l1 data cache config, i.e., {<config>|none} */
extern char *cache_dl1_opt;

/* l1 data cache hit latency (in cycles) */
extern int cache_dl1_lat;

/* l2 data cache config, i.e., {<config>|none} */
extern char *cache_dl2_opt;

/* l2 data cache hit latency (in cycles) */
extern int cache_dl2_lat;

/* l1 instruction cache config, i.e., {<config>|dl1|dl2|none} */
extern char *cache_il1_opt;

/* l1 instruction cache hit latency (in cycles) */
extern int cache_il1_lat;

/* l2 instruction cache config, i.e., {<config>|dl1|dl2|none} */
extern char *cache_il2_opt;

/* l2 instruction cache hit latency (in cycles) */
extern int cache_il2_lat;

/* flush caches on system calls */
extern int flush_on_syscalls;

/* convert 64-bit inst addresses to 32-bit inst equivalents */
extern int compress_icache_addrs;

/* --- memory options --- */

/* memory access latency (<first_chunk> <inter_chunk>) */
extern int mem_nelt;
extern int mem_lat[2];

/* memory access bus width (in bytes) */
extern int mem_bus_width;

/* --- TLB options --- */

/* instruction TLB config, i.e., {<config>|none} */
extern char *itlb_opt;

/* data TLB config, i.e., {<config>|none} */
extern char *dtlb_opt;

/* inst/data TLB miss latency (in cycles) */
extern int tlb_miss_lat;

/* --- resource configuration options --- */

/* resource pool indices, NOTE: update these if you change FU_CONFIG */
#define FU_IALU_INDEX                   0
#define FU_IMULT_INDEX                  1
#define FU_MEMPORT_INDEX                2
#define FU_FPALU_INDEX                  3
#define FU_FPMULT_INDEX                 4
#define FU_NUM_INDICES                  5

/* functional unit resource pool */
extern struct res_pool *fu_pool;

/* resource pool */
extern struct res_desc fu_config[FU_NUM_INDICES];

/* --- checker options --- */

/* inject random checker errors */
extern int inject_errors;

/* print out checker errors */
extern int print_chk_err;

/* stop on checker errors */
extern int stop_on_chk_err;

/* checker threshold - warns user if error % exceeds threshold */
extern float chk_threshold;

/* --- SIMULATOR STATS --- */

/* --- slip stats --- */

/* SLIP variable */
extern counter_t sim_slip;

/* --- instruction counts --- */

/* sim_num_insn is declared in main.c */

/* total number of instructions executed by oracle (non-spec) */
extern counter_t sim_oracle_insn;

/* total number of memory references committed */
extern counter_t sim_num_refs;

/* total number of loads committed */
extern counter_t sim_num_loads;

/* total number of branches committed */
extern counter_t sim_num_branches;

/* total number of instructions executed */
extern counter_t sim_total_insn;

/* total number of memory references executed */
extern counter_t sim_total_refs;

/* total number of loads executed */
extern counter_t sim_total_loads;

/* total number of branches executed */
extern counter_t sim_total_branches;

/* --- simulation timing stats --- */

/* sim_elapsed_time is declared in main.c */

/* --- performance stats --- */

/* cycle counter */
extern tick_t sim_cycle;

/* --- occupancy stats --- */

extern counter_t IFQ_count;		/* cumulative IFQ occupancy */
extern counter_t IFQ_fcount;		/* cumulative IFQ full count */
extern counter_t ISQ_count;		/* cumulative ISQ occupancy */
extern counter_t ISQ_fcount;		/* cumulative ISQ full count */
extern counter_t RS_count;		/* cumulative RS occupancy */
extern counter_t RS_fcount;		/* cumulative RS full count */
extern counter_t ROB_count;		/* cumulative ROB occupancy */
extern counter_t ROB_fcount;		/* cumulative ROB full count */
extern counter_t LSQ_count;		/* cumulative LSQ occupancy */
extern counter_t LSQ_fcount;		/* cumulative LSQ full count */

/* --- checker stats --- */

/* timing dependent instruction count */
extern counter_t timing_dep_insn;

/* checker errors */
extern counter_t checker_errors;

/* --- latency misprediction (scheduler replay) stats --- */

extern counter_t num_replays;

/* --- branch predictor stats --- */

extern counter_t misfetch_count;
extern counter_t misfetch_only_count;
extern counter_t recovery_count;

/* --- blind speculation stats --- */

extern counter_t blind_spec_flushes;
extern counter_t blind_spec_all_flushes;

/* --- fetch stall stats --- */
enum fetch_stall_t {
  fetch_no_stall,       /* no stall */ 
  fetch_cache_miss,     /* cache miss */
  fetch_align,          /* alignment */
  fetch_syscall,        /* syscall (draining) */
  fetch_unknown,        /* unknown target */
  fetch_branches,       /* max branches */
  fetch_ifq_full,       /* IFQ is full */
  fetch_other,		/* other */
  fetch_NUM
};
extern struct stat_stat_t *fetch_stall;

/* --- dispatch stall stats --- */
enum dp_stall_t {
  dp_no_stall,          /* no stall */ 
  dp_ifq_empty,         /* IFQ is empty */
  dp_rs_full,           /* RS is full */
  dp_lsq_full,          /* LSQ is full */
  dp_rob_full,          /* ROB is full */
  dp_other,		/* other */
  dp_NUM
};
extern struct stat_stat_t *dp_stall;

/* --- PC-based stats --- */
#define MAX_PCSTAT_VARS 8
extern int pcstat_nelt;
extern int spec_pcstat_nelt;
extern char *pcstat_vars[MAX_PCSTAT_VARS];
extern char *spec_pcstat_vars[MAX_PCSTAT_VARS];
extern struct stat_stat_t *pcstat_stats[MAX_PCSTAT_VARS];
extern struct stat_stat_t *spec_pcstat_stats[MAX_PCSTAT_VARS];
extern counter_t pcstat_lastvals[MAX_PCSTAT_VARS];
extern counter_t spec_pcstat_lastvals[MAX_PCSTAT_VARS];
extern struct stat_stat_t *pcstat_sdists[MAX_PCSTAT_VARS];
extern struct stat_stat_t *spec_pcstat_sdists[MAX_PCSTAT_VARS];

/* wedge all stat values into a counter_t */
#define STATVAL(STAT)							\
  ((STAT)->sc == sc_int							\
   ? (counter_t)*((STAT)->variant.for_int.var)				\
   : ((STAT)->sc == sc_uint						\
      ? (counter_t)*((STAT)->variant.for_uint.var)			\
      : ((STAT)->sc == sc_counter					\
	 ? *((STAT)->variant.for_counter.var)				\
	 : (panic("bad stat class"), 0))))

/* register simulator-specific options */
void
sim_reg_options(struct opt_odb_t *odb);

/* check simulator-specific option values */
void
sim_check_options(struct opt_odb_t *odb,        /* options database */
		  int argc, char **argv);       /* command line arguments */

/* print simulator-specific configuration information */
void
sim_aux_config(FILE *stream);           /* output stream */

/* register simulator-specific statistics */
void
sim_reg_stats(struct stat_sdb_t *sdb);  /* stats database */

/* dump simulator-specific auxiliary simulator statistics */
void
sim_aux_stats(FILE *stream);            /* output stream */

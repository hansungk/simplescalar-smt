/* mase-opts.c - sim-mase options and statistics */

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

/* --- SIMULATOR OPTIONS --- */

/* --- instruction limit --- */

/* maximum number of inst's to execute */
unsigned int max_insts;

/* --- trace options --- */

/* number of insts skipped before timing starts */
int fastfwd_count;

/* pipeline trace range and output filename */
int ptrace_nelt = 0;
char *ptrace_opts[2];

/* --- fetch options --- */

/* speed of front-end of machine relative to execution core */
int fetch_speed;

/* fetch instructions from multiple cache lines in same cycle */
int fetch_mult_lines;

/* re-introducr optimisitic misfetch recovery for compatability */
int mf_compat;

/* --- IFQ options --- */

/* instruction fetch queue size (in insts) */
int IFQ_size;

/* instruction fetch queue delay (in cycles) */
int IFQ_delay;

/* --- branch predictor options --- */

/* branch predictor type {nottaken|taken|perfect|bimod|2lev} */
char *pred_type;

/* perfect prediction enabled */
int pred_perfect = FALSE;

/* bimodal predictor config (<table_size>) */
int bimod_nelt = 1;
int bimod_config[1] =
  { /* bimod tbl size */2048 };

/* 2-level predictor config (<l1size> <l2size> <hist_size> <xor>) */
int twolev_nelt = 4;
int twolev_config[4] =
  { /* l1size */1, /* l2size */1024, /* hist */8, /* xor */FALSE};

/* combining predictor config (<meta_table_size> */
int comb_nelt = 1;
int comb_config[1] =
  { /* meta_table_size */1024 };

/* return address stack (RAS) size */
int ras_size = 8;

/* BTB predictor config (<num_sets> <associativity>) */
int btb_nelt = 2;
int btb_config[2] =
  { /* nsets */512, /* assoc */4 };

/* speculative bpred-update enabled */
char *bpred_spec_opt;
enum bpred_update_stage bpred_spec_update;

/* disallow branch misprediction recoveries in the shadow of a misprediction */
int recover_in_shadow = TRUE;

/* --- machine width options --- */

/* instruction decode B/W (insts/cycle) */
int decode_width;

/* instruction issue B/W (insts/cycle) */
int issue_width;

/* instruction commit B/W (insts/cycle) */
int commit_width;

/* --- issue options --- */

/* run pipeline with in-order issue */
int inorder_issue;

/* issue instructions down control mispeculated paths */
int include_spec = TRUE;

/* --- register scheduler options --- */

/* register update unit (ROB) size */
int ROB_size;

/* number of reservation stations */
int RS_size;

/* extra pipe stages for the scheduler/regfile */
int schedule_delay = 0;

/* turn on scheduler replay */
int scheduler_replay = 0;


/* --- memory scheduler options --- */

/* load/store queue (LSQ) size */
int LSQ_size = 4;

/* perfect memory disambiguation */   
int perfect_disambig = FALSE;

/* use blind load speculation for loads */
int use_blind_spec = FALSE;

/* --- cache options --- */

/* l1 data cache config, i.e., {<config>|none} */
char *cache_dl1_opt;

/* l1 data cache hit latency (in cycles) */
int cache_dl1_lat;

/* l2 data cache config, i.e., {<config>|none} */
char *cache_dl2_opt;

/* l2 data cache hit latency (in cycles) */
int cache_dl2_lat;

/* l1 instruction cache config, i.e., {<config>|dl1|dl2|none} */
char *cache_il1_opt;

/* l1 instruction cache hit latency (in cycles) */
int cache_il1_lat;

/* l2 instruction cache config, i.e., {<config>|dl1|dl2|none} */
char *cache_il2_opt;

/* l2 instruction cache hit latency (in cycles) */
int cache_il2_lat;

/* flush caches on system calls */
int flush_on_syscalls;

/* convert 64-bit inst addresses to 32-bit inst equivalents */
int compress_icache_addrs;

/* --- memory options --- */

/* memory access latency (<first_chunk> <inter_chunk>) */
int mem_nelt = 2;
int mem_lat[2] =
  { /* lat to first chunk */18, /* lat between remaining chunks */2 };

/* memory access bus width (in bytes) */
int mem_bus_width;

/* --- TLB options --- */

/* instruction TLB config, i.e., {<config>|none} */
char *itlb_opt;

/* data TLB config, i.e., {<config>|none} */
char *dtlb_opt;

/* inst/data TLB miss latency (in cycles) */
int tlb_miss_lat;

/* --- resource configuration options --- */

/* functional unit resource pool */
struct res_pool *fu_pool = NULL;

/* resource pool definition
 * Initialization is not done here, you change the values using the
 * appropriate variables below.  You can also override the defaults,
 * including the latencies via the command line or configuration file.
 * NOTE: update FU_*_INDEX defs if you change this */
struct res_desc fu_config[] = {
  {
    "integer-ALU", 0, 0,
    { { IntALU, 0, 0 } }
  },
  {
    "integer-MULT/DIV", 0, 0,
    { { IntMULT, 0, 0 }, { IntDIV, 0, 0 } }
  },
  {
    "memory-port", 0, 0,
    { { RdPort, 0, 0 }, { WrPort, 0, 0 } }
  },
  {
    "FP-adder", 0, 0,
    { { FloatADD, 0, 0 }, { FloatCMP, 0, 0 }, { FloatCVT, 0, 0 } }
  },
  {
    "FP-MULT/DIV", 0, 0,
    { { FloatMULT, 0, 0 }, { FloatDIV, 0, 0 }, { FloatSQRT, 0, 0 } }
  },
};

/* functional units available */
int res_ialu    = 4;	/* integer ALU's available */
int res_imult   = 1;	/* integer multiplier/dividers available */
int res_memport = 2;	/* memory system ports available (to CPU) */
int res_fpalu   = 4;	/* floating point ALU's available */
int res_fpmult  = 1;	/* floating point multiplier/dividers available */

/* latency variables */
int res_ialu_oplat  = 1;
int res_ialu_islat  = 1;
int res_imul_oplat  = 7;
int res_imul_islat  = 1;
int res_idiv_oplat  = 12;
int res_idiv_islat  = 9;
int res_read_oplat  = 1;
int res_read_islat  = 1;
int res_write_oplat = 1;
int res_write_islat = 1;
int res_fadd_oplat  = 4;
int res_fadd_islat  = 1;
int res_fcmp_oplat  = 4;
int res_fcmp_islat  = 1;
int res_fcvt_oplat  = 3;
int res_fcvt_islat  = 1;
int res_fmul_oplat  = 4;
int res_fmul_islat  = 1;
int res_fdiv_oplat  = 12;
int res_fdiv_islat  = 9;
int res_fsqrt_oplat = 18;
int res_fsqrt_islat = 15;

/* --- checker options --- */

/* inject random checker errors */
int inject_errors = FALSE;

/* print out checker errors */
int print_chk_err = FALSE;

/* stop on checker errors */
int stop_on_chk_err = FALSE;

/* checker threshold - warns user if error % exceeds threshold */
float chk_threshold = 1.0;

/* --- SIMULATOR STATS --- */

/* --- slip stats --- */

/* SLIP variable */
counter_t sim_slip = 0;

/* --- instruction counts --- */

/* sim_num_insn is declared in main.c */

/* total number of instructions executed by orcale (non-spec only) */
counter_t sim_oracle_insn = 0;

/* total number of memory references committed */
counter_t sim_num_refs = 0;

/* total number of loads committed */
counter_t sim_num_loads = 0;

/* total number of branches committed */
counter_t sim_num_branches = 0;

/* total number of instructions executed */
counter_t sim_total_insn = 0;

/* total number of memory references executed */
counter_t sim_total_refs = 0;

/* total number of loads executed */
counter_t sim_total_loads = 0;

/* total number of branches executed */
counter_t sim_total_branches = 0;

/* --- simulation timing stats --- */

/* sim_elapsed_time is declared in main.c */

/* --- performance stats --- */

/* cycle counter */
tick_t sim_cycle = 0;

/* --- occupancy stats --- */

counter_t IFQ_count;		/* cumulative IFQ occupancy */
counter_t IFQ_fcount;		/* cumulative IFQ full count */
counter_t ISQ_count;		/* cumulative ISQ occupancy */
counter_t ISQ_fcount;		/* cumulative ISQ full count */
counter_t RS_count;		/* cumulative RS occupancy */
counter_t RS_fcount;		/* cumulative RS full count */
counter_t ROB_count;		/* cumulative ROB occupancy */
counter_t ROB_fcount;		/* cumulative ROB full count */
counter_t LSQ_count;		/* cumulative LSQ occupancy */
counter_t LSQ_fcount;		/* cumulative LSQ full count */

/* --- checker stats --- */

/* timing dependent instruction count */
counter_t timing_dep_insn = 0;

/* checker errors */
counter_t checker_errors = 0;

/* --- latency misprediction (scheduler replay) stats --- */
counter_t num_replays = 0;


/* --- branch predictor stats --- */

counter_t misfetch_count = 0;
counter_t misfetch_only_count = 0;
counter_t recovery_count = 0;

/* --- blind speculation stats --- */

counter_t blind_spec_flushes = 0;
counter_t blind_spec_all_flushes = 0;

/* --- fetch stall stats --- */
struct stat_stat_t *fetch_stall = NULL;
static char *fetch_stall_str[fetch_NUM] = {
  "no stall",
  "cache miss",
  "alignment",
  "syscall",
  "unknown target",
  "max branches",
  "IFQ is full",
  "other"
};

/* --- dispatch stall stats --- */
struct stat_stat_t *dp_stall = NULL;
static char *dp_stall_str[dp_NUM] = {
  "no stall",
  "IFQ is empty",
  "RS is full",
  "LSQ is full",
  "ROB is full",
  "other"
};

/* --- PC-based stats --- */

int pcstat_nelt = 0;
int spec_pcstat_nelt = 0;
char *pcstat_vars[MAX_PCSTAT_VARS];
char *spec_pcstat_vars[MAX_PCSTAT_VARS];
struct stat_stat_t *pcstat_stats[MAX_PCSTAT_VARS];
struct stat_stat_t *spec_pcstat_stats[MAX_PCSTAT_VARS];
counter_t pcstat_lastvals[MAX_PCSTAT_VARS];
counter_t spec_pcstat_lastvals[MAX_PCSTAT_VARS];
struct stat_stat_t *pcstat_sdists[MAX_PCSTAT_VARS];
struct stat_stat_t *spec_pcstat_sdists[MAX_PCSTAT_VARS];

/* register simulator-specific options */
void
sim_reg_options(struct opt_odb_t *odb)
{
  opt_reg_header(odb, "sim-mase: A detailed micro-architectural simulation environment.\n");

  /* --- instruction limit --- */

  opt_reg_uint(odb, "-max:inst", "maximum number of inst's to execute",
	       &max_insts, /* default */0,
	       /* print */TRUE, /* format */NULL);

  /* --- trace options --- */

  opt_reg_int(odb, "-fastfwd", "number of insts skipped before timing starts",
	      &fastfwd_count, /* default */0,
	      /* print */TRUE, /* format */NULL);

  opt_reg_string_list(odb, "-ptrace",
	      "generate pipetrace, i.e., <fname|stdout|stderr> <range>",
	      ptrace_opts, /* arr_sz */2, &ptrace_nelt, /* default */NULL,
	      /* !print */FALSE, /* format */NULL, /* !accrue */FALSE);

  /* --- fetch options --- */

  opt_reg_int(odb, "-fetch:speed",
	      "speed of front-end of machine relative to execution core",
	      &fetch_speed, /* default */1,
	      /* print */TRUE, /* format */NULL);

  opt_reg_flag(odb, "-fetch:mult_lines",
	       "allows instructions to be fetched from multiple cache lines in same cycle",
	       &fetch_mult_lines, /* default */FALSE,
	       /* print */TRUE, /* format */NULL);

  opt_reg_flag(odb, "-fetch:mf_compat",
	       "optimistic misfetch recovery (compatability with SimpleScalar)",
	       &mf_compat, /* default */FALSE,
	       /* print */TRUE, /* format */NULL);

  /* --- IFQ options --- */

  opt_reg_int(odb, "-ifq:size", "instruction fetch queue size (in insts)",
	      &IFQ_size, /* default */16,
	      /* print */TRUE, /* format */NULL);

  opt_reg_int(odb, "-ifq:delay", "instruction fetch queue delay (in cycles)",
	      &IFQ_delay, /* default */3,
	      /* print */TRUE, /* format */NULL);

  /* --- branch predictor options --- */

  opt_reg_string(odb, "-bpred",
		 "branch predictor type {nottaken|taken|perfect|bimod|2lev|comb}",
                 &pred_type, /* default */"bimod",
                 /* print */TRUE, /* format */NULL);

  opt_reg_int_list(odb, "-bpred:bimod",
		   "bimodal predictor config (<table size>)",
		   bimod_config, bimod_nelt, &bimod_nelt,
		   /* default */bimod_config,
		   /* print */TRUE, /* format */NULL, /* !accrue */FALSE);

  opt_reg_int_list(odb, "-bpred:2lev",
                   "2-level predictor config "
		   "(<l1size> <l2size> <hist_size> <xor>)",
                   twolev_config, twolev_nelt, &twolev_nelt,
		   /* default */twolev_config,
                   /* print */TRUE, /* format */NULL, /* !accrue */FALSE);

  opt_reg_int_list(odb, "-bpred:comb",
		   "combining predictor config (<meta_table_size>)",
		   comb_config, comb_nelt, &comb_nelt,
		   /* default */comb_config,
		   /* print */TRUE, /* format */NULL, /* !accrue */FALSE);

  opt_reg_int(odb, "-bpred:ras",
              "return address stack size (0 for no return stack)",
              &ras_size, /* default */ras_size,
              /* print */TRUE, /* format */NULL);

  opt_reg_int_list(odb, "-bpred:btb",
		   "BTB config (<num_sets> <associativity>)",
		   btb_config, btb_nelt, &btb_nelt,
		   /* default */btb_config,
		   /* print */TRUE, /* format */NULL, /* !accrue */FALSE);

  opt_reg_string(odb, "-bpred:spec_update",
		 "speculative predictors update in {ID|WB} (default non-spec)",
		 &bpred_spec_opt, /* default */NULL,
		 /* print */TRUE, /* format */NULL);

  opt_reg_flag(odb, "-bpred:rcvr_shadow",
	       "allows recovery in the shadow of a misprediction",
	       &recover_in_shadow, /* default */TRUE, /* print */TRUE, NULL);

  /* --- decode options --- */

  opt_reg_int(odb, "-decode:width",
	      "instruction decode B/W (insts/cycle)",
	      &decode_width, /* default */4,
	      /* print */TRUE, /* format */NULL);

  /* --- issue options --- */

  opt_reg_int(odb, "-issue:width",
	      "instruction issue B/W (insts/cycle)",
	      &issue_width, /* default */4,
	      /* print */TRUE, /* format */NULL);

  opt_reg_flag(odb, "-issue:inorder", "run pipeline with in-order issue",
	       &inorder_issue, /* default */FALSE,
	       /* print */TRUE, /* format */NULL);

  opt_reg_flag(odb, "-issue:wrongpath",
	       "issue instructions down wrong paths caused by control mispredictions",
	       &include_spec, /* default */TRUE,
	       /* print */TRUE, /* format */NULL);

  opt_reg_int(odb, "-commit:width",
	      "instruction commit B/W (insts/cycle)",
	      &commit_width, /* default */4,
	      /* print */TRUE, /* format */NULL);

  /* --- register scheduler options --- */

  opt_reg_int(odb, "-rob:size",
	      "reorder buffer (ROB) size",
	      &ROB_size, /* default */16,
	      /* print */TRUE, /* format */NULL);

  opt_reg_int(odb, "-rs:size",
	      "number of reservation stations",
	      &RS_size, /* default */ROB_size,
	      /* print */TRUE, /* format */NULL);

  opt_reg_int(odb, "-scheduler:delay",
	      "cycles to perform regfile access",
	      &schedule_delay, /*default */0,
	      /* print */TRUE, /* format */NULL);

  opt_reg_flag(odb, "-scheduler:replay",
	       "use scheduler replay",
	       &scheduler_replay, /* default */FALSE, /* print */TRUE, NULL);


  /* --- memory scheduler options --- */

  opt_reg_int(odb, "-lsq:size",
	      "load/store queue (LSQ) size",
	      &LSQ_size, /* default */8,
	      /* print */TRUE, /* format */NULL);

  opt_reg_flag(odb, "-lsq:perfect",
               "perfect memory disambiguation",
               &perfect_disambig, /* default */FALSE, /* print */TRUE, NULL);
	      
  opt_reg_flag(odb, "-lsq:blind", "use blind speculation for loads",
	       &use_blind_spec, /* default */FALSE,
	       /* print */TRUE, /* format */NULL);

  /* --- cache options --- */

  opt_reg_string(odb, "-cache:dl1",
		 "l1 data cache config, i.e., {<config>|none}",
		 &cache_dl1_opt, "dl1:128:32:4:l",
		 /* print */TRUE, NULL);

  opt_reg_int(odb, "-cache:dl1lat",
	      "l1 data cache hit latency (in cycles)",
	      &cache_dl1_lat, /* default */1,
	      /* print */TRUE, /* format */NULL);

  opt_reg_string(odb, "-cache:dl2",
		 "l2 data cache config, i.e., {<config>|none}",
		 &cache_dl2_opt, "ul2:1024:64:4:l",
		 /* print */TRUE, NULL);

  opt_reg_int(odb, "-cache:dl2lat",
	      "l2 data cache hit latency (in cycles)",
	      &cache_dl2_lat, /* default */6,
	      /* print */TRUE, /* format */NULL);

  opt_reg_string(odb, "-cache:il1",
		 "l1 inst cache config, i.e., {<config>|dl1|dl2|none}",
		 &cache_il1_opt, "il1:512:32:1:l",
		 /* print */TRUE, NULL);

  opt_reg_int(odb, "-cache:il1lat",
	      "l1 instruction cache hit latency (in cycles)",
	      &cache_il1_lat, /* default */1,
	      /* print */TRUE, /* format */NULL);

  opt_reg_string(odb, "-cache:il2",
		 "l2 instruction cache config, i.e., {<config>|dl2|none}",
		 &cache_il2_opt, "dl2",
		 /* print */TRUE, NULL);

  opt_reg_int(odb, "-cache:il2lat",
	      "l2 instruction cache hit latency (in cycles)",
	      &cache_il2_lat, /* default */6,
	      /* print */TRUE, /* format */NULL);

  opt_reg_flag(odb, "-cache:flush", "flush caches on system calls",
	       &flush_on_syscalls, /* default */FALSE, /* print */TRUE, NULL);

  opt_reg_flag(odb, "-cache:icompress",
	       "convert 64-bit inst addresses to 32-bit inst equivalents",
	       &compress_icache_addrs, /* default */FALSE,
	       /* print */TRUE, NULL);

  /* --- memory options --- */

  opt_reg_int_list(odb, "-mem:lat",
		   "memory access latency (<first_chunk> <inter_chunk>)",
		   mem_lat, mem_nelt, &mem_nelt, mem_lat,
		   /* print */TRUE, /* format */NULL, /* !accrue */FALSE);

  opt_reg_int(odb, "-mem:width", "memory access bus width (in bytes)",
	      &mem_bus_width, /* default */8,
	      /* print */TRUE, /* format */NULL);

  /* --- TLB options --- */

  opt_reg_string(odb, "-tlb:itlb",
		 "instruction TLB config, i.e., {<config>|none}",
		 &itlb_opt, "itlb:16:4096:4:l", /* print */TRUE, NULL);

  opt_reg_string(odb, "-tlb:dtlb",
		 "data TLB config, i.e., {<config>|none}",
		 &dtlb_opt, "dtlb:32:4096:4:l", /* print */TRUE, NULL);

  opt_reg_int(odb, "-tlb:lat",
	      "inst/data TLB miss latency (in cycles)",
	      &tlb_miss_lat, /* default */30,
	      /* print */TRUE, /* format */NULL);

  /* --- resource configuration options --- */

  opt_reg_int(odb, "-res:ialu",
	      "total number of integer ALU's available",
	      &res_ialu, /* default */res_ialu,
	      /* print */TRUE, /* format */NULL);

  opt_reg_int(odb, "-res:imult",
	      "total number of integer multiplier/dividers available",
	      &res_imult, /* default */res_imult,
	      /* print */TRUE, /* format */NULL);

  opt_reg_int(odb, "-res:memport",
	      "total number of memory system ports available (to CPU)",
	      &res_memport, /* default */res_memport,
	      /* print */TRUE, /* format */NULL);

  opt_reg_int(odb, "-res:fpalu",
	      "total number of floating point ALU's available",
	      &res_fpalu, /* default */res_fpalu,
	      /* print */TRUE, /* format */NULL);

  opt_reg_int(odb, "-res:fpmult",
	      "total number of floating point multiplier/dividers available",
	      &res_fpmult, /* default */res_fpmult,
	      /* print */TRUE, /* format */NULL);

  opt_reg_int(odb, "-res:ialu_oplat",
	      "latency of integer ALU operations",
	      &res_ialu_oplat, /* default */res_ialu_oplat,
	      /* print */TRUE, /* format */NULL);

  opt_reg_int(odb, "-res:ialu_islat",
	      "issue latency of integer ALU operations",
	      &res_ialu_islat, /* default */res_ialu_islat,
	      /* print */TRUE, /* format */NULL);

  opt_reg_int(odb, "-res:imul_oplat",
	      "latency of integer MUL operations",
	      &res_imul_oplat, /* default */res_imul_oplat,
	      /* print */TRUE, /* format */NULL);

  opt_reg_int(odb, "-res:imul_islat",
	      "issue latency of integer MUL operations",
	      &res_imul_islat, /* default */res_imul_islat,
	      /* print */TRUE, /* format */NULL);

  opt_reg_int(odb, "-res:idiv_oplat",
	      "latency of integer DIV operations",
	      &res_idiv_oplat, /* default */res_idiv_oplat,
	      /* print */TRUE, /* format */NULL);

  opt_reg_int(odb, "-res:idiv_islat",
	      "issue latency of integer DIV operations",
	      &res_idiv_islat, /* default */res_idiv_islat,
	      /* print */TRUE, /* format */NULL);

  opt_reg_int(odb, "-res:read_oplat",
	      "latency of read port",
	      &res_read_oplat, /* default */res_read_oplat,
	      /* print */TRUE, /* format */NULL);

  opt_reg_int(odb, "-res:read_islat",
	      "issue latency of read port",
	      &res_read_islat, /* default */res_read_islat,
	      /* print */TRUE, /* format */NULL);

  opt_reg_int(odb, "-res:write_oplat",
	      "latency of write port",
	      &res_write_oplat, /* default */res_write_oplat,
	      /* print */TRUE, /* format */NULL);

  opt_reg_int(odb, "-res:write_islat",
	      "issue latency of write port",
	      &res_write_islat, /* default */res_write_islat,
	      /* print */TRUE, /* format */NULL);

  opt_reg_int(odb, "-res:fadd_oplat",
	      "latency of FP ADD operations",
	      &res_fadd_oplat, /* default */res_fadd_oplat,
	      /* print */TRUE, /* format */NULL);

  opt_reg_int(odb, "-res:fadd_islat",
	      "issue latency of FP ADD operations",
	      &res_fadd_islat, /* default */res_fadd_islat,
	      /* print */TRUE, /* format */NULL);

  opt_reg_int(odb, "-res:fcmp_oplat",
	      "latency of FP CMP operations",
	      &res_fcmp_oplat, /* default */res_fcmp_oplat,
	      /* print */TRUE, /* format */NULL);

  opt_reg_int(odb, "-res:fcmp_islat",
	      "issue latency of FP CMP operations",
	      &res_fcmp_islat, /* default */res_fcmp_islat,
	      /* print */TRUE, /* format */NULL);

  opt_reg_int(odb, "-res:fcvt_oplat",
	      "latency of FP CVT operations",
	      &res_fcvt_oplat, /* default */res_fcvt_oplat,
	      /* print */TRUE, /* format */NULL);

  opt_reg_int(odb, "-res:fcvt_islat",
	      "issue latency of FP CVT operations",
	      &res_fcvt_islat, /* default */res_fcvt_islat,
	      /* print */TRUE, /* format */NULL);

  opt_reg_int(odb, "-res:fmul_oplat",
	      "latency of FP MUL operations",
	      &res_fmul_oplat, /* default */res_fmul_oplat,
	      /* print */TRUE, /* format */NULL);

  opt_reg_int(odb, "-res:fmul_islat",
	      "issue latency of FP MUL operations",
	      &res_fmul_islat, /* default */res_fmul_islat,
	      /* print */TRUE, /* format */NULL);

  opt_reg_int(odb, "-res:fdiv_oplat",
	      "latency of FP DIV operations",
	      &res_fdiv_oplat, /* default */res_fdiv_oplat,
	      /* print */TRUE, /* format */NULL);

  opt_reg_int(odb, "-res:fdiv_islat",
	      "issue latency of FP DIV operations",
	      &res_fdiv_islat, /* default */res_fdiv_islat,
	      /* print */TRUE, /* format */NULL);

  opt_reg_int(odb, "-res:fsqrt_oplat",
	      "latency of FP SQRT operations",
	      &res_fsqrt_oplat, /* default */res_fsqrt_oplat,
	      /* print */TRUE, /* format */NULL);

  opt_reg_int(odb, "-res:fsqrt_islat",
	      "issue latency of FP SQRT operations",
	      &res_fsqrt_islat, /* default */res_fsqrt_islat,
	      /* print */TRUE, /* format */NULL);

  /* --- checker options --- */

  opt_reg_flag(odb, "-chk:inject_err",
      	       "randomly inject errors into checker",
	       &inject_errors, /* default */FALSE, /* print */TRUE, NULL);

  opt_reg_flag(odb, "-chk:print_err",
	       "print checker error messages",
	       &print_chk_err, /* default */FALSE, /* print */TRUE, NULL);

  opt_reg_flag(odb, "-chk:stop_on_err",
	       "stop on checker errors",
	       &stop_on_chk_err, /* default */FALSE, /* print */TRUE, NULL);

  opt_reg_float(odb, "-chk:threshold",
	      "prints warning if % of checker errors exceeds threshold",
	      &chk_threshold, /* default */chk_threshold,
	      /* print */TRUE, /* format */NULL);

  /* --- statistical options --- */

  opt_reg_string_list(odb, "-spec_pcstat",
		      "profile stat(s) against text addrs (oracle)",
		      spec_pcstat_vars, MAX_PCSTAT_VARS, &spec_pcstat_nelt, NULL,
		      /* !print */FALSE, /* format */NULL, /* accrue */TRUE);

  opt_reg_string_list(odb, "-pcstat",
		      "profile stat(s) against text addrs (commit)",
		      pcstat_vars, MAX_PCSTAT_VARS, &pcstat_nelt, NULL,
		      /* !print */FALSE, /* format */NULL, /* accrue */TRUE);
}

/* check simulator-specific option values */
void
sim_check_options(struct opt_odb_t *odb,        /* options database */
		  int argc, char **argv)        /* command line arguments */
{
  char name[128], c;
  int nsets, bsize, assoc;

  if (fastfwd_count < 0 || fastfwd_count >= 2147483647)
    fatal("bad fast forward count: %d", fastfwd_count);

  if (IFQ_size < 1)
    fatal("inst fetch queue size must be positive > 0");

  if (fetch_speed < 1)
    fatal("front-end speed must be positive and non-zero");

  if (!mystricmp(pred_type, "perfect"))
    {
      /* perfect predictor */
      pred = NULL;
      pred_perfect = TRUE;
    }
  else if (!mystricmp(pred_type, "taken"))
    {
      /*  predictor, not taken */
      pred = bpred_create(BPredTaken, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    }
  else if (!mystricmp(pred_type, "nottaken"))
    {
      /*  predictor, taken */
      pred = bpred_create(BPredNotTaken, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    }
  else if (!mystricmp(pred_type, "bimod"))
    {
      /* bimodal predictor, bpred_create() checks BTB_SIZE */
      if (bimod_nelt != 1)
	fatal("bad bimod predictor config (<table_size>)");
      if (btb_nelt != 2)
	fatal("bad btb config (<num_sets> <associativity>)");

      /* bimodal predictor, bpred_create() checks BTB_SIZE */
      pred = bpred_create(BPred2bit,
			  /* bimod table size */bimod_config[0],
			  /* 2lev l1 size */0,
			  /* 2lev l2 size */0,
			  /* meta table size */0,
			  /* history reg size */0,
			  /* history xor address */0,
			  /* btb sets */btb_config[0],
			  /* btb assoc */btb_config[1],
			  /* ret-addr stack size */ras_size);
    }
  else if (!mystricmp(pred_type, "2lev"))
    {
      /* 2-level adaptive predictor, bpred_create() checks args */
      if (twolev_nelt != 4)
	fatal("bad 2-level pred config (<l1size> <l2size> <hist_size> <xor>)");
      if (btb_nelt != 2)
	fatal("bad btb config (<num_sets> <associativity>)");

      pred = bpred_create(BPred2Level,
			  /* bimod table size */0,
			  /* 2lev l1 size */twolev_config[0],
			  /* 2lev l2 size */twolev_config[1],
			  /* meta table size */0,
			  /* history reg size */twolev_config[2],
			  /* history xor address */twolev_config[3],
			  /* btb sets */btb_config[0],
			  /* btb assoc */btb_config[1],
			  /* ret-addr stack size */ras_size);
    }
  else if (!mystricmp(pred_type, "comb"))
    {
      /* combining predictor, bpred_create() checks args */
      if (twolev_nelt != 4)
	fatal("bad 2-level pred config (<l1size> <l2size> <hist_size> <xor>)");
      if (bimod_nelt != 1)
	fatal("bad bimod predictor config (<table_size>)");
      if (comb_nelt != 1)
	fatal("bad combining predictor config (<meta_table_size>)");
      if (btb_nelt != 2)
	fatal("bad btb config (<num_sets> <associativity>)");

      pred = bpred_create(BPredComb,
			  /* bimod table size */bimod_config[0],
			  /* l1 size */twolev_config[0],
			  /* l2 size */twolev_config[1],
			  /* meta table size */comb_config[0],
			  /* history reg size */twolev_config[2],
			  /* history xor address */twolev_config[3],
			  /* btb sets */btb_config[0],
			  /* btb assoc */btb_config[1],
			  /* ret-addr stack size */ras_size);
    }
  else
    fatal("cannot parse predictor type `%s'", pred_type);

  if (!bpred_spec_opt)
    bpred_spec_update = spec_CT;
  else if (!mystricmp(bpred_spec_opt, "ID"))
    bpred_spec_update = spec_ID;
  else if (!mystricmp(bpred_spec_opt, "WB"))
    bpred_spec_update = spec_WB;
  else
    fatal("bad speculative update stage specifier, use {ID|WB}");

  if (decode_width < 1)
    fatal("decode width must be positive non-zero and a power of two");

  if (issue_width < 1)
    fatal("issue width must be positive non-zero and a power of two");

  if (commit_width < 1)
    fatal("commit width must be positive non-zero");

  if (ROB_size < 2)
    fatal("ROB size must be a positive number > 1 and a power of two");

  if (LSQ_size < 2)
    fatal("LSQ size must be a positive number > 1 and a power of two");

  /* use a level 1 D-cache? */
  if (!mystricmp(cache_dl1_opt, "none"))
    {
      cache_dl1 = NULL;

      /* the level 2 D-cache cannot be defined */
      if (strcmp(cache_dl2_opt, "none"))
	fatal("the l1 data cache must defined if the l2 cache is defined");
      cache_dl2 = NULL;
    }
  else /* dl1 is defined */
    {
      if (sscanf(cache_dl1_opt, "%[^:]:%d:%d:%d:%c",
		 name, &nsets, &bsize, &assoc, &c) != 5)
	fatal("bad l1 D-cache parms: <name>:<nsets>:<bsize>:<assoc>:<repl>");
      cache_dl1 = cache_create(name, nsets, bsize, /* balloc */FALSE,
			       /* usize */0, assoc, cache_char2policy(c),
			       dl1_access_fn, /* hit lat */cache_dl1_lat);

      /* is the level 2 D-cache defined? */
      if (!mystricmp(cache_dl2_opt, "none"))
	cache_dl2 = NULL;
      else
	{
	  if (sscanf(cache_dl2_opt, "%[^:]:%d:%d:%d:%c",
		     name, &nsets, &bsize, &assoc, &c) != 5)
	    fatal("bad l2 D-cache parms: "
		  "<name>:<nsets>:<bsize>:<assoc>:<repl>");
	  cache_dl2 = cache_create(name, nsets, bsize, /* balloc */FALSE,
				   /* usize */0, assoc, cache_char2policy(c),
				   dl2_access_fn, /* hit lat */cache_dl2_lat);
	}
    }

  /* use a level 1 I-cache? */
  if (!mystricmp(cache_il1_opt, "none"))
    {
      cache_il1 = NULL;

      /* the level 2 I-cache cannot be defined */
      if (strcmp(cache_il2_opt, "none"))
	fatal("the l1 inst cache must defined if the l2 cache is defined");
      cache_il2 = NULL;
    }
  else if (!mystricmp(cache_il1_opt, "dl1"))
    {
      if (!cache_dl1)
	fatal("I-cache l1 cannot access D-cache l1 as it's undefined");
      cache_il1 = cache_dl1;

      /* the level 2 I-cache cannot be defined */
      if (strcmp(cache_il2_opt, "none"))
	fatal("the l1 inst cache must defined if the l2 cache is defined");
      cache_il2 = NULL;
    }
  else if (!mystricmp(cache_il1_opt, "dl2"))
    {
      if (!cache_dl2)
	fatal("I-cache l1 cannot access D-cache l2 as it's undefined");
      cache_il1 = cache_dl2;

      /* the level 2 I-cache cannot be defined */
      if (strcmp(cache_il2_opt, "none"))
	fatal("the l1 inst cache must defined if the l2 cache is defined");
      cache_il2 = NULL;
    }
  else /* il1 is defined */
    {
      if (sscanf(cache_il1_opt, "%[^:]:%d:%d:%d:%c",
		 name, &nsets, &bsize, &assoc, &c) != 5)
	fatal("bad l1 I-cache parms: <name>:<nsets>:<bsize>:<assoc>:<repl>");
      cache_il1 = cache_create(name, nsets, bsize, /* balloc */FALSE,
			       /* usize */0, assoc, cache_char2policy(c),
			       il1_access_fn, /* hit lat */cache_il1_lat);

      /* is the level 2 D-cache defined? */
      if (!mystricmp(cache_il2_opt, "none"))
	cache_il2 = NULL;
      else if (!mystricmp(cache_il2_opt, "dl2"))
	{
	  if (!cache_dl2)
	    fatal("I-cache l2 cannot access D-cache l2 as it's undefined");
	  cache_il2 = cache_dl2;
	}
      else
	{
	  if (sscanf(cache_il2_opt, "%[^:]:%d:%d:%d:%c",
		     name, &nsets, &bsize, &assoc, &c) != 5)
	    fatal("bad l2 I-cache parms: "
		  "<name>:<nsets>:<bsize>:<assoc>:<repl>");
	  cache_il2 = cache_create(name, nsets, bsize, /* balloc */FALSE,
				   /* usize */0, assoc, cache_char2policy(c),
				   il2_access_fn, /* hit lat */cache_il2_lat);
	}
    }

  /* use an I-TLB? */
  if (!mystricmp(itlb_opt, "none"))
    itlb = NULL;
  else
    {
      if (sscanf(itlb_opt, "%[^:]:%d:%d:%d:%c",
		 name, &nsets, &bsize, &assoc, &c) != 5)
	fatal("bad TLB parms: <name>:<nsets>:<page_size>:<assoc>:<repl>");
      itlb = cache_create(name, nsets, bsize, /* balloc */FALSE,
			  /* usize */sizeof(md_addr_t), assoc,
			  cache_char2policy(c), itlb_access_fn,
			  /* hit latency */1);
    }

  /* use a D-TLB? */
  if (!mystricmp(dtlb_opt, "none"))
    dtlb = NULL;
  else
    {
      if (sscanf(dtlb_opt, "%[^:]:%d:%d:%d:%c",
		 name, &nsets, &bsize, &assoc, &c) != 5)
	fatal("bad TLB parms: <name>:<nsets>:<page_size>:<assoc>:<repl>");
      dtlb = cache_create(name, nsets, bsize, /* balloc */FALSE,
			  /* usize */sizeof(md_addr_t), assoc,
			  cache_char2policy(c), dtlb_access_fn,
			  /* hit latency */1);
    }

  if (cache_dl1_lat < 1)
    fatal("l1 data cache latency must be greater than zero");
  if (cache_dl1)
    res_update_oplat(fu_config, FU_NUM_INDICES, RdPort, cache_dl1_lat);

  if (cache_dl2_lat < 1)
    fatal("l2 data cache latency must be greater than zero");

  if (cache_il1_lat < 1)
    fatal("l1 instruction cache latency must be greater than zero");

  if (cache_il2_lat < 1)
    fatal("l2 instruction cache latency must be greater than zero");

  if (mem_nelt != 2)
    fatal("bad memory access latency (<first_chunk> <inter_chunk>)");

  if (mem_lat[0] < 1 || mem_lat[1] < 1)
    fatal("all memory access latencies must be greater than zero");

  if (mem_bus_width < 1 || (mem_bus_width & (mem_bus_width-1)) != 0)
    fatal("memory bus width must be positive non-zero and a power of two");

  if (tlb_miss_lat < 1)
    fatal("TLB miss latency must be greater than zero");

  if (res_ialu < 1)
    fatal("number of integer ALU's must be greater than zero");
  if (res_ialu > MAX_INSTS_PER_CLASS)
    fatal("number of integer ALU's must be <= MAX_INSTS_PER_CLASS");
  fu_config[FU_IALU_INDEX].quantity = res_ialu;
  
  if (res_imult < 1)
    fatal("number of integer multiplier/dividers must be greater than zero");
  if (res_imult > MAX_INSTS_PER_CLASS)
    fatal("number of integer mult/div's must be <= MAX_INSTS_PER_CLASS");
  fu_config[FU_IMULT_INDEX].quantity = res_imult;
  
  if (res_memport < 1)
    fatal("number of memory system ports must be greater than zero");
  if (res_memport > MAX_INSTS_PER_CLASS)
    fatal("number of memory system ports must be <= MAX_INSTS_PER_CLASS");
  fu_config[FU_MEMPORT_INDEX].quantity = res_memport;
  
  if (res_fpalu < 1)
    fatal("number of floating point ALU's must be greater than zero");
  if (res_fpalu > MAX_INSTS_PER_CLASS)
    fatal("number of floating point ALU's must be <= MAX_INSTS_PER_CLASS");
  fu_config[FU_FPALU_INDEX].quantity = res_fpalu;
  
  if (res_fpmult < 1)
    fatal("number of floating point multiplier/dividers must be > zero");
  if (res_fpmult > MAX_INSTS_PER_CLASS)
    fatal("number of FP mult/div's must be <= MAX_INSTS_PER_CLASS");
  fu_config[FU_FPMULT_INDEX].quantity = res_fpmult;

  /* Update latency information. */
  res_update_oplat(    fu_config, FU_NUM_INDICES, IntALU,    res_ialu_oplat);
  res_update_issuelat( fu_config, FU_NUM_INDICES, IntALU,    res_ialu_islat);
  res_update_oplat(    fu_config, FU_NUM_INDICES, IntMULT,   res_imul_oplat);
  res_update_issuelat( fu_config, FU_NUM_INDICES, IntMULT,   res_imul_islat);
  res_update_oplat(    fu_config, FU_NUM_INDICES, IntDIV,    res_idiv_oplat);
  res_update_issuelat( fu_config, FU_NUM_INDICES, IntDIV,    res_idiv_islat);
  res_update_oplat(    fu_config, FU_NUM_INDICES, RdPort,    res_read_oplat);
  res_update_issuelat( fu_config, FU_NUM_INDICES, RdPort,    res_read_islat);
  res_update_oplat(    fu_config, FU_NUM_INDICES, WrPort,    res_write_oplat);
  res_update_issuelat( fu_config, FU_NUM_INDICES, WrPort,    res_write_islat);
  res_update_oplat(    fu_config, FU_NUM_INDICES, FloatADD,  res_fadd_oplat);
  res_update_issuelat( fu_config, FU_NUM_INDICES, FloatADD,  res_fadd_islat);
  res_update_oplat(    fu_config, FU_NUM_INDICES, FloatCMP,  res_fcmp_oplat);
  res_update_issuelat( fu_config, FU_NUM_INDICES, FloatCMP,  res_fcmp_islat);
  res_update_oplat(    fu_config, FU_NUM_INDICES, FloatCVT,  res_fcvt_oplat);
  res_update_issuelat( fu_config, FU_NUM_INDICES, FloatCVT,  res_fcvt_islat);
  res_update_oplat(    fu_config, FU_NUM_INDICES, FloatMULT, res_fmul_oplat);
  res_update_issuelat( fu_config, FU_NUM_INDICES, FloatMULT, res_fmul_islat);
  res_update_oplat(    fu_config, FU_NUM_INDICES, FloatDIV,  res_fdiv_oplat);
  res_update_issuelat( fu_config, FU_NUM_INDICES, FloatDIV,  res_fdiv_islat);
  res_update_oplat(    fu_config, FU_NUM_INDICES, FloatSQRT, res_fsqrt_oplat);
  res_update_issuelat( fu_config, FU_NUM_INDICES, FloatSQRT, res_fsqrt_islat);
}

/* print simulator-specific configuration information */
void
sim_aux_config(FILE *stream)            /* output stream */
{
  /* nothing */
}

/* register simulator-specific statistics */
void
sim_reg_stats(struct stat_sdb_t *sdb)   /* stats database */
{
  int i;

  /* --- slip stats --- */

  stat_reg_counter(sdb, "sim_slip",
                   "total number of slip cycles",
                   &sim_slip, 0, NULL);
  stat_reg_formula(sdb, "avg_sim_slip",
                   "the average slip between issue and retirement",
                   "sim_slip / sim_num_insn", NULL);

  /* --- instruction counts --- */

  stat_reg_counter(sdb, "sim_num_insn",
		   "total number of instructions committed",
		   &sim_num_insn, sim_num_insn, NULL);
  stat_reg_counter(sdb, "sim_oracle_insn",
		   "total number of (non-spec) instructions executed by oracle",
		   &sim_oracle_insn, 0, NULL);
  stat_reg_counter(sdb, "sim_num_refs",
		   "total number of loads and stores committed",
		   &sim_num_refs, 0, NULL);
  stat_reg_counter(sdb, "sim_num_loads",
		   "total number of loads committed",
		   &sim_num_loads, 0, NULL);
  stat_reg_formula(sdb, "sim_num_stores",
		   "total number of stores committed",
		   "sim_num_refs - sim_num_loads", NULL);
  stat_reg_counter(sdb, "sim_num_branches",
		   "total number of branches committed",
		   &sim_num_branches, /* initial value */0, /* format */NULL);

  stat_reg_counter(sdb, "sim_total_insn",
		   "total number of instructions executed",
		   &sim_total_insn, 0, NULL);
  stat_reg_counter(sdb, "sim_total_refs",
		   "total number of loads and stores executed",
		   &sim_total_refs, 0, NULL);
  stat_reg_counter(sdb, "sim_total_loads",
		   "total number of loads executed",
		   &sim_total_loads, 0, NULL);
  stat_reg_formula(sdb, "sim_total_stores",
		   "total number of stores executed",
		   "sim_total_refs - sim_total_loads", NULL);
  stat_reg_counter(sdb, "sim_total_branches",
		   "total number of branches executed",
		   &sim_total_branches, /* initial value */0, /* format */NULL);

  /* --- simulation timing stats --- */

  stat_reg_int(sdb, "sim_elapsed_time",
	       "total simulation time in seconds",
	       &sim_elapsed_time, 0, NULL);
  stat_reg_formula(sdb, "sim_inst_rate",
		   "simulation speed (in insts/sec)",
		   "sim_num_insn / sim_elapsed_time", NULL);

  /* --- performance stats --- */

  stat_reg_counter(sdb, "sim_cycle",
		   "total simulation time in cycles",
		   &sim_cycle, /* initial value */0, /* format */NULL);
  stat_reg_formula(sdb, "sim_IPC",
		   "instructions per cycle",
		   "sim_num_insn / sim_cycle", /* format */NULL);
  stat_reg_formula(sdb, "sim_CPI",
		   "cycles per instruction",
		   "sim_cycle / sim_num_insn", /* format */NULL);
  stat_reg_formula(sdb, "sim_exec_BW",
		   "total instructions (mis-spec + committed) per cycle",
		   "sim_total_insn / sim_cycle", /* format */NULL);
  stat_reg_formula(sdb, "sim_IPB",
		   "instruction per branch",
		   "sim_num_insn / sim_num_branches", /* format */NULL);

  /* --- occupancy stats --- */

  stat_reg_counter(sdb, "IFQ_count", "cumulative IFQ occupancy",
                   &IFQ_count, /* initial value */0, /* format */NULL);
  stat_reg_counter(sdb, "IFQ_fcount", "cumulative IFQ full count",
                   &IFQ_fcount, /* initial value */0, /* format */NULL);
  stat_reg_formula(sdb, "ifq_occupancy", "avg IFQ occupancy (insn's)",
                   "IFQ_count / sim_cycle", /* format */NULL);
  stat_reg_formula(sdb, "ifq_rate", "avg IFQ dispatch rate (insn/cycle)",
                   "sim_total_insn / sim_cycle", /* format */NULL);
  stat_reg_formula(sdb, "ifq_latency", "avg IFQ occupant latency (cycle's)",
                   "ifq_occupancy / ifq_rate", /* format */NULL);
  stat_reg_formula(sdb, "ifq_full", "fraction of time (cycle's) IFQ was full",
                   "IFQ_fcount / sim_cycle", /* format */NULL);

  stat_reg_counter(sdb, "ISQ_count", "cumulative ISQ occupancy",
                   &ISQ_count, /* initial value */0, /* format */NULL);
  stat_reg_counter(sdb, "ISQ_fcount", "cumulative ISQ full count",
                   &ISQ_fcount, /* initial value */0, /* format */NULL);
  stat_reg_formula(sdb, "isq_occupancy", "avg ISQ occupancy (insn's)",
                   "ISQ_count / sim_cycle", /* format */NULL);
  stat_reg_formula(sdb, "isq_rate", "avg ISQ dispatch rate (insn/cycle)",
                   "sim_total_insn / sim_cycle", /* format */NULL);
  stat_reg_formula(sdb, "isq_latency", "avg ISQ occupant latency (cycle's)",
                   "isq_occupancy / isq_rate", /* format */NULL);
  stat_reg_formula(sdb, "isq_full", "fraction of time (cycle's) ISQ was full",
                   "ISQ_fcount / sim_cycle", /* format */NULL);

  stat_reg_counter(sdb, "RS_count", "cumulative RS occupancy",
                   &RS_count, /* initial value */0, /* format */NULL);
  stat_reg_counter(sdb, "RS_fcount", "cumulative RS full count",
                   &RS_fcount, /* initial value */0, /* format */NULL);
  stat_reg_formula(sdb, "rs_occupancy", "avg RS occupancy (insn's)",
                   "RS_count / sim_cycle", /* format */NULL);
  stat_reg_formula(sdb, "rs_rate", "avg RS dispatch rate (insn/cycle)",
                   "sim_total_insn / sim_cycle", /* format */NULL);
  stat_reg_formula(sdb, "rs_latency", "avg RS occupant latency (cycle's)",
                   "rs_occupancy / rs_rate", /* format */NULL);
  stat_reg_formula(sdb, "rs_full", "fraction of time (cycle's) RS was full",
                   "RS_fcount / sim_cycle", /* format */NULL);

  stat_reg_counter(sdb, "ROB_count", "cumulative ROB occupancy",
                   &ROB_count, /* initial value */0, /* format */NULL);
  stat_reg_counter(sdb, "ROB_fcount", "cumulative ROB full count",
                   &ROB_fcount, /* initial value */0, /* format */NULL);
  stat_reg_formula(sdb, "rob_occupancy", "avg ROB occupancy (insn's)",
                   "ROB_count / sim_cycle", /* format */NULL);
  stat_reg_formula(sdb, "rob_rate", "avg ROB dispatch rate (insn/cycle)",
                   "sim_total_insn / sim_cycle", /* format */NULL);
  stat_reg_formula(sdb, "rob_latency", "avg ROB occupant latency (cycle's)",
                   "rob_occupancy / rob_rate", /* format */NULL);
  stat_reg_formula(sdb, "rob_full", "fraction of time (cycle's) ROB was full",
                   "ROB_fcount / sim_cycle", /* format */NULL);

  stat_reg_counter(sdb, "LSQ_count", "cumulative LSQ occupancy",
                   &LSQ_count, /* initial value */0, /* format */NULL);
  stat_reg_counter(sdb, "LSQ_fcount", "cumulative LSQ full count",
                   &LSQ_fcount, /* initial value */0, /* format */NULL);
  stat_reg_formula(sdb, "lsq_occupancy", "avg LSQ occupancy (insn's)",
                   "LSQ_count / sim_cycle", /* format */NULL);
  stat_reg_formula(sdb, "lsq_rate", "avg LSQ dispatch rate (insn/cycle)",
                   "sim_total_insn / sim_cycle", /* format */NULL);
  stat_reg_formula(sdb, "lsq_latency", "avg LSQ occupant latency (cycle's)",
                   "lsq_occupancy / lsq_rate", /* format */NULL);
  stat_reg_formula(sdb, "lsq_full", "fraction of time (cycle's) LSQ was full",
                   "LSQ_fcount / sim_cycle", /* format */NULL);

  /* --- checker stats --- */

  stat_reg_counter(sdb, "timing_dep_insn", "timing dependent instructions",
                   &timing_dep_insn, /* initial value */0, /* format */NULL);
  stat_reg_counter(sdb, "checker_errors", "checker errors",
                   &checker_errors, /* initial value */0, /* format */NULL);


  /* --- latency mispredict (scheduler replay) stats --- */

  stat_reg_counter(sdb, "num_replays", "scheduler replays",
		   &num_replays, 0, NULL);


  /* --- branch predictor stats --- */

  stat_reg_counter(sdb, "misfetch_count", "misfetch count",
                   &misfetch_count, /* initial value */0, /* format */NULL);
  stat_reg_counter(sdb, "misfetch_only_count", "misfetch only count",
                   &misfetch_only_count, /* initial value */0, /* format */NULL);
  stat_reg_counter(sdb, "recovery_count", "recovery count",
                   &recovery_count, /* initial value */0, /* format */NULL);
  if (pred) bpred_reg_stats(pred, sdb);

  /* --- blind speculation stats --- */

  stat_reg_counter(sdb, "blind_spec_flushes", "blind spec flushes (non-spec instrs only)",
                   &blind_spec_flushes, /* initial value */0, /* format */NULL);
  stat_reg_counter(sdb, "blind_spec_all_flushes", "blind spec flushes (all instructions)",
                   &blind_spec_all_flushes, /* initial value */0, /* format */NULL);

  /* --- cache stats --- */

  if (cache_il1 && (cache_il1 != cache_dl1 && cache_il1 != cache_dl2))
    cache_reg_stats(cache_il1, sdb);
  if (cache_il2 && (cache_il2 != cache_dl1 && cache_il2 != cache_dl2))
    cache_reg_stats(cache_il2, sdb);
  if (cache_dl1) cache_reg_stats(cache_dl1, sdb);
  if (cache_dl2) cache_reg_stats(cache_dl2, sdb);
    
  /* --- TLB stats --- */

  if (itlb) cache_reg_stats(itlb, sdb);
  if (dtlb) cache_reg_stats(dtlb, sdb);

  /* --- memory stats --- */

  mem_reg_stats(mem, sdb);

  /* --- loader stats --- */

  ld_reg_stats(sdb);

  /* --- fetch stall stats --- */

  fetch_stall = stat_reg_dist(sdb, "fetch_stall",
                              "breakdown of stalls in fetch",
                              /* initial value */0,
                              /* array size */fetch_NUM,
                              /* bucket size */1,
                              /* print format */(PF_COUNT|PF_PDF),
                              /* format */NULL,
                              /* index map */fetch_stall_str,
                              /* print fn */NULL);

  /* --- dispatch stall stats --- */

  dp_stall = stat_reg_dist(sdb, "dp_stall",
                              "breakdown of stalls in dispatch",
                              /* initial value */0,
                              /* array size */dp_NUM,
                              /* bucket size */1,
                              /* print format */(PF_COUNT|PF_PDF),
                              /* format */NULL,
                              /* index map */dp_stall_str,
                              /* print fn */NULL);

  /* --- PC-based stats --- */

  for (i=0; i<spec_pcstat_nelt; i++) {

    char buf[512], buf1[512];
    struct stat_stat_t *stat;

    /* track the named statistical variable by text address */

    /* find it... */
    stat = stat_find_stat(sdb, spec_pcstat_vars[i]);
    if (!stat)
      fatal("cannot locate any statistic named `%s'", spec_pcstat_vars[i]);

    /* stat must be an integral type */
    if (stat->sc != sc_int && stat->sc != sc_uint && stat->sc != sc_counter)
      fatal("`-pcstat' statistical variable `%s' is not an integral type",
        stat->name);

    /* register this stat */
    spec_pcstat_stats[i] = stat;
    spec_pcstat_lastvals[i] = STATVAL(stat);

    /* declare the sparce text distribution */
    sprintf(buf, "%s_by_pc", stat->name);
    sprintf(buf1, "%s (by text address)", stat->desc);
    spec_pcstat_sdists[i] = stat_reg_sdist(sdb, buf, buf1,
				      /* initial value */0,
				      /* print format */(PF_COUNT|PF_PDF),
				      /* format */"0x%lx %u %.2f",
				      /* print fn */NULL);
  }

  for (i=0; i<pcstat_nelt; i++) {

    char buf[512], buf1[512];
    struct stat_stat_t *stat;

    /* track the named statistical variable by text address */

    /* find it... */
    stat = stat_find_stat(sdb, pcstat_vars[i]);
    if (!stat)
      fatal("cannot locate any statistic named `%s'", pcstat_vars[i]);

    /* stat must be an integral type */
    if (stat->sc != sc_int && stat->sc != sc_uint && stat->sc != sc_counter)
      fatal("`-pcstat' statistical variable `%s' is not an integral type",
        stat->name);

    /* register this stat */
    pcstat_stats[i] = stat;
    pcstat_lastvals[i] = STATVAL(stat);

    /* declare the sparce text distribution */
    sprintf(buf, "%s_by_pc", stat->name);
    sprintf(buf1, "%s (by text address)", stat->desc);
    pcstat_sdists[i] = stat_reg_sdist(sdb, buf, buf1,
				      /* initial value */0,
				      /* print format */(PF_COUNT|PF_PDF),
				      /* format */"0x%lx %u %.2f",
				      /* print fn */NULL);
  }
}

/* dump simulator-specific auxiliary simulator statistics */
void
sim_aux_stats(FILE *stream)             /* output stream */
{
  /* nothing */
}

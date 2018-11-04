/* mase-macros-exec.h - sim-mase macros for the execute stage */

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

/* register file macros */
#undef  SET_TPC
#define SET_TPC(EXPR)        	(re->PC += 0)
#define SET_NPC(EXPR)           (re->NPC = (EXPR))
#define CPC                     (re->PC)

#define TMP		  read_idep_list(re, DEP_NAME(DTMP)).addr
#define SET_TMP(EXPR)	  reg_value.addr = EXPR,			\
  set_odep_list(re, DEP_NAME(DTMP), reg_value, vt_addr)

#if defined(TARGET_PISA)

//#define GPR(N)            (read_idep_list(re, DEP_NAME(DGPR(N))).sw + (printf("{%d %d %llx}", re->isq_index, N, read_idep_list(re, DEP_NAME(DGPR(N))).sw)*0) )
#define GPR(N)            read_idep_list(re, DEP_NAME(DGPR(N))).sw
#define SET_GPR(N, EXPR)  reg_value.sw = EXPR,				\
  set_odep_list(re, DEP_NAME(DGPR(N)), reg_value, vt_sword)

#define FPR_L(N)          read_idep_list(re, DEP_NAME(DFPR_L(N))).sw
#define SET_FPR_L(N,EXPR) reg_value.sw = EXPR,				\
  set_odep_list(re, DEP_NAME(DFPR_L(N)), reg_value, vt_sword)

#define FPR_F(N)          read_idep_list(re, DEP_NAME(DFPR_L(N))).f
#define SET_FPR_F(N,EXPR) reg_value.f = EXPR,				\
  set_odep_list(re, DEP_NAME(DFPR_F(N)), reg_value, vt_sfloat)

#define FPR_D(N)          read_idep_list(re, DEP_NAME(DFPR_L(N))).d
#define SET_FPR_D(N,EXPR) reg_value.d = EXPR,				\
   set_odep_list(re, DEP_NAME(DFPR_D(N)), reg_value, vt_dfloat)    

#define HI		  read_idep_list(re, DEP_NAME(DHI)).sw
#define SET_HI(EXPR)	  reg_value.sw = EXPR,				\
   set_odep_list(re, DEP_NAME(DHI), reg_value, vt_sword)

#define LO		  read_idep_list(re, DEP_NAME(DLO)).sw
#define SET_LO(EXPR)      reg_value.sw = EXPR,				\
   set_odep_list(re, DEP_NAME(DLO), reg_value, vt_sword)

#define FCC		  read_idep_list(re, DEP_NAME(DFCC)).sw
#define SET_FCC(EXPR)     reg_value.sw = EXPR,				\
   set_odep_list(re, DEP_NAME(DFCC), reg_value, vt_sword)

#elif defined(TARGET_ALPHA)

#define GPR(N)            read_idep_list(re, DEP_NAME(DGPR(N))).q
#define SET_GPR(N, EXPR)  reg_value.q = EXPR,				\
  set_odep_list(re, DEP_NAME(DGPR(N)), reg_value, vt_qword)

#define FPR_Q(N)	  read_idep_list(re, DEP_NAME(DFPR(N))).q
#define SET_FPR_Q(N,EXPR) reg_value.q = EXPR,				\
  set_odep_list(re, DEP_NAME(DFPR(N)), reg_value, vt_qword)

#define FPR(N)		  read_idep_list(re, DEP_NAME(DFPR(N))).d
#define SET_FPR(N,EXPR)	  reg_value.d = EXPR,				\
  set_odep_list(re, DEP_NAME(DFPR(N)), reg_value, vt_dfloat)

#define FPCR		  read_idep_list(re, DEP_NAME(DFPCR)).q
#define SET_FPCR(EXPR)	  reg_value.q = EXPR,				\
  set_odep_list(re, DEP_NAME(DFPCR), reg_value, vt_qword)

#define UNIQ		  read_idep_list(re, DEP_NAME(DUNIQ)).q
#define SET_UNIQ(EXPR)	  reg_value.q = EXPR,				\
  set_odep_list(re, DEP_NAME(DUNIQ), reg_value, vt_qword)

#else
#error No ISA target defined...
#endif

/* memory access macros */
#define __READ_SPECMEM(DATA, FAULT)					\
  (FAULT = read_load_data(re, &DATA, sizeof(DATA))), DATA
#define READ_BYTE(ADDR, FAULT)						\
  (__READ_SPECMEM(mem_value.b, FAULT));					\
  if (re->ea_comp) { SET_TMP(ADDR); break; }
#define READ_HALF(ADDR, FAULT)						\
  (MD_SWAPH(__READ_SPECMEM(mem_value.h, FAULT)));			\
  if (re->ea_comp) { SET_TMP(ADDR); break; }
#define READ_WORD(ADDR, FAULT)						\
  (MD_SWAPW(__READ_SPECMEM(mem_value.w, FAULT)));			\
  if (re->ea_comp) { SET_TMP(ADDR); break; }
#define READ_QWORD(ADDR, FAULT)						\
  (MD_SWAPQ(__READ_SPECMEM(mem_value.q, FAULT)));			\
  if (re->ea_comp) { SET_TMP(ADDR); break; }

#define __WRITE_SPECMEM(DATA, ADDR, SIZE, FAULT)			\
  if (re->ea_comp) { SET_TMP(ADDR); break; }				\
  FAULT = read_store_data(re, &mem_value, DATA, SIZE);			\
  {re->mem_value = mem_value; /*printf("(%llx, %llx)",  mem_value, DATA);*/}
#define WRITE_BYTE(DATA, ADDR, FAULT)					\
  __WRITE_SPECMEM(DATA, ADDR, 1, FAULT);
#define WRITE_HALF(DATA, ADDR, FAULT)					\
  __WRITE_SPECMEM(MD_SWAPH(DATA), ADDR, 2, FAULT);
#define WRITE_WORD(DATA, ADDR, FAULT)					\
  __WRITE_SPECMEM(MD_SWAPW(DATA), ADDR, 4, FAULT);
#define WRITE_QWORD(DATA, ADDR, FAULT)					\
  {__WRITE_SPECMEM(MD_SWAPQ(DATA), ADDR, 8, FAULT); /*printf("[%llx]",  DATA);*/}

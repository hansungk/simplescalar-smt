/* mase-macros-oracle.h - sim-mase macros for the oracle */

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

/* These macros are used for executing instructions in the oracle.
 * 
 * They are also used to execute the instructions during fast-forwarding,
 * when start-up instructions are executed prior to the start of the
 * timing simulation. During fast-forward mode, the pre-update register
 * file and architectural memory (not pre-update memory) are used. */

/* register file macros */
#undef  SET_TPC
#define SET_TPC(EXPR)        	(target_addr = (EXPR))
#define SET_NPC(EXPR)           (pu_regs.regs_NPC = (EXPR))
#define CPC                     (pu_regs.regs_PC)

#if defined(TARGET_PISA)

#define GPR(N)                  (pu_regs.regs_R[N]) 
#define SET_GPR(N, EXPR)	(pu_regs.regs_R[N] = (EXPR))
#define FPR_L(N)                (pu_regs.regs_F.l[(N)])
#define SET_FPR_L(N,EXPR)       (pu_regs.regs_F.l[(N)] = (EXPR))
#define FPR_F(N)                (pu_regs.regs_F.f[(N)])
#define SET_FPR_F(N,EXPR)       (pu_regs.regs_F.f[(N)] = (EXPR))
#define FPR_D(N)                (pu_regs.regs_F.d[(N) >> 1])
#define SET_FPR_D(N,EXPR)       (pu_regs.regs_F.d[(N) >> 1] = (EXPR))
#define HI			(pu_regs.regs_C.hi)
#define SET_HI(EXPR)		(pu_regs.regs_C.hi = (EXPR))
#define LO			(pu_regs.regs_C.lo)
#define SET_LO(EXPR)		(pu_regs.regs_C.lo = (EXPR))
#define FCC			(pu_regs.regs_C.fcc)
#define SET_FCC(EXPR)		(pu_regs.regs_C.fcc = (EXPR))

#elif defined(TARGET_ALPHA)

#define GPR(N)                  (pu_regs.regs_R[N]) 
#define SET_GPR(N, EXPR)	(pu_regs.regs_R[N] = (EXPR))
#define FPR_Q(N)		(pu_regs.regs_F.q[(N)])
#define SET_FPR_Q(N,EXPR)	(pu_regs.regs_F.q[(N)] = (EXPR))
#define FPR(N)			(pu_regs.regs_F.d[(N)])
#define SET_FPR(N,EXPR)		(pu_regs.regs_F.d[(N)] = (EXPR))
#define FPCR			(pu_regs.regs_C.fpcr)
#define SET_FPCR(EXPR)		(pu_regs.regs_C.fpcr = (EXPR))
#define UNIQ			(pu_regs.regs_C.uniq)
#define SET_UNIQ(EXPR)		(pu_regs.regs_C.uniq = (EXPR))
#define FCC			(pu_regs.regs_C.fcc)
#define SET_FCC(EXPR)		(pu_regs.regs_C.fcc = (EXPR))

#else
#error No ISA target defined...
#endif

/* memory access macros */
#define __READ_SPECMEM(SRC, SRC_V, FAULT)                               \
  (mem_addr = (SRC), 							\
   mem_size = sizeof(SRC_V),						\
   ((FAULT) = pu_mem_access(mem, Read, mem_addr, &SRC_V, sizeof(SRC_V)), SRC_V))
#define READ_BYTE(SRC, FAULT)                                           \
  __READ_SPECMEM((SRC), mem_value.b, (FAULT))
#define READ_HALF(SRC, FAULT)                                           \
  MD_SWAPH(__READ_SPECMEM((SRC), mem_value.h, (FAULT)))
#define READ_WORD(SRC, FAULT)                                           \
  MD_SWAPW(__READ_SPECMEM((SRC), mem_value.w, (FAULT)))
#define READ_QWORD(SRC, FAULT)                                          \
  MD_SWAPQ(__READ_SPECMEM((SRC), mem_value.q, (FAULT)))

#define __WRITE_SPECMEM(SRC, DST, DST_V, FAULT)                         \
  (DST_V = (SRC), mem_addr = (DST), 					\
  ((FAULT) = (!fastfwd_mode						\
     ? pu_mem_access(mem, Write, mem_addr, &DST_V, sizeof(DST_V))	\
     : mem_access(mem, Write, mem_addr, &DST_V, sizeof(DST_V))		\
  )))
#define WRITE_BYTE(SRC, DST, FAULT)                                     \
  __WRITE_SPECMEM((SRC), (DST), mem_value.b, (FAULT)),			\
  mem_size = sizeof(mem_value.b);
#define WRITE_HALF(SRC, DST, FAULT)                                     \
  __WRITE_SPECMEM(MD_SWAPH(SRC), (DST), mem_value.h, (FAULT)),		\
  mem_size = sizeof(mem_value.h);
#define WRITE_WORD(SRC, DST, FAULT)                                     \
  __WRITE_SPECMEM(MD_SWAPW(SRC), (DST), mem_value.w, (FAULT)),		\
  mem_size = sizeof(mem_value.w);
#define WRITE_QWORD(SRC, DST, FAULT)                                    \
  __WRITE_SPECMEM(MD_SWAPQ(SRC), (DST), mem_value.q, (FAULT)),		\
  mem_size = sizeof(mem_value.q);

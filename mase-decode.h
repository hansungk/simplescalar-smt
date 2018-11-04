/* mase-decode.h - sim-mase decode macros */

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

#define DNA                     (-1)

/* The index to register name macros (such as DGPR) convert an
 * index into the register name.  The register name combines type
 * and register information into a single name.  The type and the
 * dependence name can be extracted using the GET_TYPE and DEP_NAME
 * macros respectively.  The index into the register file using the
 * INT_REG_INDEX and FP_REG_INDEX macros.
 *
 * Here is an example using the PISA instruction set:
 *
 * Floating point register $f10 can hold a single-precision
 * floating point number or a double-precision floating point
 * number.  The single-precision macro DFPR_F(N) returns 42 while
 * the double-precision macro DFPR_D(N) returns 78.  This allows
 * MASE to read the proper type of data when reading the register
 * file.  The dependence name is used to track dependencies
 * between instructions and needs to be the same regardless of
 * the tpye of data stored in $f10.  This is where the DEP_NAME
 * macro comes into play.  You can see that DEP_NAME(78) ==
 * DEP_NAME(42) == 42 so that dependencies are tracked between
 * instructions. */

/* --- PISA MACROS --- */
#if defined(TARGET_PISA)

/* One pecularity with the PISA instruction set not discussed in
 * the above example is that the register file can use successive
 * registers to represent larger values.  Even numbered registers
 * can store 64-bit values using itself and the following register.
 * Similarily, even numbered FP registers can store double-prec.
 * values using itself and the following registers.  This means
 * that if an instruction reads or writes one of these larger values,
 * it has a dependency with both of the smaller registers.  This
 * has been explicitly handled in pisa.def for the integer registers
 * except for a single instruction where explicitly adding the extra
 * input dependency would give it four input dependencies, one more
 * than is allowed.  For the FP registers, the dependencies are not
 * explicitly handled  as the compiler will not generate code that
 * intermixes single-precision and double-precision code. */

/* index to register name macros */
#define DGPR(N)                 (N)
#define DGPR_D(N)               ((N) & ~1)
#define DFPR_L(N)               ((N)+32)
#define DFPR_F(N)               ((N)+32)
#define DFPR_D(N)               (((N) & ~1)+68)
#define DHI                     (64)
#define DLO                     (65)
#define DFCC                    (66)
#define DTMP                    (67)

/* get the type from the register name */
#define GET_TYPE(N)				\
  (N == DNA) ? vt_none :			\
  (N >= 0 && N < 32) ? vt_sword :		\
  (N >= 32 && N < 64) ? vt_sfloat :		\
  (N >= 68) ? vt_dfloat :			\
  (N == DHI) ? vt_sword :			\
  (N == DLO) ? vt_sword :			\
  (N == DFCC) ? vt_sword :			\
  (N == DTMP) ? vt_addr : vt_none

/* convert the register name to a dependence name */
#define DEP_NAME(N)		((N >= 68) ? N - 36 : N)

/* check if the dependence name corresponds to an FP register */
#define REG_IS_FP(N)            (((N) >= 32) && ((N) < 64))

/* dependence name to index macros */
#define INT_REG_INDEX(N)        (N)
#define FP_REG_INDEX(N)         ((N)-32)

/* string for the dependence name */
#define REG_NAME(N) 					\
  (N == DTMP) ? "$tmp" : 				\
  (N == DFCC) ? "$fcc" : 				\
  (N == DHI) ? "$hi" : 					\
  (N == DLO) ? "$lo" : 					\
  (REG_IS_FP(N)) ? 					\
  md_reg_name(rt_fpr, FP_REG_INDEX(N)) :		\
  md_reg_name(rt_gpr, INT_REG_INDEX(N))

/* --- ALPHA MACROS --- */
#elif defined(TARGET_ALPHA)

/* Alpha only has double-precision FP values and 64-bit
 * integers.  Thus, the dependence names are the same as
 * the register names. */

/* index to register name macros */
#define DGPR(N)                 (N)
#define DFPR(N)                 ((N)+32)
#define DFPCR                   (64)
#define DUNIQ                   (65)
#define DTMP                    (66)

/* get the type from the register name */
#define GET_TYPE(N)				\
  (N == DNA) ? vt_none :			\
  (N >= 0 && N < 32) ? vt_qword :		\
  (N >= 32 && N < 64) ? vt_dfloat :		\
  (N == DFPCR) ? vt_qword :			\
  (N == DUNIQ) ? vt_qword :			\
  (N == DTMP) ? vt_addr : vt_none

/* convert the register name to a dependence name */
#define DEP_NAME(N)		(N)

/* check if the dependence name corresponds to an FP register */
#define REG_IS_FP(N)            (((N) >= 32) && ((N) < 64))

/* dependence name to index macros */
#define INT_REG_INDEX(N)        (N)
#define FP_REG_INDEX(N)         ((N)-32)

/* string for the dependence name */
#define REG_NAME(N) 					\
  (N == DTMP) ? "$tmp" : 				\
  (N == DUNIQ) ? "$uniq" : 				\
  (N == DFPCR) ? "$fpcr" :				\
  (REG_IS_FP(N)) ? 					\
  md_reg_name(rt_fpr, FP_REG_INDEX(N)) :		\
  md_reg_name(rt_gpr, INT_REG_INDEX(N))
 
#else
#error No ISA target defined...
#endif

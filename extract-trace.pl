#!/usr/ucb/perl

# extract-trace.pl - extracts relevant trace information

# This script was created by Eric Larson as part of the MASE project while
# at the University of Michigan.

# SimpleScalar(TM) Tool Suite
# Copyright (C) 1994-2001 by Todd M. Austin, Ph.D. and SimpleScalar, LLC.
# All Rights Reserved. 
# 
# THIS IS A LEGAL DOCUMENT, BY USING SIMPLESCALAR,
# YOU ARE AGREEING TO THESE TERMS AND CONDITIONS.
# 
# No portion of this work may be used by any commercial entity, or for any
# commercial purpose, without the prior, written permission of SimpleScalar,
# LLC (info@simplescalar.com). Nonprofit and noncommercial use is permitted
# as described below.
# 
# 1. SimpleScalar is provided AS IS, with no warranty of any kind, express
# or implied. The user of the program accepts full responsibility for the
# application of the program and the use of any results.
# 
# 2. Nonprofit and noncommercial use is encouraged. SimpleScalar may be
# downloaded, compiled, executed, copied, and modified solely for nonprofit,
# educational, noncommercial research, and noncommercial scholarship
# purposes provided that this notice in its entirety accompanies all copies.
# Copies of the modified software can be delivered to persons who use it
# solely for nonprofit, educational, noncommercial research, and
# noncommercial scholarship purposes provided that this notice in its
# entirety accompanies all copies.
# 
# 3. ALL COMMERCIAL USE, AND ALL USE BY FOR PROFIT ENTITIES, IS EXPRESSLY
# PROHIBITED WITHOUT A LICENSE FROM SIMPLESCALAR, LLC (info@simplescalar.com).
# 
# 4. No nonprofit user may place any restrictions on the use of this software,
# including as modified by the user, by any other authorized user.
# 
# 5. Noncommercial and nonprofit users may distribute copies of SimpleScalar
# in compiled or executable form as set forth in Section 2, provided that
# either: (A) it is accompanied by the corresponding machine-readable source
# code, or (B) it is accompanied by a written offer, with no time limit, to
# give anyone a machine-readable copy of the corresponding source code in
# return for reimbursement of the cost of distribution. This written offer
# must permit verbatim duplication by anyone, or (C) it is distributed by
# someone who received only the executable form, and is accompanied by a
# copy of the written offer of source code.
# 
# 6. SimpleScalar was developed by Todd M. Austin, Ph.D. The tool suite is
# currently maintained by SimpleScalar LLC (info@simplescalar.com). US Mail:
# 2395 Timbercrest Court, Ann Arbor, MI 48105.
# 
# Copyright (C) 2000-2001 by The Regents of The University of Michigan.
# Copyright (C) 1994-2001 by Todd M. Austin, Ph.D. and SimpleScalar, LLC.

# This script extracts the relevant trace information to compare traces.
# It removes the first word from the trace and removes speculative
# instructions.

# parse command line
#
if (@ARGV != 1) {
  die "Usage: extract_trace.pl <input_file>\n";
}

# open the simulation files
#
$infile = $ARGV[0];
open(INFILE, $infile) || die "Cannot open file: $infile\n";

while (defined ($line = <INFILE>)) {
  if ($line =~ /spec/) { next; }
  ($first_item) = split(/:/, $line);
  $line = substr($line, length($first_item) + 1);
  print $line;
}
close(INFILE);

exit 0;

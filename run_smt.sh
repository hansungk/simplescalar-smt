#!/bin/bash

if [[ $# -lt 2 ]]; then
    echo "Usage: $0 outfile args"
    exit 1
fi

outfile=$1
shift
args=$@

echo "outfile:" $outfile
echo "args:" $args

for thr in 1 2 4 8; do
    ./sim-outorder -config smt.config -smt:threads $thr $args >$outfile.t${thr} 2>&1 &
    ./sim-outorder -config smt.config -smt:threads $thr -smt:fetch_policy rr2.8 $args >$outfile.rr2.8.t${thr} 2>&1 &
    ./sim-outorder -config smt.config -smt:threads $thr -smt:fetch_policy rr2.4 $args >$outfile.rr2.4.t${thr} 2>&1 &
    ./sim-outorder -config smt.config -smt:threads $thr -smt:fetch_policy rand1.8 $args >$outfile.rand1.8.t${thr} 2>&1 &
    ./sim-outorder -config smt.config -smt:threads $thr -smt:fetch_policy rr1.8 $args >$outfile.rr1.8.t${thr} 2>&1 &
    ./sim-outorder -config smt.config -smt:threads $thr -smt:fgmt $args >$outfile.fgmt.t${thr} 2>&1 &
done

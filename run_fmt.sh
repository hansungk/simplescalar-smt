#!/bin/bash

if [[ $# -lt 1 ]]; then
    echo "Usage: $0 <binary>"
    exit 1
fi

outfile=$1
shift

echo "outfile:" $outfile
echo "args:" $@

config="-fastfwd 2000000000 -max:inst 1000000000 -ruu:size 128 -lsq:size 64 -decode:width 4 -issue:width 8 -commit:width 4 -cache:il1 il1:256:32:1:l -cache:dl1 dl1:128:32:4:l -cache:dl1lat 2 -cache:dl2 ul2:2048:64:8:l -cache:dl2lat 9 -mem:lat 250 2"

echo "All perfect:"
./sim-outorder $config -cache:dl1perfect -bpred perfect -cache:il1perfect -cache:il2perfect -tlb:itlbperfect -cache:dl2perfect -tlb:dtlbperfect $@ >$outfile.perf 2>&1 &
echo "+ real dl1c:"
./sim-outorder $config                   -bpred perfect -cache:il1perfect -cache:il2perfect -tlb:itlbperfect -cache:dl2perfect -tlb:dtlbperfect $@ >$outfile.dl1c 2>&1 &
echo "+ real bpred:"
./sim-outorder $config                                  -cache:il1perfect -cache:il2perfect -tlb:itlbperfect -cache:dl2perfect -tlb:dtlbperfect $@ >$outfile.bpred 2>&1 &
echo "+ real il1c:"
./sim-outorder $config                                                    -cache:il2perfect -tlb:itlbperfect -cache:dl2perfect -tlb:dtlbperfect $@ >$outfile.il1c 2>&1 &
echo "+ real il2c:"
./sim-outorder $config                                                                      -tlb:itlbperfect -cache:dl2perfect -tlb:dtlbperfect $@ >$outfile.il2c 2>&1 &
echo "+ real itlb:"
./sim-outorder $config                                                                                       -cache:dl2perfect -tlb:dtlbperfect $@ >$outfile.itlb 2>&1 &
echo "+ real dl2c:"
./sim-outorder $config                                                                                                         -tlb:dtlbperfect $@ >$outfile.dl2c 2>&1 &
echo "+ real dtlb:"
./sim-outorder $config                                                                                                                          $@ >$outfile.dtlb 2>&1 &

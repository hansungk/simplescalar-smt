#!/bin/bash

if [[ $# -lt 1 ]]; then
    echo "Usage: $0 <binary>"
    exit 1
fi

echo "args:" $@
echo "All perfect:"
./sim-outorder -cache:dl1perfect -bpred perfect -cache:il1perfect -cache:il2perfect -tlb:itlbperfect -cache:dl2perfect -tlb:dtlbperfect $@ >output.log.perf 2>&1 &
echo "+ real dl1c:"
./sim-outorder                   -bpred perfect -cache:il1perfect -cache:il2perfect -tlb:itlbperfect -cache:dl2perfect -tlb:dtlbperfect $@ >output.log.dl1c 2>&1 &
echo "+ real bpred:"
./sim-outorder                                  -cache:il1perfect -cache:il2perfect -tlb:itlbperfect -cache:dl2perfect -tlb:dtlbperfect $@ >output.log.bpred 2>&1 &
echo "+ real il1c:"
./sim-outorder                                                    -cache:il2perfect -tlb:itlbperfect -cache:dl2perfect -tlb:dtlbperfect $@ >output.log.il1c 2>&1 &
echo "+ real il2c:"
./sim-outorder                                                                      -tlb:itlbperfect -cache:dl2perfect -tlb:dtlbperfect $@ >output.log.il2c 2>&1 &
echo "+ real itlb:"
./sim-outorder                                                                                       -cache:dl2perfect -tlb:dtlbperfect $@ >output.log.itlb 2>&1 &
echo "+ real dl2c:"
./sim-outorder                                                                                                         -tlb:dtlbperfect $@ >output.log.dl2c 2>&1 &
echo "+ real dtlb:"
./sim-outorder                                                                                                                          $@ >output.log.dtlb 2>&1 &

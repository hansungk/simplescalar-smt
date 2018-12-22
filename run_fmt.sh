#!/bin/bash

if [[ $# -lt 1 ]]; then
    echo "$#!"
    echo "Usage: $0 <binary>"
    exit 1
fi

echo "binary: " $1
echo "All perfect:"
./sim-outorder -cache:dl1perfect -bpred perfect -cache:il1perfect -cache:il2perfect -tlb:itlbperfect -cache:dl2perfect -tlb:dtlbperfect $1 2>&1 | grep CPI
echo "+ real dl1c:"
./sim-outorder                   -bpred perfect -cache:il1perfect -cache:il2perfect -tlb:itlbperfect -cache:dl2perfect -tlb:dtlbperfect $1 2>&1 | grep CPI
echo "+ real bpred:"
./sim-outorder                                  -cache:il1perfect -cache:il2perfect -tlb:itlbperfect -cache:dl2perfect -tlb:dtlbperfect $1 2>&1 | grep CPI
echo "+ real il1c:"
./sim-outorder                                                    -cache:il2perfect -tlb:itlbperfect -cache:dl2perfect -tlb:dtlbperfect $1 2>&1 | grep CPI
echo "+ real il2c:"
./sim-outorder                                                                      -tlb:itlbperfect -cache:dl2perfect -tlb:dtlbperfect $1 2>&1 | grep CPI
echo "+ real itlb:"
./sim-outorder                                                                                       -cache:dl2perfect -tlb:dtlbperfect $1 2>&1 | grep CPI
echo "+ real dl2c:"
./sim-outorder                                                                                                         -tlb:dtlbperfect $1 2>&1 | grep CPI
echo "+ real dtlb:"
./sim-outorder                                                                                                                          $1 2>&1 | grep CPI

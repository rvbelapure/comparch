#!/bin/bash

./sim --print_inst=0 --max_inst_count=10000000 --max_sim_count=0 --use_bpred=1 --bpred_type=3 --enable_vmem=1 --print_pipe_freq=0 --perfect_dcache=0 --trace_file="trace.pzip" --run_thread_num=1 --output_file="trace.out" > /dev/null &
./sim --print_inst=0 --max_inst_count=10000000 --max_sim_count=0 --use_bpred=1 --bpred_type=3 --enable_vmem=1 --print_pipe_freq=0 --perfect_dcache=0 --trace_file="trace2.pzip" --run_thread_num=1 --output_file="trace2.out" > /dev/null &
./sim --print_inst=0 --max_inst_count=10000000 --max_sim_count=0 --use_bpred=1 --bpred_type=3 --enable_vmem=1 --print_pipe_freq=0 --perfect_dcache=0 --trace_file="trace3.pzip" --run_thread_num=1 --output_file="trace3.out" > /dev/null &



./sim --print_inst=0 --max_inst_count=10000000 --max_sim_count=0 --use_bpred=1 --bpred_type=3 --enable_vmem=1 --print_pipe_freq=0 --perfect_dcache=0 --trace_file="trace.pzip" --trace_file2="trace2.pzip" --run_thread_num=2 --output_file="trace12.out" > /dev/null &
./sim --print_inst=0 --max_inst_count=10000000 --max_sim_count=0 --use_bpred=1 --bpred_type=3 --enable_vmem=1 --print_pipe_freq=0 --perfect_dcache=0 --trace_file="trace.pzip" --trace_file2="trace3.pzip" --run_thread_num=2 --output_file="trace13.out" > /dev/null &
./sim --print_inst=0 --max_inst_count=10000000 --max_sim_count=0 --use_bpred=1 --bpred_type=3 --enable_vmem=1 --print_pipe_freq=0 --perfect_dcache=0 --trace_file="trace2.pzip" --trace_file2="trace3.pzip" --run_thread_num=2 --output_file="trace23.out" > /dev/null &

wait

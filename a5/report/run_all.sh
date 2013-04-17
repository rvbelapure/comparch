#!/bin/bash

rm -f *.out *.xml

# POWER

# Simulation 1
for freq in 900 1200 1500
do
./sim --print_inst=0 --max_inst_count=0 --max_sim_count=0 --use_bpred=1 --bpred_type=3  --bpred_hist_len=12 --enable_vmem=1 --print_pipe_freq=1 --perfect_dcache=0 --dcache_latency=5 --dcache_size=1024 --dcache_way=8 --mem_latency_row_hit=100 --mem_latency_row_miss=200 --clock_freq=$freq --run_thread_num=1 --trace_file="test2.pzip" --output_file="test2_freq"$freq".out" > /dev/null &
done

wait
echo "Done simulation 1"

# Simulation 2
for hlen in 4 8 12
do
./sim --print_inst=0 --max_inst_count=0 --max_sim_count=0 --use_bpred=1 --bpred_type=3  --bpred_hist_len=$hlen --enable_vmem=1 --print_pipe_freq=1 --perfect_dcache=0 --dcache_latency=5 --dcache_size=1024 --dcache_way=8 --mem_latency_row_hit=100 --mem_latency_row_miss=200 --clock_freq=1200 --run_thread_num=1 --trace_file="test2.pzip" --output_file="test2_hlen"$hlen".out" > /dev/null &
done

wait 
echo "Done simulation 2"

# Simulation 3
for csize in 1024 2048 4096
do
./sim --print_inst=0 --max_inst_count=0 --max_sim_count=0 --use_bpred=1 --bpred_type=3  --bpred_hist_len=12 --enable_vmem=1 --print_pipe_freq=1 --perfect_dcache=0 --dcache_latency=5 --dcache_size=$csize --dcache_way=8 --mem_latency_row_hit=100 --mem_latency_row_miss=200 --clock_freq=1200 --run_thread_num=1 --trace_file="test2.pzip" --output_file="test2_cache"$csize".out" > /dev/null &
done

wait
echo "Done simulation 3"

# Simulation 4
for threads in 1 2 4
do
./sim --print_inst=0 --max_inst_count=0 --max_sim_count=0 --use_bpred=1 --bpred_type=3  --bpred_hist_len=12 --enable_vmem=1 --print_pipe_freq=1 --perfect_dcache=0 --dcache_latency=5 --dcache_size=1024 --dcache_way=8 --mem_latency_row_hit=100 --mem_latency_row_miss=200 --clock_freq=$freq --run_thread_num=$threads --trace_file="test1.pzip" --trace_file2="test2.pzip" --trace_file3="test3.pzip" --trace_file4="test4.pzip" --output_file="test_MT"$threads".out" > /dev/null &
done

wait
echo "Done simulation 4"

# PERCEPTRON
# Q1 of perceptron

for size in 9 11 12 13
do
./sim --print_inst=0 --max_inst_count=0 --max_sim_count=0 --use_bpred=1 --bpred_type=3  --bpred_hist_len=$size --enable_vmem=1 --print_pipe_freq=1 --perfect_dcache=0 --dcache_latency=5 --dcache_size=1024 --dcache_way=8 --mem_latency_row_hit=100 --mem_latency_row_miss=200 --trace_file="long_trace1.pzip" --run_thread_num=1 --output_file="long1_gshare_hlen"$size".out" > /dev/null &

./sim --print_inst=0 --max_inst_count=0 --max_sim_count=0 --use_bpred=1 --bpred_type=3  --bpred_hist_len=$size --enable_vmem=1 --print_pipe_freq=1 --perfect_dcache=0 --dcache_latency=5 --dcache_size=1024 --dcache_way=8 --mem_latency_row_hit=100 --mem_latency_row_miss=200 --trace_file="long_trace2.pzip" --run_thread_num=1 --output_file="long2_gshare_hlen"$size".out" > /dev/null &
done

wait
echo "Done Perceptron Q1: part 1"

for size in 1 7 15 31
do
./sim --print_inst=0 --max_inst_count=0 --max_sim_count=0 --use_bpred=1 --bpred_type=4  --perceptron_hist_len=$size --perceptron_table_entry=512 --enable_vmem=1 --print_pipe_freq=1 --perfect_dcache=0 --dcache_latency=5 --dcache_size=1024 --dcache_way=8 --mem_latency_row_hit=100 --mem_latency_row_miss=200 --trace_file="long_trace1.pzip" --run_thread_num=1 --output_file="long1_percep_hlen"$size".out" > /dev/null &

./sim --print_inst=0 --max_inst_count=0 --max_sim_count=0 --use_bpred=1 --bpred_type=4  --perceptron_hist_len=$size --perceptron_table_entry=512 --enable_vmem=1 --print_pipe_freq=1 --perfect_dcache=0 --dcache_latency=5 --dcache_size=1024 --dcache_way=8 --mem_latency_row_hit=100 --mem_latency_row_miss=200 --trace_file="long_trace2.pzip" --run_thread_num=1 --output_file="long2_percep_hlen"$size".out" > /dev/null &
done

wait
echo "Done Perceptron Q1: part 2"

# Q2 of perceptron
	# sequence of numbers in comments : storage_budget, num_of_entries, hlen

	# 4K, 128, 31
./sim --print_inst=0 --max_inst_count=0 --max_sim_count=0 --use_bpred=1 --bpred_type=4  --perceptron_hist_len=31 --perceptron_table_entry=128 --enable_vmem=1 --print_pipe_freq=1 --perfect_dcache=0 --dcache_latency=5 --dcache_size=1024 --dcache_way=8 --mem_latency_row_hit=100 --mem_latency_row_miss=200 --trace_file="long_trace1.pzip" --run_thread_num=1 --output_file="long1_q2_4k_entries128.out" > /dev/null &

./sim --print_inst=0 --max_inst_count=0 --max_sim_count=0 --use_bpred=1 --bpred_type=4  --perceptron_hist_len=31 --perceptron_table_entry=128 --enable_vmem=1 --print_pipe_freq=1 --perfect_dcache=0 --dcache_latency=5 --dcache_size=1024 --dcache_way=8 --mem_latency_row_hit=100 --mem_latency_row_miss=200 --trace_file="long_trace2.pzip" --run_thread_num=1 --output_file="long2_q2_4k_entries128.out" > /dev/null &

	# 4K, 256, 15
./sim --print_inst=0 --max_inst_count=0 --max_sim_count=0 --use_bpred=1 --bpred_type=4  --perceptron_hist_len=15 --perceptron_table_entry=256 --enable_vmem=1 --print_pipe_freq=1 --perfect_dcache=0 --dcache_latency=5 --dcache_size=1024 --dcache_way=8 --mem_latency_row_hit=100 --mem_latency_row_miss=200 --trace_file="long_trace1.pzip" --run_thread_num=1 --output_file="long1_q2_4k_entries256.out" > /dev/null &

./sim --print_inst=0 --max_inst_count=0 --max_sim_count=0 --use_bpred=1 --bpred_type=4  --perceptron_hist_len=15 --perceptron_table_entry=256 --enable_vmem=1 --print_pipe_freq=1 --perfect_dcache=0 --dcache_latency=5 --dcache_size=1024 --dcache_way=8 --mem_latency_row_hit=100 --mem_latency_row_miss=200 --trace_file="long_trace2.pzip" --run_thread_num=1 --output_file="long2_q2_4k_entries256.out" > /dev/null &

	# 4K, 512, 7
./sim --print_inst=0 --max_inst_count=0 --max_sim_count=0 --use_bpred=1 --bpred_type=4  --perceptron_hist_len=7 --perceptron_table_entry=512 --enable_vmem=1 --print_pipe_freq=1 --perfect_dcache=0 --dcache_latency=5 --dcache_size=1024 --dcache_way=8 --mem_latency_row_hit=100 --mem_latency_row_miss=200 --trace_file="long_trace1.pzip" --run_thread_num=1 --output_file="long1_q2_4k_entries512.out" > /dev/null &

./sim --print_inst=0 --max_inst_count=0 --max_sim_count=0 --use_bpred=1 --bpred_type=4  --perceptron_hist_len=7 --perceptron_table_entry=512 --enable_vmem=1 --print_pipe_freq=1 --perfect_dcache=0 --dcache_latency=5 --dcache_size=1024 --dcache_way=8 --mem_latency_row_hit=100 --mem_latency_row_miss=200 --trace_file="long_trace2.pzip" --run_thread_num=1 --output_file="long2_q2_4k_entries512.out" > /dev/null &

	# 8K, 128, 63
./sim --print_inst=0 --max_inst_count=0 --max_sim_count=0 --use_bpred=1 --bpred_type=4  --perceptron_hist_len=63 --perceptron_table_entry=128 --enable_vmem=1 --print_pipe_freq=1 --perfect_dcache=0 --dcache_latency=5 --dcache_size=1024 --dcache_way=8 --mem_latency_row_hit=100 --mem_latency_row_miss=200 --trace_file="long_trace1.pzip" --run_thread_num=1 --output_file="long1_q2_8k_entries128.out" > /dev/null &

./sim --print_inst=0 --max_inst_count=0 --max_sim_count=0 --use_bpred=1 --bpred_type=4  --perceptron_hist_len=63 --perceptron_table_entry=128 --enable_vmem=1 --print_pipe_freq=1 --perfect_dcache=0 --dcache_latency=5 --dcache_size=1024 --dcache_way=8 --mem_latency_row_hit=100 --mem_latency_row_miss=200 --trace_file="long_trace2.pzip" --run_thread_num=1 --output_file="long2_q2_8k_entries128.out" > /dev/null &

	# 8K, 256, 31
./sim --print_inst=0 --max_inst_count=0 --max_sim_count=0 --use_bpred=1 --bpred_type=4  --perceptron_hist_len=31 --perceptron_table_entry=256 --enable_vmem=1 --print_pipe_freq=1 --perfect_dcache=0 --dcache_latency=5 --dcache_size=1024 --dcache_way=8 --mem_latency_row_hit=100 --mem_latency_row_miss=200 --trace_file="long_trace1.pzip" --run_thread_num=1 --output_file="long1_q2_8k_entries256.out" > /dev/null &

./sim --print_inst=0 --max_inst_count=0 --max_sim_count=0 --use_bpred=1 --bpred_type=4  --perceptron_hist_len=31 --perceptron_table_entry=256 --enable_vmem=1 --print_pipe_freq=1 --perfect_dcache=0 --dcache_latency=5 --dcache_size=1024 --dcache_way=8 --mem_latency_row_hit=100 --mem_latency_row_miss=200 --trace_file="long_trace2.pzip" --run_thread_num=1 --output_file="long2_q2_8k_entries256.out" > /dev/null &

	# 8K, 512, 15
./sim --print_inst=0 --max_inst_count=0 --max_sim_count=0 --use_bpred=1 --bpred_type=4  --perceptron_hist_len=15 --perceptron_table_entry=512 --enable_vmem=1 --print_pipe_freq=1 --perfect_dcache=0 --dcache_latency=5 --dcache_size=1024 --dcache_way=8 --mem_latency_row_hit=100 --mem_latency_row_miss=200 --trace_file="long_trace1.pzip" --run_thread_num=1 --output_file="long1_q2_8k_entries512.out" > /dev/null &

./sim --print_inst=0 --max_inst_count=0 --max_sim_count=0 --use_bpred=1 --bpred_type=4  --perceptron_hist_len=15 --perceptron_table_entry=512 --enable_vmem=1 --print_pipe_freq=1 --perfect_dcache=0 --dcache_latency=5 --dcache_size=1024 --dcache_way=8 --mem_latency_row_hit=100 --mem_latency_row_miss=200 --trace_file="long_trace2.pzip" --run_thread_num=1 --output_file="long2_q2_8k_entries512.out" > /dev/null &

wait
echo "Done Perceptron Q2"

#Q3 of perceptron
for theta in 0 13 27
do
./sim --print_inst=0 --max_inst_count=0 --max_sim_count=0 --use_bpred=1 --bpred_type=4  --perceptron_hist_len=7 --perceptron_table_entry=512 --perceptron_threshold_ovd=$theta --enable_vmem=1 --print_pipe_freq=1 --perfect_dcache=0 --dcache_latency=5 --dcache_size=1024 --dcache_way=8 --mem_latency_row_hit=100 --mem_latency_row_miss=200 --trace_file="long_trace1.pzip" --run_thread_num=1 --output_file="long1_q3_theta"$theta".out" > /dev/null &

./sim --print_inst=0 --max_inst_count=0 --max_sim_count=0 --use_bpred=1 --bpred_type=4  --perceptron_hist_len=7 --perceptron_table_entry=512 --perceptron_threshold_ovd=$theta --enable_vmem=1 --print_pipe_freq=1 --perfect_dcache=0 --dcache_latency=5 --dcache_size=1024 --dcache_way=8 --mem_latency_row_hit=100 --mem_latency_row_miss=200 --trace_file="long_trace2.pzip" --run_thread_num=1 --output_file="long2_q3_theta"$theta".out" > /dev/null &
done

wait
echo "Done Perceptron Q3"

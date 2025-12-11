#!/usr/bin/env bash
set -e   # 遇到错误立即退出

# =========================================
# Build all benchmarks
# =========================================
echo "=== Building project ==="
make clean
make

# =========================================
# Run all microbenchmarks sequentially
# =========================================
echo "=== Running all microbenchmarks ==="

./bin/00_function_call
echo "-----------------------------------"

./bin/01_context_switch
echo "-----------------------------------"

./bin/02_fetch_throughput
echo "-----------------------------------"

./bin/03_retire_throughput
echo "-----------------------------------"

./bin/04_load_store_throughput
echo "-----------------------------------"

./bin/05_branch_penalty
echo "-----------------------------------"

./bin/06_exec_unit_throughput
echo "-----------------------------------"

./bin/07_cache_latency
echo "-----------------------------------"

./bin/08_cache_bandwidth
echo "-----------------------------------"

./bin/09_dram_latency
echo "-----------------------------------"

./bin/010_dram_bandwidth
echo "-----------------------------------"

./bin/011_smt_sim
echo "-----------------------------------"

echo "=== All benchmarks completed successfully ==="
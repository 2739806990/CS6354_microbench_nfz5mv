# CS6354 Microbenchmarking Project — Checkpoint 2

**Group Member:** nfz5mv  

## Environment
- macOS Sequoia 15.3.1 (Apple Silicon)
- Apple M2 Max, 32 GB RAM
- Clang 17.0.0
- C11 standard
- No external dependencies

## Directory Structure
- `bin/` — compiled binaries (auto-created by `make`)
- `include/` — common header file (`harness.h`)
- `report/` — write-ups and result summaries
- `scripts/` — helper scripts (`run_all.sh`)
- `src/` — source code:
  - `harness.c` — timing utilities
  - `00_function_call.c` — benchmark for function call overhead
  - `01_context_switch.c` — benchmark for syscall and thread context switches
  - `02_fetch_throughput.c` — instruction fetch throughput
  - `03_retire_throughput.c` — effective instruction retire throughput
  - `04_load_store_throughput.c` — load/store bandwidth
  - `05_branch_penalty.c` — branch misprediction penalty
  - `06_exec_unit_throughput.c` — integer ALU bandwidth
  - `07_cache_latency.c` — cache latency (L1I / L1D / L2 / L3)
  - `08_cache_bandwidth.c` — sustained cache read/write throughput
  - `09_dram_latency.c` — main memory (DRAM) latency
  - `010_dram_bandwidth.c ` —  main memory (DRAM) bandwidth
  - `011_smt_sim.c` — SMT contention and symbiosis (simulated on Apple Silicon)
  
  


## Build Instructions

make

## To clean all binaries before rebuilding:

make clean


## Run Instructions

### Run Each Benchmark Manually:
 - ./bin/00_function_call
 - ./bin/01_context_switch
 - ./bin/02_fetch_throughput
 - ./bin/03_retire_throughput
 - ./bin/04_load_store_throughput
 - ./bin/05_branch_penalty
 - ./bin/06_exec_unit_throughput
 - ./bin/07_cache_latency
 - ./bin/08_cache_bandwidth
 - ./bin/09_dram_latency
 - ./bin/010_dram_bandwidth
 - ./bin/011_smt_sim



### Run Everything At Once:
 - ./scripts/run_all.sh


## Sample Output

[00]Measuring function call cost (ns per call), N=10000000
  f()            : 0.600 ns/call
  f(int)         : 0.600 ns/call
  f(int,int)     : 0.500 ns/call
  f(double)      : 0.400 ns/call


[01] 
 [Syscall round-trip] getpid() : 112.5 ns/call
[Thread ping-pong]  context switch : 1130.9 ns/switch


[02] Instruction Fetch Throughput Test (128 NOPs)
Blocks: 400000  (each block = 128 NOPs)
Total NOP instructions ≈ 51200000
NOP latency per instruction : 0.054 ns
≈ 0.172 cycles per instruction
≈ 5.818 instructions per cycle (fetch throughput)

[03] Effective Instruction Throughput Test (Independent Ops)
Iters=5000000

ILP	IPC
-----------
1	0.545
2	1.205
3	1.453
4	1.549
5	0.825
6	0.993
7	1.144
8	1.295

Min IPC: 0.545
Max IPC: 1.549
Median IPC: 1.174


[04] Load/Store Throughput Test (Independent Ops)

Load Throughput : 2.960 loads/cycle
Store Throughput: 1.935 stores/cycle

[05] Branch Misprediction Penalty Test (Fixed)
Iterations = 800000

Predictable branch cost : 1.96 cycles
Random branch cost      : 11.02 cycles
Estimated misprediction penalty = 9.06 cycles

[06] Integer Execution Unit Bandwidth Test (6-way ILP, unrolled)
Blocks per type: 1000000, UNROLL=16, LANES=6 (ops/block=96)

ADD throughput: 0.977 ops/cycle
MUL throughput: 0.845 ops/cycle
DIV throughput: 0.499 ops/cycle

[07] Cache Latency Test (L1I / L1D / L2 / L3)
Assumed CPU freq = 3.20 GHz

L1I latency : 7.01 cycles
L1D latency : 7.40 cycles
L2  latency : 17.33 cycles
L3  latency : 20.25 cycles

[08] Cache Bandwidth Test (L1I / L1D / L2 / L3)
Total buffer: 64.0 MiB, target traffic per level ≈ 512 MiB

Lvl         Size   Read BW (GB/s)  Write BW (GB/s)
-------------------------------------------------------------
L1I      16.0 KB           30.406           55.697
L1D      32.0 KB           32.796           55.133
L2      256.0 KB           31.967           51.143
L3        4.0 MB           31.718           51.089

[09] Main Memory (DRAM) Latency Test (pointer chasing)
Assumed CPU freq = 3.20 GHz
Working set size = 256.0 MiB (beyond LLC)

Estimated DRAM load latency : 331.11 cycles (≈ 103.471 ns)

[010] Main Memory (DRAM) Bandwidth Test (streaming loads/stores)
Working set: 512.0 MiB (beyond LLC), target traffic ≈ 2.0 GiB

DRAM Read  Bandwidth : 28.407 GB/s
DRAM Write Bandwidth : 42.196 GB/s

[11] SMT Contention & Symbiosis (Simulated)
NOTE: Apple Silicon does NOT support SMT.
This experiment simulates ALU vs MEM contention with co-scheduled threads.

Baseline ALU alone: 1.531 s

=== Case 1: ALU-heavy + ALU-heavy (Contention) ===
Thread1 time: 1.589 s
Thread2 time: 1.578 s
Total   time: 1.589 s

=== Case 2: ALU-heavy + Memory-heavy (Symbiosis) ===
Thread1 time: 1.549 s
Thread2 time: 0.489 s
Total   time: 1.549 s

## Notes
- On macOS, syscall(SYS_getpid) shows a deprecation warning, this is expected and does not affect correctness.
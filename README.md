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

### Run Everything At Once:
 - ./scripts/run_all.sh


## Sample Output

[00]Measuring function call cost (ns per call), N=10000000
  f()            : 0.600 ns/call
  f(int)         : 0.600 ns/call
  f(int,int)     : 0.500 ns/call
  f(double)      : 0.500 ns/call

[01] 
 [Syscall round-trip] getpid() : 112.7 ns/call  
[Thread ping-pong]  context switch : 1146.4 ns/switch

[02] Instruction Fetch Throughput Test
Iterations: 50000000  
Average time per instruction : 0.555 ns  
≈ 1.776 cycles per instruction  
≈ 0.563 instructions per cycle (fetch throughput)

[03] Effective Instruction Throughput Test  
Iterations: 5000000

ILP	IPC (Instrs/Cycle)  

1	0.528  
2	1.210  
3	1.351  
4	1.324  
5	1.593  
6	1.751  
7	1.751  
8	1.783  

Min IPC: 0.528  
Max IPC: 1.783  
Median IPC: 1.472

[04] Load/Store Throughput Test  
Iterations: 10000000

Load Throughput : 0.231 loads/cycle  
Store Throughput: 2.182 stores/cycle

[05] Branch Misprediction Penalty Test  
Unpredictable branch iterations: 1000000  
Estimated Branch Misprediction Penalty: 2.81 cycles

[06] Integer Execution Unit Bandwidth Test  
Iterations per type: 10000000  
ADD throughput: 0.97 ops/cycle  
MUL throughput: 0.28 ops/cycle  
DIV throughput: 0.55 ops/cycle  

=== All benchmarks completed successfully ===

## Notes
- On macOS, syscall(SYS_getpid) shows a deprecation warning, this is expected and does not affect correctness.
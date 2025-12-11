// 06_integer_bandwidth.c

#include <stdio.h>
#include <stdint.h>
#include "harness.h"

#define FREQ_GHZ 3.2         // CPU 主频（GHz）
#define BLOCKS   1000000     // 外层循环次数
#define LANES    6           // 一次并行的独立算术运算个数
#define UNROLL   16          // 每个 block 内展开次数

#define OPS_PER_BLOCK (LANES * UNROLL)


// 路独立 ADD，展开 UNROLL 次

static double run_add_benchmark(void) {
    volatile uint64_t a1 = 1,  a2 = 2,  a3 = 3;
    volatile uint64_t a4 = 4,  a5 = 5,  a6 = 6;
    volatile uint64_t b1 = 11, b2 = 13, b3 = 17;
    volatile uint64_t b4 = 19, b5 = 23, b6 = 29;

    uint64_t t_oh = timer_overhead_ns();

    uint64_t t0 = now_ns();
    for (int blk = 0; blk < BLOCKS; ++blk) {
        // 完全独立的 ADD：a1+=b1, a2+=b2, ...
        for (int u = 0; u < UNROLL; ++u) {
            a1 += b1; a2 += b2; a3 += b3;
            a4 += b4; a5 += b5; a6 += b6;
        }
    }
    uint64_t t1 = now_ns();

    uint64_t dt = t1 - t0 - t_oh;
    if ((int64_t)dt < 0) dt = 0;

    double cycles = (double)dt * FREQ_GHZ;
    double total_ops = (double)BLOCKS * (double)OPS_PER_BLOCK;

    return total_ops / cycles;  // ops / cycle
}


//  路独立 MUL，结构同上

static double run_mul_benchmark(void) {
    volatile uint64_t a1 = 3,  a2 = 5,  a3 = 7;
    volatile uint64_t a4 = 11, a5 = 13, a6 = 17;
    // 乘数选择为奇数，避免被编译器优化
    volatile uint64_t m1 = 3,  m2 = 5,  m3 = 7;
    volatile uint64_t m4 = 9,  m5 = 11, m6 = 13;

    uint64_t t_oh = timer_overhead_ns();

    uint64_t t0 = now_ns();
    for (int blk = 0; blk < BLOCKS; ++blk) {
        for (int u = 0; u < UNROLL; ++u) {
            a1 *= m1; a2 *= m2; a3 *= m3;
            a4 *= m4; a5 *= m5; a6 *= m6;
        }
    }
    uint64_t t1 = now_ns();

    uint64_t dt = t1 - t0 - t_oh;
    if ((int64_t)dt < 0) dt = 0;

    double cycles = (double)dt * FREQ_GHZ;
    double total_ops = (double)BLOCKS * (double)OPS_PER_BLOCK;

    return total_ops / cycles;  // ops / cycle
}


// 路独立 DIV，结构同上

static double run_div_benchmark(void) {
    volatile uint64_t a1 = 1000003, a2 = 2000003, a3 = 3000007;
    volatile uint64_t a4 = 4000007, a5 = 5000011, a6 = 6000011;
    // 除数取 >1 的常数，避免被编译器优化
    volatile uint64_t d1 = 3, d2 = 5, d3 = 7;
    volatile uint64_t d4 = 9, d5 = 11, d6 = 13;

    uint64_t t_oh = timer_overhead_ns();

    uint64_t t0 = now_ns();
    for (int blk = 0; blk < BLOCKS; ++blk) {
        for (int u = 0; u < UNROLL; ++u) {
            a1 /= d1; a2 /= d2; a3 /= d3;
            a4 /= d4; a5 /= d5; a6 /= d6;
        }
    }
    uint64_t t1 = now_ns();

    uint64_t dt = t1 - t0 - t_oh;
    if ((int64_t)dt < 0) dt = 0;

    double cycles = (double)dt * FREQ_GHZ;
    double total_ops = (double)BLOCKS * (double)OPS_PER_BLOCK;

    return total_ops / cycles;  // ops / cycle
}

// main：分别打印三种整数运算的吞吐率

int main(void) {
    printf("[06] Integer Execution Unit Bandwidth Test (6-way ILP, unrolled)\n");
    printf("Blocks per type: %d, UNROLL=%d, LANES=%d (ops/block=%d)\n\n",
           BLOCKS, UNROLL, LANES, OPS_PER_BLOCK);

    double add_ipc = run_add_benchmark();
    double mul_ipc = run_mul_benchmark();
    double div_ipc = run_div_benchmark();

    printf("ADD throughput: %.3f ops/cycle\n", add_ipc);
    printf("MUL throughput: %.3f ops/cycle\n", mul_ipc);
    printf("DIV throughput: %.3f ops/cycle\n", div_ipc);

    return 0;
}
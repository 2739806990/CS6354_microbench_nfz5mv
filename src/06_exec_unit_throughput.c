// 06_integer_bandwidth.c
// 实验目的：测量 CPU 整数执行单元的最大带宽
// 方法：通过执行仅包含一种算术操作的循环测量总时间并换算为每周期完成的操作数。

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "harness.h"

#define N 10000000     // 每种算术操作的循环次数
#define FREQ_GHZ 3.2   // 主频（GHz），根据机器实际情况可修改

// 测加法带宽：使用最简单的整型加法作为基准
static double run_add_benchmark(void) {
    volatile int x = 1, y = 2, z = 0;

    uint64_t t0 = now_ns();
    for (int i = 0; i < N; ++i)
        z += x + y;   // 加法操作
    uint64_t t1 = now_ns();

    double duration = (t1 - t0 - timer_overhead_ns()) * FREQ_GHZ; // 换算为时钟周期
    return N / duration;  // 返回每周期完成的加法操作数（ops/cycle）
}

// 测乘法带宽：乘法通常延迟更高，因此吞吐率会更低
static double run_mul_benchmark(void) {
    volatile int x = 3, y = 4, z = 0;

    uint64_t t0 = now_ns();
    for (int i = 0; i < N; ++i)
        z += x * y;   // 乘法操作
    uint64_t t1 = now_ns();

    double duration = (t1 - t0 - timer_overhead_ns()) * FREQ_GHZ;
    return N / duration;
}

// 测除法带宽：除法是最慢的整数运算之一，用于体现执行单元瓶颈
static double run_div_benchmark(void) {
    volatile int x = 10000, y = 3, z = 0;

    uint64_t t0 = now_ns();
    for (int i = 0; i < N; ++i)
        z += x / y;   // 除法操作
    uint64_t t1 = now_ns();

    double duration = (t1 - t0 - timer_overhead_ns()) * FREQ_GHZ;
    return N / duration;
}

int main(void) {
    printf("[06] Integer Execution Unit Bandwidth Test\n");
    printf("Iterations per type: %d\n", N);

    // 分别测量加法、乘法、除法的吞吐率
    double add_ips = run_add_benchmark();
    double mul_ips = run_mul_benchmark();
    double div_ips = run_div_benchmark();

    printf("ADD throughput: %.2f ops/cycle\n", add_ips);
    printf("MUL throughput: %.2f ops/cycle\n", mul_ips);
    printf("DIV throughput: %.2f ops/cycle\n", div_ips);

    return 0;
}
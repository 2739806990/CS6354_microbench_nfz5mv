// 05_branch_penalty.c
// 实验目的：测量分支预测失败造成的流水线冲刷延迟。
// 方法：构造一个难以预测的分支序列（交替执行 if/else），测量每次分支平均耗费的周期数。

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "harness.h"

#define REPEAT 50  // 重复次数，取中位数以减小系统波动

// 计算中位数函数
static double median(double *a, size_t n) {
    for (size_t i = 1; i < n; ++i) {
        double key = a[i];
        size_t j = i;
        while (j > 0 && a[j - 1] > key) {
            a[j] = a[j - 1];
            --j;
        }
        a[j] = key;
    }
    if (n % 2)
        return a[n / 2];
    else
        return 0.5 * (a[n / 2 - 1] + a[n / 2]);
}

// 测量核心函数：
// 构造不可预测的分支序列（每次循环 i 奇偶交替）。
// 分支预测器几乎无法正确预测，导致 CPU 不断清空流水线。
static double measure_branch_penalty(int iters, double freq_GHz) {
    double results[REPEAT];
    const uint64_t t_oh = timer_overhead_ns();  // 计时器开销

    for (int r = 0; r < REPEAT; ++r) {
        warmup_busy_loop(100000);  // 预热阶段，减少冷启动效应

        volatile int x = 0;
        volatile int sink = 0;  // 防止编译器优化掉分支体

        uint64_t t0 = now_ns();

        for (int i = 0; i < iters; ++i) {
            // 交替分支：一半条件为真，一半为假
            // 这样预测器无法学习出稳定模式
            if ((i % 2) == 0)
                sink += x;
            else
                sink -= x;
        }

        uint64_t t1 = now_ns();
        int64_t delta = (int64_t)t1 - (int64_t)t0 - (int64_t)t_oh;
        if (delta < 0) delta = 0;

        double total_cycles = delta * freq_GHz;          // 换算成时钟周期
        double penalty_per_branch = total_cycles / iters; // 平均每次分支耗费的周期数

        results[r] = penalty_per_branch;
    }

    // 返回中位数结果
    return median(results, REPEAT);
}

int main(void) {
    const int N = 1000000;       // 分支执行次数
    const double freq_GHz = 3.2; // 假设主频为 3.2GHz

    printf("[05] Branch Misprediction Penalty Test\n");
    printf("Unpredictable branch iterations: %d\n", N);

    double latency = measure_branch_penalty(N, freq_GHz);
    printf("Estimated Branch Misprediction Penalty: %.2f cycles\n", latency);

    return 0;
}
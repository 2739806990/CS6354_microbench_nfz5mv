// 03_retire_throughput.c
// 实验目的：测试 CPU 在不同指令级并行度 (ILP) 下的实际指令吞吐率
// 方法：通过多个独立累加器（acc）构造不同并行度的加法操作，测量在相同循环次数下，每周期平均提交（retire）的指令数。

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include "harness.h"

#define REPEAT 21  // 重复次数，用于统计中位数减少偶然误差

// 计算一组样本的中位数
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

// 测试核心函数：根据设定的 ILP 数量执行循环，计算平均 IPC
// ilp 表示独立累加器的数量（越大表示并行度越高）
// 每个累加器在一次循环中执行 8 次加法操作
static double measure_ilp_retire(size_t iters, int ilp, double freq_GHz) {
    double samples[REPEAT];
    const uint64_t t_oh = timer_overhead_ns();  // 计时器开销

    for (int r = 0; r < REPEAT; ++r) {
        warmup_busy_loop(100000);  // 预热 CPU，减少波动

        // 定义 8 个独立累加器，用于控制并行度
        volatile uint64_t acc1 = 0, acc2 = 0, acc3 = 0, acc4 = 0;
        volatile uint64_t acc5 = 0, acc6 = 0, acc7 = 0, acc8 = 0;

        uint64_t t0 = now_ns();

        // 根据并行度依次激活不同数量的累加器
        for (size_t i = 0; i < iters; ++i) {
            if (ilp >= 1) { acc1 += 1; acc1 += 1; acc1 += 1; acc1 += 1; acc1 += 1; acc1 += 1; acc1 += 1; acc1 += 1; }
            if (ilp >= 2) { acc2 += 1; acc2 += 1; acc2 += 1; acc2 += 1; acc2 += 1; acc2 += 1; acc2 += 1; acc2 += 1; }
            if (ilp >= 3) { acc3 += 1; acc3 += 1; acc3 += 1; acc3 += 1; acc3 += 1; acc3 += 1; acc3 += 1; acc3 += 1; }
            if (ilp >= 4) { acc4 += 1; acc4 += 1; acc4 += 1; acc4 += 1; acc4 += 1; acc4 += 1; acc4 += 1; acc4 += 1; }
            if (ilp >= 5) { acc5 += 1; acc5 += 1; acc5 += 1; acc5 += 1; acc5 += 1; acc5 += 1; acc5 += 1; acc5 += 1; }
            if (ilp >= 6) { acc6 += 1; acc6 += 1; acc6 += 1; acc6 += 1; acc6 += 1; acc6 += 1; acc6 += 1; acc6 += 1; }
            if (ilp >= 7) { acc7 += 1; acc7 += 1; acc7 += 1; acc7 += 1; acc7 += 1; acc7 += 1; acc7 += 1; acc7 += 1; }
            if (ilp >= 8) { acc8 += 1; acc8 += 1; acc8 += 1; acc8 += 1; acc8 += 1; acc8 += 1; acc8 += 1; acc8 += 1; }
        }

        uint64_t t1 = now_ns();
        int64_t delta = (int64_t)t1 - (int64_t)t0 - (int64_t)t_oh;
        if (delta < 0) delta = 0;

        // 总指令数 = 循环次数 × 并行度 × 每个累加器的加法数（8）
        int total_inst = iters * ilp * 8;
        double time_ns = (double)delta;
        double time_cycles = time_ns * freq_GHz;  // 转换为时钟周期
        double ipc = (double)total_inst / time_cycles;  // 每周期指令数

        samples[r] = ipc;
    }

    // 返回中位数，减少抖动
    return median(samples, REPEAT);
}

int main(void) {
    const size_t N = 5 * 1000 * 1000; // 500 万次循环
    const double freq_GHz = 3.2;      // 假设 CPU 主频为 3.2GHz

    printf("[03] Effective Instruction Throughput Test\n");
    printf("Iterations: %zu\n\n", N);

    printf("ILP\tIPC (Instrs/Cycle)\n");
    printf("---------------------------\n");

    double min_ipc = 1e9, max_ipc = 0.0;
    double all_ipc[8];

    // 依次测试 ILP = 1~8 的情况
    for (int ilp = 1; ilp <= 8; ++ilp) {
        double ipc = measure_ilp_retire(N, ilp, freq_GHz);
        all_ipc[ilp - 1] = ipc;

        if (ipc < min_ipc) min_ipc = ipc;
        if (ipc > max_ipc) max_ipc = ipc;

        printf("%d\t%.3f\n", ilp, ipc);
    }

    // 输出最小、最大和中位 IPC
    printf("\nMin IPC: %.3f\n", min_ipc);
    printf("Max IPC: %.3f\n", max_ipc);
    printf("Median IPC: %.3f\n", median(all_ipc, 8));

    return 0;
}
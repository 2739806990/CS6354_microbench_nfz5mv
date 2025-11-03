// 02_fetch_throughput.c
// 实验目的：测试 CPU 的指令取指吞吐率（Instruction Fetch Throughput）
// 方法：通过运行展开的空操作循环，估算每个周期可取指的数量。

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include "harness.h"

// 计算一组样本的中位数，用于减小偶然误差
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

// 主要测试函数：执行展开的循环体，测量取指平均耗时
static double measure_fetch_throughput(size_t iters) {
    const size_t REPEAT = 21;  // 重复多次以取中位数
    double *samples = (double *)malloc(REPEAT * sizeof(double));
    const uint64_t t_oh = timer_overhead_ns();  // 计时器开销

    for (size_t r = 0; r < REPEAT; ++r) {
        warmup_busy_loop(50000);  // 预热 CPU，避免冷启动干扰

        // 空循环，计算循环本身的开销
        uint64_t t0 = now_ns();
        for (size_t i = 0; i < iters; ++i) {
            __asm__ volatile("" ::: "memory");
        }
        uint64_t t1 = now_ns();
        uint64_t base_ns = t1 - t0;

        // 实际循环，每次循环执行 8 条简单加法指令
        t0 = now_ns();
        volatile uint64_t acc = 0;
        for (size_t i = 0; i < iters; ++i) {
            acc += 1; acc += 1; acc += 1; acc += 1;
            acc += 1; acc += 1; acc += 1; acc += 1;
        }
        t1 = now_ns();
        uint64_t with_ops_ns = t1 - t0;

        // 去掉基线时间与计时器开销
        int64_t diff = (int64_t)with_ops_ns - (int64_t)base_ns - (int64_t)(2 * t_oh);
        if (diff < 0) diff = 0;

        samples[r] = (double)diff / (double)(iters * 8);  // 每条指令平均耗时
    }

    double med = median(samples, REPEAT);
    free(samples);

    // 保留三位小数（ns/指令）
    return floor(med * 1000.0 + 0.5) / 1000.0;
}

int main(void) {
    const size_t N = 50 * 1000 * 1000ull; // 5千万次循环，保证稳定性
    printf("[02] Instruction Fetch Throughput Test\n");
    printf("Iterations: %zu\n", N);

    double ns_per_inst = measure_fetch_throughput(N);

    // 假设主频为 3.2GHz，计算对应的时钟周期数和取指速率
    double GHz = 3.2;
    double cycles_per_inst = ns_per_inst * GHz;
    double inst_per_cycle = 1.0 / cycles_per_inst;

    printf("Average time per instruction : %.3f ns\n", ns_per_inst);
    printf("≈ %.3f cycles per instruction\n", cycles_per_inst);
    printf("≈ %.3f instructions per cycle (fetch throughput)\n", inst_per_cycle);

    return 0;
}
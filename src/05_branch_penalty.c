// 05_branch_penalty_fixed.c
// 通过可预测分支与不可预测分支的平均周期差估计 misprediction penalty

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "harness.h"

#define REPEAT 30   // 重复测量次数，用于降低抖动

// 计算中位数
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
    return (n % 2) ? a[n/2] : 0.5 * (a[n/2 - 1] + a[n/2]);
}

// 可预测分支
// 分支条件恒为真，预测器 100% 命中，测得正常分支执行成本
static double measure_predictable(int iters, double freq) {
    double samples[REPEAT];
    uint64_t t_oh = timer_overhead_ns();

    for (int r = 0; r < REPEAT; r++) {
        warmup_busy_loop(20000);

        volatile int x = 1, sink = 0;

        uint64_t t0 = now_ns();
        for (int i = 0; i < iters; i++) {
            if (x)
                sink += 1;
        }
        uint64_t t1 = now_ns();

        double ns = (double)(t1 - t0 - t_oh);
        if (ns < 0) ns = 0;

        samples[r] = ns * freq / iters;  // 每次分支的平均周期
    }
    return median(samples, REPEAT);
}

// 不可预测分支
static double measure_unpredictable(int iters, double freq) {
    double samples[REPEAT];
    uint64_t t_oh = timer_overhead_ns();

    uint8_t *randbits = malloc(iters);

    for (int i = 0; i < iters; i++)
        randbits[i] = rand() & 1;

    for (int r = 0; r < REPEAT; r++) {
        warmup_busy_loop(20000);

        volatile int sink = 0;

        uint64_t t0 = now_ns();
        for (int i = 0; i < iters; i++) {
            if (randbits[i])
                sink++;
            else
                sink--;
        }
        uint64_t t1 = now_ns();

        double ns = (double)(t1 - t0 - t_oh);
        if (ns < 0) ns = 0;

        samples[r] = ns * freq / iters;
    }

    free(randbits);
    return median(samples, REPEAT);
}

int main(void) {
    const int N = 800000;   // 足够大的循环次数
    const double freq = 3.2;

    printf("[05] Branch Misprediction Penalty Test (Fixed)\n");
    printf("Iterations = %d\n\n", N);

    double cyc_pred = measure_predictable(N, freq);     // 可预测分支成本
    double cyc_rand = measure_unpredictable(N, freq);   // 随机分支成本

    double penalty = cyc_rand - cyc_pred;               // 差分得出 mispred penalty

    printf("Predictable branch cost : %.2f cycles\n", cyc_pred);
    printf("Random branch cost      : %.2f cycles\n", cyc_rand);
    printf("Estimated misprediction penalty = %.2f cycles\n", penalty);

    return 0;
}
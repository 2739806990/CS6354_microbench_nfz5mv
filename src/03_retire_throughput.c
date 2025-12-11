// 03_retire_throughput.c
// 实验目的：测试 CPU 在不同并行度 (ILP) 下的实际指令提交速率 (IPC)

#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include "harness.h"

#define REPEAT 15  // 中位数减少波动

// 中位数
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
    return (n % 2) ? a[n / 2] : 0.5 * (a[n/2 - 1] + a[n/2]);
}

// 执行 ILP 个独立 ADD 指令
static inline void run_ilp_block(int ilp,
                                 volatile uint64_t *a1, volatile uint64_t *a2,
                                 volatile uint64_t *a3, volatile uint64_t *a4,
                                 volatile uint64_t *a5, volatile uint64_t *a6,
                                 volatile uint64_t *a7, volatile uint64_t *a8)
{
    if (ilp >= 1) *a1 += 1;
    if (ilp >= 2) *a2 += 1;
    if (ilp >= 3) *a3 += 1;
    if (ilp >= 4) *a4 += 1;
    if (ilp >= 5) *a5 += 1;
    if (ilp >= 6) *a6 += 1;
    if (ilp >= 7) *a7 += 1;
    if (ilp >= 8) *a8 += 1;
}

static double measure_ilp(int iters, int ilp, double freq_GHz) {
    double samples[REPEAT];
    uint64_t t_oh = timer_overhead_ns();

    for (int r = 0; r < REPEAT; r++) {
        warmup_busy_loop(30000);

        volatile uint64_t a1=0, a2=0, a3=0, a4=0;
        volatile uint64_t a5=0, a6=0, a7=0, a8=0;

        uint64_t t0 = now_ns();
        for (int i = 0; i < iters; i++) {
            // ILP independent ops:
            // a1+=1; a2+=1; ... (独立不依赖)
            run_ilp_block(ilp, &a1,&a2,&a3,&a4,&a5,&a6,&a7,&a8);
        }
        uint64_t t1 = now_ns();

        double ns = (double)(t1 - t0 - t_oh);
        if (ns < 0) ns = 0;

        double cycles = ns * freq_GHz;
        double total_inst = (double)iters * ilp;  // 每轮发射 ilp 条指令

        samples[r] = total_inst / cycles;
    }
    return median(samples, REPEAT);
}

int main(void) {
    const int iters = 5 * 1000000;
    const double freq = 3.2;

    printf("[03] Effective Instruction Throughput Test (Independent Ops)\n");
    printf("Iters=%d\n\n", iters);

    printf("ILP\tIPC\n");
    printf("-----------\n");

    double all[8];
    double min=1e9, max=0;

    for (int ilp = 1; ilp <= 8; ilp++) {
        double ipc = measure_ilp(iters, ilp, freq);
        all[ilp-1] = ipc;

        if (ipc < min) min = ipc;
        if (ipc > max) max = ipc;

        printf("%d\t%.3f\n", ilp, ipc);
    }

    printf("\nMin IPC: %.3f\n", min);
    printf("Max IPC: %.3f\n", max);
    printf("Median IPC: %.3f\n", median(all, 8));

    return 0;
}
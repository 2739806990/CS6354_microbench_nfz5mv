// 09_dram_latency.c

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "harness.h"

#define REPEAT       7
#define DRAM_BYTES   (256ull * 1024ull * 1024ull)   // 256 MiB，远大于 L3
#define CPU_FREQ_GHZ 3.2

// 简单中位数
static double median(double *a, int n) {
    for (int i = 1; i < n; i++) {
        double k = a[i];
        int j = i;
        while (j > 0 && a[j - 1] > k) {
            a[j] = a[j - 1];
            j--;
        }
        a[j] = k;
    }
    return (n % 2) ? a[n/2] : 0.5 * (a[n/2] + a[n/2 - 1]);
}

// 构造一个随机 permutation ring: 0 -> p[0] -> p[p[0]] -> ...
static void build_random_ring(uint32_t *buf, size_t len) {
    uint32_t *tmp = (uint32_t *)malloc(len * sizeof(uint32_t));
    if (!tmp) {
        fprintf(stderr, "malloc failed in build_random_ring\n");
        exit(1);
    }

    for (size_t i = 0; i < len; i++)
        tmp[i] = (uint32_t)i;

    // Fisher–Yates
    for (size_t i = len - 1; i > 0; i--) {
        size_t j = (size_t)(rand() % (int)(i + 1));
        uint32_t t = tmp[i];
        tmp[i] = tmp[j];
        tmp[j] = t;
    }

    // 建环：buf[tmp[i]] = tmp[i+1]
    for (size_t i = 0; i < len - 1; i++)
        buf[tmp[i]] = tmp[i + 1];
    buf[tmp[len - 1]] = tmp[0];

    free(tmp);
}

// DRAM pointer chasing
static double measure_dram_latency(size_t bytes, double freq_GHz) {
    size_t len = bytes / sizeof(uint32_t);
    if (len < 1024) len = 1024; // 稍微兜个底

    uint32_t *buf = aligned_alloc(64, len * sizeof(uint32_t));
    if (!buf) {
        fprintf(stderr, "aligned_alloc(%zu) failed\n", len * sizeof(uint32_t));
        exit(1);
    }

    build_random_ring(buf, len);

    // 预热
    warmup_busy_loop(100000);

    double samples[REPEAT];
    uint64_t t_oh = timer_overhead_ns();

    // 保证统计稳定
    const size_t steps = 8ull * 1024ull * 1024ull; // 8M pointer derefs

    for (int r = 0; r < REPEAT; r++) {
        volatile uint32_t idx = 0;

        uint64_t t0 = now_ns();
        for (size_t i = 0; i < steps; i++) {
            idx = buf[idx];  
        }
        uint64_t t1 = now_ns();

        (void)idx; // 防止被优化

        double ns = (double)(t1 - t0 - t_oh);
        if (ns < 0) ns = 0;

        double cycles = ns * freq_GHz;
        samples[r] = cycles / (double)steps;   // 每次 pointer 追踪的 cycles
    }

    free(buf);
    return median(samples, REPEAT);
}

int main(void) {
    double freq = CPU_FREQ_GHZ;

    printf("[09] Main Memory (DRAM) Latency Test (pointer chasing)\n");
    printf("Assumed CPU freq = %.2f GHz\n", freq);
    printf("Working set size = %.1f MiB (beyond LLC)\n\n",
           DRAM_BYTES / 1024.0 / 1024.0);

    double cycles = measure_dram_latency(DRAM_BYTES, freq);
    double ns = cycles / freq;

    printf("Estimated DRAM load latency : %.2f cycles (≈ %.3f ns)\n",
           cycles, ns);

    return 0;
}
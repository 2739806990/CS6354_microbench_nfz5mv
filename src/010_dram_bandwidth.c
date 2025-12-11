// 10_dram_bandwidth.c

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "harness.h"

#define REPEAT        7                           // 取中位数减小抖动
#define DRAM_BYTES    (512ull * 1024ull * 1024ull) // 512 MiB 工作集
#define TARGET_BYTES  (2ull * 1024ull * 1024ull * 1024ull) // 目标总流量 ≈ 2 GiB

// 中位数
static double median(double *a, int n) {
    for (int i = 1; i < n; ++i) {
        double key = a[i];
        int j = i;
        while (j > 0 && a[j - 1] > key) {
            a[j] = a[j - 1];
            --j;
        }
        a[j] = key;
    }
    return (n % 2) ? a[n / 2] : 0.5 * (a[n / 2] + a[n / 2 - 1]);
}

// DRAM 读带宽：在 size_bytes 工作集上做大量顺序 load（8-way independent）
static double measure_dram_read_bw(uint8_t *buf, size_t size_bytes) {
    const size_t elems  = size_bytes / sizeof(uint64_t);
    const size_t unroll = 8;
    const size_t step   = unroll;

    double samples[REPEAT];
    const uint64_t t_oh = timer_overhead_ns();

    // 让总访问量接近 TARGET_BYTES
    size_t outer = TARGET_BYTES / size_bytes;
    if (outer < 1) outer = 1;

    for (int r = 0; r < REPEAT; ++r) {
        warmup_busy_loop(50000);

        volatile uint64_t sink0 = 0, sink1 = 0, sink2 = 0, sink3 = 0;
        volatile uint64_t sink4 = 0, sink5 = 0, sink6 = 0, sink7 = 0;

        uint64_t *p = (uint64_t *)buf;

        uint64_t t0 = now_ns();
        for (size_t o = 0; o < outer; ++o) {
            for (size_t i = 0; i + step <= elems; i += step) {
                // 8 条完全独立的 load（符合 TA 一直强调的 independent ops）
                sink0 += p[i + 0];
                sink1 += p[i + 1];
                sink2 += p[i + 2];
                sink3 += p[i + 3];
                sink4 += p[i + 4];
                sink5 += p[i + 5];
                sink6 += p[i + 6];
                sink7 += p[i + 7];
            }
        }
        uint64_t t1 = now_ns();

        (void)sink0; (void)sink1; (void)sink2; (void)sink3;
        (void)sink4; (void)sink5; (void)sink6; (void)sink7;

        int64_t dt = (int64_t)t1 - (int64_t)t0 - (int64_t)t_oh;
        if (dt < 0) dt = 0;

        double bytes    = (double)size_bytes * (double)outer;
        double bw_gb_s  = bytes / (double)dt;  // bytes/ns → GB/s（1 GB = 1e9 B）
        samples[r] = bw_gb_s;
    }

    return median(samples, REPEAT);
}

// DRAM 写带宽：在 size_bytes 工作集上做大量顺序 store（8-way independent）
static double measure_dram_write_bw(uint8_t *buf, size_t size_bytes) {
    const size_t elems  = size_bytes / sizeof(uint64_t);
    const size_t unroll = 8;
    const size_t step   = unroll;

    double samples[REPEAT];
    const uint64_t t_oh = timer_overhead_ns();

    size_t outer = TARGET_BYTES / size_bytes;
    if (outer < 1) outer = 1;

    for (int r = 0; r < REPEAT; ++r) {
        warmup_busy_loop(50000);

        volatile uint64_t *p = (volatile uint64_t *)buf;

        uint64_t v0 = 1, v1 = 2, v2 = 3, v3 = 4;
        uint64_t v4 = 5, v5 = 6, v6 = 7, v7 = 8;

        uint64_t t0 = now_ns();
        for (size_t o = 0; o < outer; ++o) {
            for (size_t i = 0; i + step <= elems; i += step) {
                // 8 条独立的 store 指令
                p[i + 0] = v0;
                p[i + 1] = v1;
                p[i + 2] = v2;
                p[i + 3] = v3;
                p[i + 4] = v4;
                p[i + 5] = v5;
                p[i + 6] = v6;
                p[i + 7] = v7;

                // 简单变化一下，避免被当成死写消掉
                v0++; v1++; v2++; v3++;
                v4++; v5++; v6++; v7++;
            }
        }
        uint64_t t1 = now_ns();

        int64_t dt = (int64_t)t1 - (int64_t)t0 - (int64_t)t_oh;
        if (dt < 0) dt = 0;

        double bytes    = (double)size_bytes * (double)outer;
        double bw_gb_s  = bytes / (double)dt;  // GB/s
        samples[r] = bw_gb_s;
    }

    return median(samples, REPEAT);
}

int main(void) {
    // 申请 512 MiB 作为 DRAM 工作集（远大于 LLC）
    uint8_t *buf = (uint8_t *)malloc(DRAM_BYTES);
    if (!buf) {
        fprintf(stderr, "Failed to allocate %.1f MiB buffer\n",
                DRAM_BYTES / 1024.0 / 1024.0);
        return 1;
    }
    memset(buf, 0, DRAM_BYTES);  // 轻微初始化，避免 page fault 峰值

    printf("[10] Main Memory (DRAM) Bandwidth Test (streaming loads/stores)\n");
    printf("Working set: %.1f MiB (beyond LLC), target traffic ≈ %.1f GiB\n\n",
           DRAM_BYTES / 1024.0 / 1024.0,
           TARGET_BYTES / 1024.0 / 1024.0 / 1024.0);

    double read_bw  = measure_dram_read_bw(buf, DRAM_BYTES);
    double write_bw = measure_dram_write_bw(buf, DRAM_BYTES);

    printf("DRAM Read  Bandwidth : %.3f GB/s\n", read_bw);
    printf("DRAM Write Bandwidth : %.3f GB/s\n", write_bw);

    free(buf);
    return 0;
}
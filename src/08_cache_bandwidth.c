// 08_cache_bandwidth.c

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "harness.h"

#define REPEAT        7                             // 每个 size 重复次数，取中位数
#define TARGET_BYTES  (512ull * 1024ull * 1024ull)  // 每个层级尽量访问 ~512MB 数据

// 简单的中位数函数（直接插入排序）
static double median(double *a, int n) {
    for (int i = 1; i < n; i++) {
        double key = a[i];
        int j = i;
        while (j > 0 && a[j - 1] > key) {
            a[j] = a[j - 1];
            j--;
        }
        a[j] = key;
    }
    return (n % 2) ? a[n / 2] : 0.5 * (a[n / 2] + a[n / 2 - 1]);
}

// 读带宽测试：对给定 buffer 大小 size_bytes，执行大量顺序 load，输出 GB/s
static double measure_read_bw(uint8_t *buf, size_t size_bytes) {
    const size_t elems  = size_bytes / sizeof(uint64_t); // 8B 单位
    const size_t UNROLL = 8;                             // 每轮 8 个独立 load
    const size_t STEP   = UNROLL;

    double samples[REPEAT];
    const uint64_t t_oh = timer_overhead_ns();

    // 动态选择外层循环次数，使总访问量接近 TARGET_BYTES
    size_t outer = TARGET_BYTES / size_bytes;
    if (outer < 1) outer = 1;

    for (int r = 0; r < REPEAT; r++) {
        warmup_busy_loop(50000);

        uint64_t *p = (uint64_t *)buf;
        volatile uint64_t s0 = 0, s1 = 0, s2 = 0, s3 = 0;
        volatile uint64_t s4 = 0, s5 = 0, s6 = 0, s7 = 0;

        uint64_t t0 = now_ns();
        for (size_t o = 0; o < outer; o++) {
            for (size_t i = 0; i + STEP <= elems; i += STEP) {
                // 独立load
                uint64_t v0 = p[i + 0];
                uint64_t v1 = p[i + 1];
                uint64_t v2 = p[i + 2];
                uint64_t v3 = p[i + 3];
                uint64_t v4 = p[i + 4];
                uint64_t v5 = p[i + 5];
                uint64_t v6 = p[i + 6];
                uint64_t v7 = p[i + 7];

                // 防止优化
                s0 += v0; s1 += v1; s2 += v2; s3 += v3;
                s4 += v4; s5 += v5; s6 += v6; s7 += v7;
            }
        }
        uint64_t t1 = now_ns();

        (void)s0; (void)s1; (void)s2; (void)s3;
        (void)s4; (void)s5; (void)s6; (void)s7;

        int64_t dt = (int64_t)t1 - (int64_t)t0 - (int64_t)t_oh;
        if (dt < 0) dt = 0;

        double bytes = (double)size_bytes * (double)outer;
        
        double bw_gb_s = bytes / (double)dt;
        samples[r] = bw_gb_s;
    }

    return median(samples, REPEAT);
}

// 写带宽测试：顺序 store，输出 GB/s
static double measure_write_bw(uint8_t *buf, size_t size_bytes) {
    const size_t elems  = size_bytes / sizeof(uint64_t);
    const size_t UNROLL = 8;
    const size_t STEP   = UNROLL;

    double samples[REPEAT];
    const uint64_t t_oh = timer_overhead_ns();

    size_t outer = TARGET_BYTES / size_bytes;
    if (outer < 1) outer = 1;

    for (int r = 0; r < REPEAT; r++) {
        warmup_busy_loop(50000);

        volatile uint64_t *p = (volatile uint64_t *)buf; // volatile 防止 store 被优化
        uint64_t v0 = 1, v1 = 2, v2 = 3, v3 = 4;
        uint64_t v4 = 5, v5 = 6, v6 = 7, v7 = 8;

        uint64_t t0 = now_ns();
        for (size_t o = 0; o < outer; o++) {
            for (size_t i = 0; i + STEP <= elems; i += STEP) {
                // 8 个独立的 store 指令
                p[i + 0] = v0;
                p[i + 1] = v1;
                p[i + 2] = v2;
                p[i + 3] = v3;
                p[i + 4] = v4;
                p[i + 5] = v5;
                p[i + 6] = v6;
                p[i + 7] = v7;

                // 改变写入值
                v0++; v1++; v2++; v3++;
                v4++; v5++; v6++; v7++;
            }
        }
        uint64_t t1 = now_ns();

        int64_t dt = (int64_t)t1 - (int64_t)t0 - (int64_t)t_oh;
        if (dt < 0) dt = 0;

        double bytes = (double)size_bytes * (double)outer;
        double bw_gb_s = bytes / (double)dt;
        samples[r] = bw_gb_s;
    }

    return median(samples, REPEAT);
}

int main(void) {
    // 统一申请一个 64 MiB 的 buffer，不同“层级”使用前缀子区间
    const size_t BUF_SIZE = 64ull * 1024ull * 1024ull; // 64 MiB
    uint8_t *buf = (uint8_t *)malloc(BUF_SIZE);
    if (!buf) {
        fprintf(stderr, "Failed to allocate buffer\n");
        return 1;
    }
    memset(buf, 0, BUF_SIZE);

    struct level_cfg {
        const char *name;      // 打印名：L1I / L1D / L2 / L3
        size_t      size_bytes;
    } levels[] = {
        { "L1I",   16ull * 1024ull },   // 16 KB footprint
        { "L1D",   32ull * 1024ull },   // 32 KB footprint
        { "L2",   256ull * 1024ull },   // 256 KB footprint
        { "L3",  4096ull * 1024ull }    // 4 MB footprint
    };

    const int NUM_LEVELS = (int)(sizeof(levels) / sizeof(levels[0]));

    printf("[08] Cache Bandwidth Test (L1I / L1D / L2 / L3)\n");
    printf("Total buffer: %.1f MiB, target traffic per level ≈ %.0f MiB\n\n",
           BUF_SIZE / 1024.0 / 1024.0,
           TARGET_BYTES / 1024.0 / 1024.0);

    printf("%-4s  %10s  %15s  %15s\n",
           "Lvl", "Size", "Read BW (GB/s)", "Write BW (GB/s)");
    printf("-------------------------------------------------------------\n");

    for (int i = 0; i < NUM_LEVELS; i++) {
        size_t sz = levels[i].size_bytes;
        if (sz > BUF_SIZE) sz = BUF_SIZE;

        double read_bw  = measure_read_bw(buf, sz);
        double write_bw = measure_write_bw(buf, sz);

        double sz_kb = sz / 1024.0;
        const char *unit = "KB";
        if (sz_kb >= 1024.0) {
            sz_kb /= 1024.0;
            unit = "MB";
        }

        printf("%-4s  %7.1f %2s  %15.3f  %15.3f\n",
               levels[i].name, sz_kb, unit, read_bw, write_bw);
    }

    free(buf);
    return 0;
}
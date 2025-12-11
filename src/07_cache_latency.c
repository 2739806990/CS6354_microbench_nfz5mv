// 07_cache_latency.c
// 测量 L1I / L1D / L2 / L3 的缓存访问延迟

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "harness.h"

// 计算中位数，用于降低测量抖动
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
    return (n % 2) ? a[n/2] : 0.5*(a[n/2] + a[n/2 - 1]);
}

/*  
   第一部分：测量 L1I 缓存延迟
*/
static double measure_L1I_latency(size_t code_bytes, double freq_GHz)
{
    size_t count = code_bytes / sizeof(uint32_t);
    if (count < 16) count = 16;

    // 分配一段“伪指令”缓冲区，制造 I-cache footprint
    uint32_t *code = aligned_alloc(64, count * sizeof(uint32_t));
    for (size_t i = 0; i < count; i++)
        code[i] = i;

    warmup_busy_loop(50000);

    uint64_t t0 = now_ns();

    volatile uint32_t acc = 0;
    size_t steps = 4 * 1024 * 1024;

    // 顺序访问 code[]，模拟取指行为
    for (size_t i = 0; i < steps; i++) {
        acc += code[i % count];
    }

    uint64_t t1 = now_ns();
    free(code);

    double ns = (double)(t1 - t0 - timer_overhead_ns());
    if (ns < 0) ns = 0;

    double cycles = ns * freq_GHz;
    return cycles / (double)steps;
}

/*
   第二部分：测量 L1D / L2 / L3 的缓存访问延迟
   使用 pointer chasing
*/

// 构建随机单向环，用于 pointer chasing
static void build_random_ring(uint32_t *buf, size_t len)
{
    uint32_t *tmp = malloc(len*sizeof(uint32_t));
    for (size_t i = 0; i < len; i++)
        tmp[i] = i;

    // 洗牌，打乱访问顺序
    for (size_t i = len - 1; i > 0; i--) {
        size_t j = rand() % (i + 1);
        uint32_t t = tmp[i];
        tmp[i] = tmp[j];
        tmp[j] = t;
    }

    // 构建环结构，形成依赖链
    for (size_t i = 0; i < len - 1; i++)
        buf[tmp[i]] = tmp[i + 1];
    buf[tmp[len - 1]] = tmp[0];

    free(tmp);
}

// 真实 pointer-chasing 延迟测量
static double measure_pointer_latency(size_t bytes, double freq_GHz)
{
    size_t len = bytes / sizeof(uint32_t);
    uint32_t *buf = aligned_alloc(64, bytes);

    build_random_ring(buf, len);
    warmup_busy_loop(50000);

    volatile uint32_t idx = 0;
    size_t steps = 4 * 1024 * 1024;

    uint64_t t0 = now_ns();
    for (size_t i = 0; i < steps; i++)
        idx = buf[idx]; 
    uint64_t t1 = now_ns();

    free(buf);

    double ns = (double)(t1 - t0 - timer_overhead_ns());
    if (ns < 0) ns = 0;

    double cycles = ns * freq_GHz;
    return cycles / (double)steps;
}

int main()
{
    double freq = 3.2;
    printf("[07] Cache Latency Test (L1I / L1D / L2 / L3)\n");
    printf("Assumed CPU freq = %.2f GHz\n\n", freq);

    // L1I：取指缓存延迟
    double l1i = measure_L1I_latency(32*1024, freq);

    // 数据侧：L1D / L2 / L3
    double l1d = measure_pointer_latency(32*1024, freq);
    double l2  = measure_pointer_latency(256*1024, freq);
    double l3  = measure_pointer_latency(4*1024*1024, freq);

    printf("L1I latency : %.2f cycles\n", l1i);
    printf("L1D latency : %.2f cycles\n", l1d);
    printf("L2  latency : %.2f cycles\n", l2);
    printf("L3  latency : %.2f cycles\n", l3);

    return 0;
}
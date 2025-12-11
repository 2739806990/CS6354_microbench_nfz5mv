// 02_fetch_throughput.c
// 实验目的：测试 CPU 的指令取指吞吐率（Instruction Fetch Throughput）
// 方法：通过运行展开的 NOP 循环（完全无依赖），估算每周期可取指数量。

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include "harness.h"

// 计算中位数
static double median(double *a, size_t n) {
    for (size_t i = 1; i < n; ++i) {
        double key = a[i];
        size_t j = i;
        while (j > 0 && a[j - 1] > key) {
            a[j] = a[j - 1];
            j--;
        }
        a[j] = key;
    }
    return (n % 2) ? a[n / 2] : 0.5 * (a[n/2 - 1] + a[n/2]);
}

// 定义一个 16-NOP block
#define NOP16 \
    "nop\n\tnop\n\tnop\n\tnop\n\t" \
    "nop\n\tnop\n\tnop\n\tnop\n\t" \
    "nop\n\tnop\n\tnop\n\tnop\n\t" \
    "nop\n\tnop\n\tnop\n\tnop\n\t"

// 8 × NOP16 = 128 NOP
#define NOP128 NOP16 NOP16 NOP16 NOP16 NOP16 NOP16 NOP16 NOP16

// 测试函数：每次循环执行 128 条 NOP
static double measure_fetch_throughput(size_t blocks) {
    const size_t REPEAT = 21;
    double samples[REPEAT];
    uint64_t t_oh = timer_overhead_ns();

    for (size_t r = 0; r < REPEAT; r++) {

        warmup_busy_loop(50000);

        uint64_t t0 = now_ns();

        for (size_t i = 0; i < blocks; i++) {
            __asm__ volatile(
                NOP128
            );
        }

        uint64_t t1 = now_ns();

        int64_t delta = (int64_t)t1 - (int64_t)t0 - (int64_t)t_oh;
        if (delta < 0) delta = 0;

        samples[r] = (double)delta / (double)(blocks * 128);
    }

    return median(samples, REPEAT);
}

int main(void) {
    const size_t BLOCKS = 400000;   // blocks × 128 ≈ 50M NOPs
    const double GHz = 3.2;

    printf("[02] Instruction Fetch Throughput Test (128 NOPs)\n");
    printf("Blocks: %zu  (each block = 128 NOPs)\n", BLOCKS);
    printf("Total NOP instructions ≈ %.0f\n", BLOCKS * 128.0);

    double ns_per_inst = measure_fetch_throughput(BLOCKS);

    double cycles = ns_per_inst * GHz;
    double ipc = 1.0 / cycles;

    printf("NOP latency per instruction : %.3f ns\n", ns_per_inst);
    printf("≈ %.3f cycles per instruction\n", cycles);
    printf("≈ %.3f instructions per cycle (fetch throughput)\n", ipc);

    return 0;
}
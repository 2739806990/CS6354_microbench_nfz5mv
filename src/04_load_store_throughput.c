// 04_load_store_throughput.c
// 实验目的：测试 CPU 的 Load（读）与 Store（写）吞吐率
// 方法：通过展开的无依赖内存访问循环，测量每周期可完成的读写操作数量。

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include "harness.h"

#define REPEAT 21  // 重复测量次数，用于取中位数减小波动

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

// 核心测试函数：测量读（load）或写（store）的吞吐率
// op_type = 0 表示 load，1 表示 store
// 每次循环展开 8 次独立操作，避免数据依赖造成的延迟
static double measure_mem_throughput(size_t iters, int op_type, double freq_GHz) {
    double samples[REPEAT];
    const uint64_t t_oh = timer_overhead_ns();  // 计时器开销

    for (int r = 0; r < REPEAT; ++r) {
        warmup_busy_loop(100000);  // CPU 预热，减少第一次波动

        volatile uint64_t mem[64] = {0};  // 模拟内存区域
        volatile uint64_t sink = 0;       // 防止编译器优化掉结果

        uint64_t t0 = now_ns();

        for (size_t i = 0; i < iters; ++i) {
            if (op_type == 0) {  // load 操作：从数组读取
                sink += mem[0]; sink += mem[1]; sink += mem[2]; sink += mem[3];
                sink += mem[4]; sink += mem[5]; sink += mem[6]; sink += mem[7];
            } else {  // store 操作：向数组写入
                mem[0] = i; mem[1] = i; mem[2] = i; mem[3] = i;
                mem[4] = i; mem[5] = i; mem[6] = i; mem[7] = i;
            }
        }

        uint64_t t1 = now_ns();
        int64_t delta = (int64_t)t1 - (int64_t)t0 - (int64_t)t_oh;
        if (delta < 0) delta = 0;

        int total_ops = iters * 8;             // 每轮执行的总操作数
        double time_ns = (double)delta;        // 总耗时 (ns)
        double time_cycles = time_ns * freq_GHz;  // 转换为时钟周期
        double ops_per_cycle = (double)total_ops / time_cycles;  // 每周期完成的操作数

        samples[r] = ops_per_cycle;
    }

    // 返回中位数结果
    return median(samples, REPEAT);
}

int main(void) {
    const size_t N = 10 * 1000 * 1000;  // 循环次数
    const double freq_GHz = 3.2;        // 假设主频 3.2GHz

    printf("[04] Load/Store Throughput Test\n");
    printf("Iterations: %zu\n\n", N);

    double load_tp = measure_mem_throughput(N, 0, freq_GHz);   // 测 load
    double store_tp = measure_mem_throughput(N, 1, freq_GHz);  // 测 store

    printf("Load Throughput : %.3f loads/cycle\n", load_tp);
    printf("Store Throughput: %.3f stores/cycle\n", store_tp);

    return 0;
}
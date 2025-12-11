// 04_load_store_throughput.c
// 加载/存储吞吐量测试 —— 使用完全独立的内存操作。
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include "harness.h"

#define REPEAT 21   // 重复采样次数，用于减小测量抖动

// 计算中位数函数
static double median(double *a, size_t n) {
    for (size_t i = 1; i < n; i++) {
        double key = a[i];
        size_t j = i;
        while (j > 0 && a[j - 1] > key) {
            a[j] = a[j - 1];
            j--;
        }
        a[j] = key;
    }
    return (n % 2) ? a[n/2] : 0.5 * (a[n/2] + a[n/2 - 1]);
}

// 核心测量逻辑：测试独立 load/store 指令的吞吐量
// iters: 循环次数
// op_type: 0 = LOAD, 1 = STORE
// freq: CPU 频率
static double measure_mem_throughput(size_t iters, int op_type, double freq) {
    double samples[REPEAT];
    uint64_t t_oh = timer_overhead_ns();  // 计时器开销

    for (int r = 0; r < REPEAT; r++) {
        warmup_busy_loop(50000);  // 预热，避免冷启动影响

        volatile uint64_t mem[64] = {0};  // 小型固定数组，避免跨 cache line 抖动
        volatile uint64_t sink = 0;       // 防止编译器优化

        uint64_t t0 = now_ns();

        for (size_t i = 0; i < iters; i++) {
            // 8 条完全独立的加载/存储操作
            uint64_t a,b,c,d,e,f,g,h;

            if (op_type == 0) {  // LOAD 测试
                a = mem[0];  b = mem[1];
                c = mem[2];  d = mem[3];
                e = mem[4];  f = mem[5];
                g = mem[6];  h = mem[7];
            } else {             // STORE 测试
                mem[0] = i; mem[1] = i;
                mem[2] = i; mem[3] = i;
                mem[4] = i; mem[5] = i;
                mem[6] = i; mem[7] = i;
            }

            // 防止编译器删除 load 指令（保持 a–h 的“使用”）
            sink = a + b + c + d + e + f + g + h;
        }

        uint64_t t1 = now_ns();
        double ns = (double)(t1 - t0 - t_oh);
        if (ns < 0) ns = 0;

        double cycles = ns * freq;  // 换算成周期数
        double ops = iters * 8;     // 每轮执行 8 个 load/store
        samples[r] = ops / cycles;  // ops per cycle
    }

    return median(samples, REPEAT);
}

int main() {
    const size_t N = 10 * 1000 * 1000;  // 测试长度
    const double freq = 3.2;            // 假定 CPU 频率（GHz）

    printf("[04] Load/Store Throughput Test (Independent Ops)\n\n");

    double load_tp  = measure_mem_throughput(N, 0, freq);
    double store_tp = measure_mem_throughput(N, 1, freq);

    printf("Load Throughput : %.3f loads/cycle\n", load_tp);
    printf("Store Throughput: %.3f stores/cycle\n", store_tp);

    return 0;
}
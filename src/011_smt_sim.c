// 011_smt_sim.c
// SMT 争用与协同效应实验模拟
// Apple SiliconM2不支持 SMT，本实验通过同时调度两个线程让它们运行不同类型的负载，从而模拟 SMT 下的资源争用与协同行为

#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "harness.h"

// 线程参数结构体，记录任务类型和运行时间
typedef struct {
    int type;          // 0 表示 ALU 密集型  1 表示内存密集型
    double seconds;    // 用于返回线程实际运行时间（秒）
} thread_arg;

#define LOOP 500000000ULL
#define MEM_SIZE (64 * 1024 * 1024)

//  工作负载定义 

// ALU 密集型工作（模拟执行单元竞争）
// 连续进行加法、异或、乘法等整数计算
void *run_alu(void *arg) {
    uint64_t start = now_ns();
    volatile uint64_t x = 0;

    for (uint64_t i = 0; i < LOOP; i++) {
        x += 1;
        x ^= 0x12345678;
        x *= 3;
        x += 7;
    }

    uint64_t end = now_ns();
    ((thread_arg*)arg)->seconds = (end - start) / 1e9;
    return NULL;
}

// 内存密集型工作（模拟缓存和内存带宽竞争）
void *run_mem(void *arg) {
    uint8_t *buf = aligned_alloc(64, MEM_SIZE);
    memset(buf, 1, MEM_SIZE);

    uint64_t start = now_ns();
    volatile uint64_t sum = 0;

    // 访问方式为固定步长，持续打满 L3/DRAM
    for (uint64_t i = 0; i < LOOP; i++) {
        sum += buf[(i * 64) & (MEM_SIZE - 1)];
    }

    uint64_t end = now_ns();
    free(buf);

    ((thread_arg*)arg)->seconds = (end - start) / 1e9;
    return NULL;
}


// 启动两个线程并测量总体执行时间
void launch_dual(const char *label, int typeA, int typeB)
{
    pthread_t t1, t2;
    thread_arg a = {.type = typeA}, b = {.type = typeB};

    printf("=== %s ===\n", label);

    uint64_t start = now_ns();

    // 创建两个线程并行运行不同的负载
    pthread_create(&t1, NULL, (typeA==0?run_alu:run_mem), &a);
    pthread_create(&t2, NULL, (typeB==0?run_alu:run_mem), &b);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    uint64_t end = now_ns();
    double total_sec = (end - start) / 1e9;

    printf("Thread1 time: %.3f s\n", a.seconds);
    printf("Thread2 time: %.3f s\n", b.seconds);
    printf("Total   time: %.3f s\n\n", total_sec);
}


int main() {
    printf("[11] SMT Contention & Symbiosis (Simulated)\n");
    printf("NOTE: Apple Silicon does NOT support SMT.\n");
    printf("This experiment simulates ALU vs MEM contention with co-scheduled threads.\n\n");

    // 基准测试：仅 ALU 工作负载
    thread_arg solo = {.type = 0};
    run_alu(&solo);
    printf("Baseline ALU alone: %.3f s\n\n", solo.seconds);

    // 两个 ALU 线程同时运行
    launch_dual("Case 1: ALU-heavy + ALU-heavy (Contention)", 0, 0);

    // 一个 ALU 线程 + 一个内存线程
    launch_dual("Case 2: ALU-heavy + Memory-heavy (Symbiosis)", 0, 1);

    return 0;
}
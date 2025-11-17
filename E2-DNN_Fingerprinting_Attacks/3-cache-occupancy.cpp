#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <string.h>

long read_count;

typedef uint32_t u32;

volatile int end = 1;
long gogo;

void pin_to_core(int core) {
    cpu_set_t cpuset;
    pthread_t thread;

    thread = pthread_self();

    CPU_ZERO(&cpuset);
    CPU_SET(core, &cpuset);

    pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
}

int get_hwmon(const char* path) {
    int ret;
    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        printf("error opening file: %s\n", path);
        return 0;
    }
    fscanf(fp, "%d", &ret);
    fclose(fp);
    return ret;
}

static inline uint64_t read_cycle_counter(void) {
    uint64_t cycles;
    asm volatile ("mrs %0, CNTVCT_EL0" : "=r" (cycles));
    return cycles;
}

typedef struct {
    char* input_path;
    char* output_path;
    long sampling_interval_us;  // 采样时间间隔，单位us
    int core_id;                // 传入的 core_id，用于绑定 CPU 核心
} thread_data_t;

static inline uint64_t read_cntvct_el0(void)
{
    uint64_t val;
    asm volatile("mrs %0, cntvct_el0" : "=r"(val));
    return val;
}

int svalue[1000000]={0};
#define MEM_SIZE (128 * 1024)
#define STRIDE 64

void *read_fun(void *thread_data) {
    thread_data_t *data = (thread_data_t *)thread_data;
    pin_to_core(data->core_id);  // 绑定到指定 CPU 核心
    FILE *fp = fopen(data->output_path, "w");
    if (fp == NULL) {
        perror("File open failed!\n");
        return NULL;
    }

        uint8_t *mem = (uint8_t *)malloc(MEM_SIZE);
    if (mem == NULL)
    {
        perror("malloc failed");
        return NULL;
    }
    for (size_t offset = 0; offset < MEM_SIZE; offset += STRIDE)
    {
        mem[offset] = (uint8_t)(offset / STRIDE);
    }

    struct timespec starttime, endtime;
    unsigned long count = 0;
    clock_gettime(CLOCK_MONOTONIC, &starttime);

    int i=0;
    uint64_t tsc_start, tsc_end, tsc_old, cnt=0;
    size_t value;
    tsc_start = read_cntvct_el0();
    tsc_end = read_cntvct_el0();
    tsc_old=tsc_end;
    while (end) {
        tsc_end = read_cntvct_el0();
        while (tsc_end - tsc_start < 500000)
        {
            cnt++;
            for (size_t offset = 0; offset < MEM_SIZE; offset += STRIDE)
            {
                value = mem[offset];
            }
            tsc_end = read_cntvct_el0();
        }
        svalue[i]=cnt;
        i++;
        cnt = 0;
        tsc_start = tsc_start+500000;
    }

    for(int j=0;j<i;j++){
        fprintf(fp, "%d\n", svalue[j]);
    }

    fclose(fp);
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if (argc < 5) {  // 增加 core_id 参数，所以需要 5 个参数
        fprintf(stderr, "Usage: %s <input path> <output path> <sampling interval us> <collection time us> <core_id>\n", argv[0]);
        return 1;
    }

    char* input_path = argv[1];
    char* output_path = argv[2];
    long sampling_interval_us = atol(argv[3]);  // 将采样时间间隔转换为长整型
    long collection_time_us = atol(argv[4]);    // 采集时间
    int core_id = atoi(argv[5]);                // 将 core_id 转换为整数

    pthread_t p;
    thread_data_t thread_data = {input_path, output_path, sampling_interval_us, core_id};
    pthread_create(&p, NULL, read_fun, &thread_data);
    sched_yield();

    usleep(collection_time_us);  // 使用传入的采集时间

    end = 0;
    pthread_join(p, NULL);
    return 0;
}

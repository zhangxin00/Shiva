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

typedef uint32_t u32;

int trace[10000000] = {0};

int group = 32;
#define MEM_SIZE (2 * 1024 * 1024)
#define STRIDE 64

#define CORE_RECEIVER 1

void pin_to_core(int core)
{
    cpu_set_t cpuset;
    pthread_t thread;

    thread = pthread_self();

    CPU_ZERO(&cpuset);
    CPU_SET(core, &cpuset);

    pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
}

int lock = 1;
void sigterm(int sig, siginfo_t *siginfo_t, void *context)
{
    lock = 0;
    return;
}

int set_signal(void)
{
    struct sigaction act;
    act.sa_sigaction = sigterm;
    act.sa_flags = SA_SIGINFO;
    return sigaction(SIGTERM, &act, NULL);
}

int main(int argc, char *argv[])
{
    if (argc < 4)
    {
        fprintf(stderr, "Usage: %s <send bits> <send time> <sampling interval us>\n", argv[0]);
        return 1;
    }

    int cycles = atol(argv[1]);
    int send_time = atol(argv[2]); // time per bit (unit: us)
    int sampling_interval_us = atol(argv[3]);

    pin_to_core(CORE_RECEIVER);

    set_signal();

    FILE *fp = fopen("trace.txt", "w");
    if (fp == NULL)
    {
        perror("File open failed!\n");
    }

    uint8_t *mem = (uint8_t *)malloc(MEM_SIZE);
    if (mem == NULL)
    {
        perror("malloc failed");
        return -1;
    }
    for (size_t offset = 0; offset < MEM_SIZE; offset += STRIDE)
    {
        mem[offset] = (uint8_t)(offset / STRIDE);
    }
    int cnt = 0;
    size_t value;
    size_t offset = 0;
    struct timespec starttime, endtime;
    int samples_per_bit = send_time / sampling_interval_us;
    int shift = 1400 / sampling_interval_us;
    while (lock)
    {
    };
    clock_gettime(CLOCK_MONOTONIC, &starttime);
    for (long i = 0; i < cycles * samples_per_bit + shift; i++)
    {
        clock_gettime(CLOCK_MONOTONIC, &endtime);
        while ((endtime.tv_sec - starttime.tv_sec) * 1000000L + (endtime.tv_nsec - starttime.tv_nsec) / 1000 < sampling_interval_us)
        {
            cnt++;

            for (int access_time = 0; access_time < group; access_time++)
            {
                value = mem[offset];
                offset = (offset + STRIDE) % MEM_SIZE;
            }

            clock_gettime(CLOCK_MONOTONIC, &endtime);
        }
        long duration = (endtime.tv_sec - starttime.tv_sec) * 1000000000L + (endtime.tv_nsec - starttime.tv_nsec);
        trace[i] = (cnt != 0) ? (duration / cnt) : 0;
        cnt = 0;
        starttime.tv_nsec += sampling_interval_us * 1000;
        if (starttime.tv_nsec >= 1000000000)
        {
            starttime.tv_sec += 1;
            starttime.tv_nsec -= 1000000000;
        }
    }

    int total = (cycles)*samples_per_bit;
    for (int num = shift; num < shift + total; num++)
    {
        fprintf(fp, "%d\n", trace[num]);
    }
    FILE *fp1 = fopen("receive_bits.txt", "w");
    double decode[cycles + 1];
    double sum = 0;
    double thresh = 0;
    int index = 0;

    int bit_max = 10000;

    int invalid = 0;
    for (int i = shift; i < total + shift; i++)
    {
        if (trace[i] <= bit_max && trace[i] != 0)
        {
            sum += trace[i];
        }
        else
        {
            invalid++;
        }
        if ((i + 1 - shift) % samples_per_bit == 0)
        {
            int valid = samples_per_bit - invalid;
            decode[index] = sum / valid;
            thresh += decode[index];
            sum = 0;
            invalid = 0;
            index++;
        }
    }
    thresh /= cycles;
    printf("thresh is %lf", thresh);
    FILE *fp2 = fopen("decode_raw.txt", "w");
    for (int num = 0; num < cycles; num++)
    {
        fprintf(fp1, "%d", decode[num] > thresh ? 1 : 0);
        fprintf(fp2, "%lf\n", decode[num]);
    }

    fclose(fp);
    fclose(fp1);
    fclose(fp2);
    return 0;
}

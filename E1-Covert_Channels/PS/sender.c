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
#include <errno.h>

#define HLS_REG_BASE_PHYS 0x43C00000
#define HLS_REG_MAP_SIZE 0x10000
#define REG_AP_CTRL 0x00
#define REG_DST_ADDR 0x10

#define RESV_BUF_PHY 0x1F000000
#define RESV_BUF_SIZE 0x00100000 // 1 MB

static inline void reg_write(volatile uint8_t *regs, off_t off, uint32_t val)
{
    *(volatile uint32_t *)(regs + off) = val;
}

static inline uint32_t reg_read(volatile uint8_t *regs, off_t off)
{
    return *(volatile uint32_t *)(regs + off);
}

volatile uint8_t *regs;

volatile uint32_t *buf;

void pl_write_ddr()
{
    reg_write(regs, REG_AP_CTRL, 0x01);

    uint32_t ap;
    do
    {
        ap = reg_read(regs, REG_AP_CTRL);
    } while ((ap & 0x2) == 0);
}

int main(int argc, char *argv[])
{
    /*
     * Program arguments:
     * argv[1] - Number of bits to send
     * argv[2] - Transmission time per bit (unit: us)
     */
    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s <number_of_bits> <time_per_bit(us)>\n", argv[0]);
        return 1;
    }
    const int send_bits = atol(argv[1]);
    const int send_time = atol(argv[2]);

    // prepare for the IP core
    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0)
    {
        perror("open /dev/mem");
        return -1;
    }

    regs = mmap(NULL, HLS_REG_MAP_SIZE,
                PROT_READ | PROT_WRITE, MAP_SHARED, fd, HLS_REG_BASE_PHYS);
    if (regs == MAP_FAILED)
    {
        perror("mmap regs");
        return -1;
    }

    buf = mmap(NULL, RESV_BUF_SIZE,
               PROT_READ | PROT_WRITE, MAP_SHARED, fd, RESV_BUF_PHY);
    if (buf == MAP_FAILED)
    {
        perror("mmap buf");
        return -1;
    }

    for (int i = 0; i <= 1000; i++)
    {
        buf[i] = 0;
    }

    reg_write(regs, REG_DST_ADDR, (uint32_t)RESV_BUF_PHY);

    int cycles = send_bits;

    int str[send_bits + 1];
    memset(str, 0, cycles);

    int i;

    for (i = 0; i < cycles; i++)
    {
        str[i] = rand() % 2;
    }

    struct timespec start, end;
    system("pkill -15 receiver");
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (i = 0; i < cycles; i++)
    {
        clock_gettime(CLOCK_MONOTONIC, &end);
        if (str[i] == 1)
        {
            while ((end.tv_sec - start.tv_sec) * 1000000 + (end.tv_nsec - start.tv_nsec) / 1000 < send_time)
            {
                pl_write_ddr();
                clock_gettime(CLOCK_MONOTONIC, &end);
            }
        }
        else if (str[i] == 0)
        {
            while ((end.tv_sec - start.tv_sec) * 1000000 + (end.tv_nsec - start.tv_nsec) / 1000 < send_time)
            {
                clock_gettime(CLOCK_MONOTONIC, &end);
            }
        }
        start.tv_nsec += (send_time * 1000);
        if (start.tv_nsec >= 1000000000)
        {
            start.tv_sec += 1;
            start.tv_nsec -= 1000000000;
        }
    }

    FILE *fp_write = fopen("./send_bits.txt", "w");
    for (i = 0; i < cycles; i++)
    {
        fprintf(fp_write, "%d", str[i]);
    }
    fclose(fp_write);

    munmap((void *)regs, HLS_REG_MAP_SIZE);
    munmap((void *)buf, RESV_BUF_SIZE);
    close(fd);

    return 0;
}

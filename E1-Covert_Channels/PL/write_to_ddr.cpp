#include <stdint.h>

#define LEN 1000

extern "C"
{

    void write_to_ddr(volatile uint32_t *dst_addr, volatile uint32_t *ctrl_reg)
    {
#pragma HLS INTERFACE m_axi port = dst_addr offset = slave bundle = gmem
#pragma HLS INTERFACE s_axilite port = dst_addr bundle = control
#pragma HLS INTERFACE s_axilite port = ctrl_reg bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control

        for (int i = 1; i <= LEN; i++)
        {
#pragma HLS PIPELINE II = 1
            dst_addr[i] = (uint32_t)(i);
        }

        uint32_t st = ctrl_reg[0];
        st |= (1 << 1);
        ctrl_reg[0] = st;
    }
}

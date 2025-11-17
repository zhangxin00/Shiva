# DPU Fingerprinting Attacks

The equipment used in this experiment is the Xilinx ZCU102 development board, featuring a Zynq UltraScale+ MPSoC FPGA.

This experiment uses the official Vitis AI pre-built SD card image, which can be downloaded from the following link: https://xilinx.github.io/Vitis-AI/3.0/html/docs/quickstart/mpsoc.html.

The experimental steps are as follows:

Prepare the DPU environment by following the instructions provided at https://xilinx.github.io/Vitis-AI/3.0/html/docs/quickstart/mpsoc.html.

Compile 1/2/3/4/5-XXX.cpp into an executable named gather using the command: gcc 1-XXX.cpp -o exp -lpthread.

Use command: sudo ./run.sh to run data collection and dpu inference. Note that some paths are required to be changed according to your system.
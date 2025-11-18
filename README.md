# Shiva

This repository contains the experiments of covert channel and DNN fingerprinting attacks discussed in the paper:
* "Shiva: Revealing and Exploiting Emerging Attack Surfaces in Heterogeneous ARM-FPGA SoCs"

Shiva is a novel remote side-channel attack on ARM-FPGA SoCs that elimimates the need of either co-located circuits or unprevileged sensors, identifying representative channels capturing CPU-level observables from shared-resource interactions and revealing meaningful leakage via interrupts and DRAM access latency.

## Tested Setup

### Software dependencies

To run the experiments, the following prerequisites need to be fulfilled:

* Linux installation
  * Build tools (gcc, make)
  * Python 3
  * Vivado

### Hardware dependencies

We evaluate Shiva on PYNQ-Z2 and Xilinx ZCU102 boards, with both connected to a monitoring system via UART for control and data collection. The PYNQ-Z2 board is used for covert channel evaluation (E1) and ZCU102 is used for DNN fingerprinting attacks (E2).

| Property          | PYNQ-Z2      | ZCU102           |
| ----------------- | ------------ | ---------------- |
| FPGA Family       | Zynq-7000    | Zynq UltraScale+ |
| FPGA Core Voltage | 0.95∼1.05V   | 0.825∼0.876V     |
| CPU Model         | Cortex-A9    | Cortex-A53       |
| Memory (DRAM)     | 512 MB       | 4 GB             |

## Materials

This repository contains the following materials:

* `E1-Covert_Channels`: contains customized DRAM write IP core, pre-built Petalinux images,  sender/receiver code, and tools to calculate covert channel BER (bit error rate) under different per-bit sending times.
* `E2-DNN_Fingerprinting_Attacks`: The experiment uses machine learning classifiers to correlate the collected side-channel traces with the underlying DPU model architectures, achieving high accuracy in identifying model architectures based on side-channel information.


# Covert Channels

The equipment used in this experiment is the PYNQ-Z2, which features a Zynq-7000 SoC.

## PL
The PL directory provides pre-built Petalinux images and source files for the customized DRAM write IP core: 
```
PL/
  |BOOT.BIN                     
  |image.ub                     
  |write_to_ddr.cpp       
```
### Step 1: Export Customized DRAM Write IP Core via Vivado HLS
To generate the customized DRAM write IP core, follow these steps using Vivado HLS:
1. Create a new HLS Project, select write_to_ddr as the top-level function, and add the write_to_ddr.cpp source file.
2. Set the Target Part to xc7z020clg400-1 (matching the PYNQ-Z2 board).
3. Run C Synthesis to verify the function logic.
4. Navigate to Solution -> Export RTL to export the IP core.
### Step 2: Integrate IP Cores in Vivado Project
In your Vivado project, integrate the exported HLS IP core and the ZYNQ7 Processing System IP core with the following configurations:
- For the ZYNQ7 Processing System IP core: Enable the S_AXI_HP0 port (for DDR access) and M_AXI_GP0 port (for register control).
- Connect the HLS IP core's m_axi interface to the PS HP port (enabling PL access to DDR) and its s_axilite interface to the GP port (allowing PS to write registers).
- In Vivado's Address Editor, locate the HLS IP core (typically named hls_write_to_ddr_0) and record the S_AXI base address (e.g., 0x43C00000, subject to actual project configuration).
### Step 3: Build Petalinux Image
To build a custom Petalinux image, modify the device tree to reserve physical memory for the PL IP core:
- Navigate to the device tree directory: project-spec/meta-user/recipes-bsp/device-tree/files/system-user.dtsi. Reserve a block of physical memory (e.g., 16 MB) for the PL IP core in the device tree.
### Direct Deployment (Recommended for PYNQ-Z2)
You can directly use the pre-built Petalinux images provided in the PL directory:
1. Copy BOOT.BIN and image.ub to the boot partition of your SD card.
2. Insert the SD card into the PYNQ-Z2 board and power it on— the images include the customized DRAM write IP core and pre-configured device tree, enabling out-of-the-box usage.


## PS

```
PS/
  |channel.sh             : A script used to observe the error rate of this covert channel under different sending times per bit.
  |error-rate.c           : Calculates the error rate between sent and received data.
  |receiver.cpp           : Determine the received bit value by calculating the average of the access DRAM latency for each bit transmission time.
  |sender.c               : Sends a sequence of bits by stressing the FPGA. It agrees on the number of bits to be sent and the sending time for each bit with the receiver.
```

Execute the above code on the ARM core of the PYNQ-Z2 through the `channel.sh` script to obtain the error rates under different sending times per bit. The results will be written to `result.txt`.

```shell
./channel.sh
```

The content in the `result.txt` file is roughly as follows:

```
...
per_bit_us: 260 us

real    0m0.336s
user    0m0.276s
sys     0m0.059s
error rate is:0.20 %
per_bit_us: 240 us

real    0m0.313s
user    0m0.273s
sys     0m0.040s
error rate is:0.70 %
...
```

- The time displayed after `per_bit_us` indicates the time per bit controlled by the sender.  The error rate shows the current bit error rate at the specified send time per bit.
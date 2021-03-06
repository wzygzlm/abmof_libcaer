/*
© Copyright 2013 - 2016 Xilinx, Inc. All rights reserved. 

This file contains confidential and proprietary information of Xilinx, Inc. and
is protected under U.S. and international copyright and other intellectual
property laws.

DISCLAIMER 
This disclaimer is not a license and does not grant any rights to the materials
distributed herewith. Except as otherwise provided in a valid license issued to
you by Xilinx, and to the maximum extent permitted by applicable law: (1) THESE
MATERIALS ARE MADE AVAILABLE "AS IS" AND WITH ALL FAULTS, AND XILINX HEREBY
DISCLAIMS ALL WARRANTIES AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY,
INCLUDING BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-INFRINGEMENT, OR
FITNESS FOR ANY PARTICULAR PURPOSE; and (2) Xilinx shall not be liable (whether
in contract or tort, including negligence, or under any other theory of
liability) for any loss or damage of any kind or nature related to, arising
under or in connection with these materials, including for any direct, or any
indirect, special, incidental, or consequential loss or damage (including loss
of data, profits, goodwill, or any type of loss or damage suffered as a result
of any action brought by a third party) even if such damage or loss was
reasonably foreseeable or Xilinx had been advised of the possibility of the
same.

CRITICAL APPLICATIONS
Xilinx products are not designed or intended to be fail-safe, or for use in any
application requiring fail-safe performance, such as life-support or safety
devices or systems, Class III medical devices, nuclear facilities, applications
related to the deployment of airbags, or any other applications that could lead
to death, personal injury, or severe property or environmental damage
(individually and collectively, "Critical Applications"). Customer assumes the
sole risk and liability of any use of Xilinx products in Critical Applications,
subject only to applicable laws and regulations governing limitations on product
liability.

THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS PART OF THIS FILE AT
ALL TIMES. 
*/

#ifndef ZERO_COPY_DM_H
#define ZERO_COPY_DM_H

#include "cf_lib.h"
#include "sysport_info.h"

#ifdef __cplusplus
extern "C" {
#endif

#define XLNK_ZERO_COPY_STAT_REG_READ(x) (x << 8)
#define XLNK_ZERO_COPY_STAT_REG_WRITE(x) (x << 16)

#define XLNK_ZERO_COPY_STAT_REG_NONE 1
#define XLNK_ZERO_COPY_STAT_REG_QUEUING 2
#define XLNK_ZERO_COPY_STAT_REG_IDLE 4
#define XLNK_ZERO_COPY_STAT_REG_NOCHECK 8

#define XLNK_ZERO_COPY_KEYHOLE 1

struct zero_copy_info_struct {
	accel_info_t *acc_info;
	uint64_t phys_base_addr;
	int status_reg_offset;
	int data_reg_offset;
	int config;
	sysport_info_t* data_sysport;
	int dir; // XLNK_{DMA_TO_DEV,DMA_FROM_DEV,BI_DIRECTIONAL}
        uint64_t cache;
	int dm_id;
	int max_done_issued;
	void* cache_virt_addr;
	void (*write_status_check)(void* base, int reg, int* wait_complete);
	void (*read_status_check)(void* base, int reg, int* data_valid, int* wait_complete);
};
typedef struct zero_copy_info_struct zero_copy_info_t;

int zero_copy_open (zero_copy_info_t* info);
int zero_copy_close (zero_copy_info_t* info);

int zero_copy_send_ref_i (cf_port_send_t *port,
		const void *buf,
		unsigned int len,
		cf_request_handle_t *request);

int zero_copy_send_i (cf_port_send_t *port,
		const void *buf,
		unsigned int len,
		cf_request_handle_t *request);

int zero_copy_recv (cf_port_receive_t *port,
		void *buf,
		unsigned int len,
		unsigned int *num_recd,
		cf_request_handle_t *request);

int zero_copy_recv_ref (cf_port_receive_t *port,
		void **buf,
		unsigned int *len,
		cf_request_handle_t *request);



#ifdef __cplusplus
};
#endif

#endif


// 67d7842dbbe25473c3c32b93c0da8047785f30d78e8a024de1b57352245f9689

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

#ifndef ACCEL_INFO_H
#define ACCEL_INFO_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct accel_info_struct {
	int device_id; /* ID of the uio device */
	uint64_t phys_base_addr; /* passed in to accel_register */
	int addr_range;
	char *ip_type; /* axis_accelerator_adapter or generic_axi_lite or other */
	void* virt_base_addr; /* this cannot be computed at compile time, and must be generated by a call to accel_open */
	int (*is_done)(void* v_addr, unsigned int done_offset);
	unsigned int done_reg_offset;
	int done_counter;
	
	int arg_dm_id[256];
	int arg_dm_id_count;
	char* func_name;
	int irq;
};
typedef struct accel_info_struct accel_info_t;

typedef struct accel_direct_connection_struct {
	accel_info_t* src;
	accel_info_t* dst;
	int dm_id;
} accel_direct_connection_t;

/* Fills resource dm_id for registering accelerator networks */
int accel_direct_connection_allocate(accel_direct_connection_t* dc);

/* accel_open opens the uio device and mmaps its base address to set the virtual_base_addr */
int accel_open(accel_info_t *accel_info);

/* accel_open closes the uio device and munmaps it */
void accel_close(accel_info_t *accel_info);

/* accel_wait waits for the accel to be done */
int accel_wait(void *info, int poll);

/* Check if adapter has space. Call before initiating transfer when necessary */
int accel_adapter_has_space(void *info, unsigned int offset);

/* accel_get_reg_info passes back the offsets related to the named register 
 * input 1 = void * pointer to the accel_info struct
 * input 2 = char * representing the register name
 * output 1 = offset of the register for reading/writing data 
 * output 2 = offset of the status register to check before writing data
 * output 3 = evaluation function to determine if writing is permitted
 * output 4 = offset of the status register to check before reading data
 * output 5 = evaluation function to determine if reading is permitted
 */
void accel_get_reg_info(void*, char*, int*,
	int*, void (**)(void*, int, int*),
	int*, void (**)(void*, int, int*, int*));

/* accel_get_start_seq passes back an int array representing the command sequence to use for starting the accelerator
 * input 1 = number of input scalars
 * input 2 = number of input arrays
 * input 3 = number of output scalars
 * input 4 = number of output arrays
 * input 5 = run mode - 0 for single execution and 1 for continous run
 * output 1 = int array representing the command sequence
 * output 2 = number of elements in the array of output 1
 */ 
void accel_get_start_seq(int , int , int , int , int , int **, int *);

/* accel_release_start_seq frees the array passed to the caller by accel_get_start_seq */
void accel_release_start_seq(int **);
#ifdef __cplusplus
};
#endif
#endif


// 67d7842dbbe25473c3c32b93c0da8047785f30d78e8a024de1b57352245f9689

/*
 * Copyright (c) 2016, ARM Limited and Contributors. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * Neither the name of ARM nor the names of its contributors may be used
 * to endorse or promote products derived from this software without specific
 * prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef __MTK_PLAT_COMMON_H__
#define __MTK_PLAT_COMMON_H__
#include <stdint.h>
/*******************************************************************************
 * Function and variable prototypes
 ******************************************************************************/
#define DEVINFO_SIZE 4
#define LINUX_KERNEL_32 0
#define SMC32_PARAM_MASK		(0xFFFFFFFF)

/*******************************************************************************
 * MTK struct
 ******************************************************************************/
typedef enum {
    BOOT_OPT_64S3 = 0,
    BOOT_OPT_64S1,
    BOOT_OPT_32S3,
    BOOT_OPT_32S1,
    BOOT_OPT_64N2,
    BOOT_OPT_64N1,
    BOOT_OPT_32N2,
    BOOT_OPT_32N1,
    BOOT_OPT_UNKNOWN
} boot_option_t;

#define DEVINFO_SIZE 4
#define ATF_AEE_BUFFER_SIZE             (0x4000)//16KB

typedef struct _atf_arg_t{
    unsigned int atf_magic;
    unsigned int tee_support;
    unsigned int tee_entry;
    unsigned int tee_boot_arg_addr;
    unsigned int hwuid[4];     // HW Unique id for t-base used  
    unsigned int HRID[2];      // HW random id for t-base used  
    unsigned int atf_log_port;
    unsigned int atf_log_baudrate;
    unsigned int atf_log_buf_start;
    unsigned int atf_log_buf_size;
    unsigned int atf_irq_num;
    unsigned int devinfo[DEVINFO_SIZE];
    unsigned int atf_aee_debug_buf_start;
    unsigned int atf_aee_debug_buf_size;
} atf_arg_t, *atf_arg_t_ptr;

extern unsigned int BL33_ARG0;
extern unsigned int BL33_ARG1;
extern unsigned int BL33_ARG2;
extern unsigned int BOOT_ARGUMENT_LOCATION;
extern unsigned int BOOT_ARGUMENT_SIZE;
extern unsigned int BL33_START_ADDRESS;
extern uintptr_t TEE_BOOT_INFO_ADDR;

struct atf_aee_regs {
    uint64_t    regs[31];
    uint64_t    sp;
    uint64_t    pc;
    uint64_t    pstate;
};

struct kernel_info {
	uint64_t pc;
	uint64_t r0;
	uint64_t r1;
	uint64_t r2;
	uint64_t k32_64;
};

struct mtk_bl_param_t {
	uint64_t bootarg_loc;
	uint64_t bootarg_size;
	uint64_t bl33_start_addr;
	uint64_t tee_info_addr;
};

/* boot reason */
#define BOOT_TAG_BOOT_REASON	0x88610001
struct boot_tag_boot_reason {
	uint32_t boot_reason;
};

struct boot_tag_plat_dbg_info {
	uint32_t info_max;
};

/* charger type info */
#define BOOT_TAG_IS_VOLT_UP      0x8861001A
struct boot_tag_is_volt_up {
	uint32_t is_volt_up;
};


struct boot_tag_header {
	uint32_t size;
	uint32_t tag;
};

struct boot_tag{
	struct boot_tag_header hdr;
	union {
		struct boot_tag_boot_reason boot_reason;
		struct boot_tag_plat_dbg_info plat_dbg_info;
		struct boot_tag_is_volt_up volt_info;
	} u;
};

typedef enum {
	BR_POWER_KEY = 0,
	BR_USB,
	BR_RTC,
	BR_WDT,
	BR_WDT_BY_PASS_PWK,
	BR_TOOL_BY_PASS_PWK,
	BR_2SEC_REBOOT,
	BR_UNKNOWN,
	BR_KERNEL_PANIC,
	BR_WDT_SW,
	BR_WDT_HW
} boot_reason_t;

#define boot_tag_next(t)    ((struct boot_tag *)((uint32_t *)(t) + (t)->hdr.size))
#define boot_tag_size(type)	((sizeof(struct boot_tag_header) + sizeof(struct type)) >> 2)

/* bit operations */
#define SET_BIT(_arg_, _bits_)					(uint32_t)((_arg_) |=  (uint32_t)(1 << (_bits_)))
#define CLEAR_BIT(_arg_, _bits_)				((_arg_) &= ~(1 << (_bits_)))
#define TEST_BIT(_arg_, _bits_)					((uint32_t)(_arg_) & (uint32_t)(1 << (_bits_)))
#define EXTRACT_BIT(_arg_, _bits_)				((_arg_ >> (_bits_)) & 1)
#define MASK_BITS(_msb_, _lsb_)					(((1U << ((_msb_) - (_lsb_) + 1)) - 1) << (_lsb_))
#define MASK_FIELD(_field_)						MASK_BITS(_field_##_MSB, _field_##_LSB)
#define EXTRACT_BITS(_arg_, _msb_, _lsb_)		((_arg_ & MASK_BITS(_msb_, _lsb_)) >> (_lsb_))
#define EXTRACT_FIELD(_arg_, _field_)			EXTRACT_BITS(_arg_, _field_##_MSB, _field_##_LSB)
#define INSERT_BIT(_arg_, _bits_, _value_)		((_value_) ? ((_arg_) |= (1 << (_bits_))) : ((_arg_) &= ~(1 << (_bits_))))
#define INSERT_BITS(_arg_, _msb_, _lsb_, _value_) \
				((_arg_) = ((_arg_) & ~MASK_BITS(_msb_, _lsb_)) | (((_value_) << (_lsb_)) & MASK_BITS(_msb_, _lsb_)))
#define INSERT_FIELD(_arg_, _field_, _value_)	INSERT_BITS(_arg_, _field_##_MSB, _field_##_LSB, _value_)

/* Declarations for mtk_plat_common.c */
uint32_t plat_get_spsr_for_bl32_entry(void);
uint32_t plat_get_spsr_for_bl33_entry(void);
void clean_top_32b_of_param(uint32_t smc_fid, uint64_t *x1,
				uint64_t *x2,
				uint64_t *x3,
				uint64_t *x4);
void bl31_prepare_kernel_entry(uint64_t k32_64);
void enable_ns_access_to_cpuectlr(void);
void boot_to_kernel(uint64_t x1, uint64_t x2, uint64_t x3, uint64_t x4);
uint64_t get_kernel_info_pc(void);
uint64_t get_kernel_info_r0(void);
uint64_t get_kernel_info_r1(void);
uint64_t get_kernel_info_r2(void);

extern  atf_arg_t gteearg;
#endif

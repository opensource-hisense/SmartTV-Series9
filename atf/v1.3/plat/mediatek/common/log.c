/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to MediaTek Inc. and/or its licensors. Without
 * the prior written permission of MediaTek inc. and/or its licensors, any
 * reproduction, modification, use or disclosure of MediaTek Software, and
 * information contained herein, in whole or in part, shall be strictly
 * prohibited.
 *
 * MediaTek Inc. (C) 2010. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER
 * ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 * NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH
 * RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 * INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES
 * TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
 * RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
 * OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK
 * SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE
 * RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S
 * ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE
 * RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE
 * MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE
 * CHARGE PAID BY RECEIVER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek
 * Software") have been modified by MediaTek Inc. All revisions are subject to
 * any receiver's applicable license agreements with MediaTek Inc.
 */

#include <arch.h>
#include <arch_helpers.h>
#include <debug.h>
#include <gic_common.h>
#include <log.h>
#include <mmio.h>
#include <mtk_plat_common.h>
#include <plat_private.h>
#include <platform.h>
#include <platform_def.h>
#include <string.h>
#include <stdint.h>

#define DEBUG_LOG_SERVICE 1
#if DEBUG_LOG_SERVICE
#define debug_print(...) tf_printf(__VA_ARGS__)
#else
#define debug_print(...) ((void)0)
#endif

extern void bl31_log_service_register(int (*lock_get)(),
	int (*log_putc)(unsigned char),
	int (*lock_release)());
extern void gicd_set_ispendr(uintptr_t, unsigned int);
extern atf_arg_t gteearg;

static unsigned int mt_log_buf_start = 0;
static unsigned int mt_log_buf_size = 0;
static unsigned int mt_log_buf_end = 0;
static unsigned int mt_log_sanity = 0;
unsigned int *mt_exception_buf_end = 0;
atf_log_ctrl_t *p_atf_log_ctrl = 0;
unsigned int mt_log_ktime_sync = 0;

/* Set ptr_atf_crash_flg to ATF_BAD_PTR instead of NULL pointer, this
 *  could prevent any unexpect access before .bss initialization. */
unsigned int *ptr_atf_crash_flag = (unsigned int *)ATF_BAD_PTR;
unsigned int *ptr_atf_crash_buf_size = 0;
unsigned long *ptr_atf_except_write_pos_per_cpu = 0;

#ifdef MTK_ATF_RAM_DUMP
uint64_t	atf_ram_dump_base;
uint64_t	atf_ram_dump_size;
spinlock_t	atf_ram_dump_lock;
#endif

#if !LOG_USE_SPIN_LOCK
DEFINE_BAKERY_LOCK(log_lock);
#endif
void mt_log_suspend_flush()
{
	if( mt_log_buf_size != 0 )
	{
		flush_dcache_range((uint64_t)mt_log_buf_start, (uint64_t)mt_log_buf_size);
	}
}

/* don't use any print function here in this function */
int mt_log_lock_acquire()
{
#if LOG_USE_SPIN_LOCK
	spin_lock(&(p_atf_log_ctrl->info.atf_buf_lock));
#else
	bakery_lock_get(&log_lock);
#endif
	return 0;
}

/* don't use any print function here in this function */
int mt_log_write(unsigned char c)
{
	*(unsigned char*)(uintptr_t)p_atf_log_ctrl->info.atf_write_pos = c;

	p_atf_log_ctrl->info.atf_total_write_count++;

	if( p_atf_log_ctrl->info.atf_write_pos < mt_log_buf_end )
		p_atf_log_ctrl->info.atf_write_pos++;
	else
		p_atf_log_ctrl->info.atf_write_pos = mt_log_buf_start + ATF_LOG_CTRL_BUF_SIZE;

	return 0;
}
static int mt_crash_log_dump_sanity(atf_log_ctrl_t *ctrl,uint32_t start, uint32_t size,uint32_t aee_buf_size)
{
	uint32_t atf_buf_size_sanity = size - ATF_LOG_CTRL_BUF_SIZE - aee_buf_size - ATF_CRASH_LAST_LOG_SIZE - ATF_EXCEPT_BUF_SIZE;
	uint32_t atf_buf_addr_sanity = start + ATF_LOG_CTRL_BUF_SIZE;
	if(ATF_LAST_LOG_MAGIC_NO != ctrl->info.atf_crash_flag && ATF_CRASH_MAGIC_NO != ctrl->info.atf_crash_flag)
		goto FAIL;
	if(ctrl->info.atf_buf_addr != atf_buf_addr_sanity)
		goto FAIL;
	if(ctrl->info.atf_buf_size != atf_buf_size_sanity)
		goto FAIL;
	if(ctrl->info.atf_crash_log_addr != (start + ATF_LOG_CTRL_BUF_SIZE) + (size - ATF_LOG_CTRL_BUF_SIZE - aee_buf_size - ATF_CRASH_LAST_LOG_SIZE - ATF_EXCEPT_BUF_SIZE))
		goto FAIL;
	if(ctrl->info.atf_write_pos < atf_buf_addr_sanity || ctrl->info.atf_write_pos > atf_buf_addr_sanity+atf_buf_size_sanity)
		goto FAIL;
	if(ctrl->info.atf_read_pos < atf_buf_addr_sanity || ctrl->info.atf_write_pos > atf_buf_addr_sanity+atf_buf_size_sanity)
		goto FAIL;
	return 1;
FAIL:
	INFO("atf RAW dump\n");
	return 0;

}
static int mt_crash_log_dump(uint8_t *crash_log_addr, uint32_t crash_log_size, uint32_t start, uint32_t size,uint32_t aee_buf_size)
{
	uint64_t ret = 0;
	uint64_t read_count;
	uint64_t offset;
	uint64_t write_count = p_atf_log_ctrl->info.atf_total_write_count;
	uint8_t *r_ptr = NULL;
	uint8_t *w_ptr = NULL;
	uint8_t *start_ptr = (uint8_t*)(uintptr_t)(mt_log_buf_start + ATF_LOG_CTRL_BUF_SIZE);

	/* Sanity check */
	if(!mt_crash_log_dump_sanity(p_atf_log_ctrl, start, size, aee_buf_size))
		return 0;
	if(write_count == 0)
		return 0;
	if(write_count > crash_log_size) {
		offset = crash_log_size-1;
		read_count = crash_log_size;
	} else {
		offset = write_count-1;
		read_count = write_count;
	}

	r_ptr = (uint8_t *)((uintptr_t)p_atf_log_ctrl->info.atf_write_pos);

	w_ptr = crash_log_addr + offset;
	while(read_count) {
		*(w_ptr--) = *(r_ptr--);
		if(r_ptr < start_ptr)
			r_ptr = (uint8_t*)(uintptr_t)mt_log_buf_end;
		read_count--;
		ret++;
	}

	return ret;
}
/*
static int mt_get_unread_log_size()
{
	if(p_atf_log_ctrl->info.atf_read_pos == p_atf_log_ctrl->info.atf_write_pos)
	{
		return 0;
	}
	else if(p_atf_log_ctrl->info.atf_read_pos < p_atf_log_ctrl->info.atf_write_pos)
	{
		return (p_atf_log_ctrl->info.atf_write_pos - p_atf_log_ctrl->info.atf_read_pos);
	}

	//Wrap around case
	return (p_atf_log_ctrl->info.atf_write_pos + p_atf_log_ctrl->info.atf_buf_size)
		- p_atf_log_ctrl->info.atf_read_pos;
}*/

/* don't use any print function here in this function */
int mt_log_lock_release()
{
	//unsigned int atf_buf_unread_size = mt_get_unread_log_size();

	//check if need to notify normal world to update
	/*
	if(atf_buf_unread_size >= ATF_LOG_SIGNAL_THRESHOLD_SIZE)
	{
		// Clean caches before re-entering normal world
		 dcsw_op_louis(DCCSW); //Level of Unification inner shareable
		 dcsw_op_all(DCCSW); //Flush all

		// Notify normal world
		if(p_atf_log_ctrl->info.atf_reader_alive)
		{
			//gicd_set_ispendr(BASE_GICD_BASE, ATF_LOG_IRQ_ID);
			p_atf_log_ctrl->info.atf_irq_count++;
		}
	}
	p_atf_log_ctrl->info.atf_buf_unread_size = atf_buf_unread_size;
	*/
	//release the lock
#if LOG_USE_SPIN_LOCK
	spin_unlock(&(p_atf_log_ctrl->info.atf_buf_lock));
#else
	bakery_lock_release(&log_lock);
#endif
	return 0;
}

/*uint32_t is_power_on_boot(void)
{
	uint32_t wdt_sta, wdt_interval;
	wdt_sta = mmio_read_32(MTK_WDT_STATUS);
	wdt_interval = mmio_read_32(MTK_WDT_INTERVAL);
	NOTICE("sta=0x%x int=0x%x\r\n", wdt_sta, wdt_interval);

	// Bit 2: IS_POWER_ON_RESET.
	// (bit2 will be set in preloader when reboot_from power on)
	if(wdt_interval & (1<<2))
		return 1;
	return 0;
}*/

void mt_log_set_crash_flag(void)
{
	p_atf_log_ctrl->info.atf_crash_flag = ATF_CRASH_MAGIC_NO;
}

unsigned int mt_log_get_crash_log_addr(void)
{
	if(!mt_log_sanity){
		return gteearg.atf_log_buf_start;
	}
	else{
		return p_atf_log_ctrl->info.atf_crash_log_addr;
	}
}

unsigned int mt_log_get_crash_log_size(void)
{
	if(!mt_log_sanity){
		return gteearg.atf_log_buf_size;
	}
	else{
		return p_atf_log_ctrl->info.atf_crash_log_size;
	}
}

unsigned int *mt_log_get_crash_flag_addr(void)
{
	return ptr_atf_crash_flag;
}

void mt_log_setup(unsigned int start, unsigned int size, unsigned int aee_buf_size, unsigned int is_abnormal_boot)
{
	uint32_t dump_ret=0;
	int i;
	uint32_t local_flag = 0;

	mt_log_buf_start = start;
	mt_log_buf_size = size;
	mt_log_buf_end = start + size - 1 - aee_buf_size - ATF_CRASH_LAST_LOG_SIZE - ATF_EXCEPT_BUF_SIZE;
	mt_exception_buf_end = (unsigned int*)(uintptr_t)(start + size - 1 - aee_buf_size);

	p_atf_log_ctrl = (atf_log_ctrl_t*)(uintptr_t)start;
#if !LOG_USE_SPIN_LOCK
	bakery_lock_init(&log_lock);
#endif
	// Initialize those two pointers for mt_console_core_putc
	ptr_atf_except_write_pos_per_cpu = &p_atf_log_ctrl->info.atf_except_write_pos_per_cpu[0];
	INFO("abnormal_boot: 0x%x, cflag: 0x%x\n", is_abnormal_boot, p_atf_log_ctrl->info.atf_crash_flag);

	if(!is_abnormal_boot || ATF_DUMP_DONE_MAGIC_NO == p_atf_log_ctrl->info.atf_crash_flag) {
		memset((void*)(uintptr_t)start, 0x0, size);
		p_atf_log_ctrl->info.atf_crash_flag = ATF_LAST_LOG_MAGIC_NO;
		p_atf_log_ctrl->info.atf_crash_log_size = ATF_CRASH_LAST_LOG_SIZE;
	} else {
		dump_ret = mt_crash_log_dump((uint8_t*)(uintptr_t)p_atf_log_ctrl->info.atf_crash_log_addr, ATF_CRASH_LAST_LOG_SIZE, start, size, aee_buf_size);
		if(dump_ret) {
			local_flag = p_atf_log_ctrl->info.atf_crash_flag;

			//
			// |--- Ctrl  ---|--- Ring Buffer ---|--- Crash Log -|-Except-|--- AEE buffer --|
			//      clean         clean               keep          keep        clean
			//
			memset((void*)(uintptr_t)start, 0x0,
					size-aee_buf_size-ATF_CRASH_LAST_LOG_SIZE - ATF_EXCEPT_BUF_SIZE);
			memset((void*)(uintptr_t)(start+(size-aee_buf_size)), 0x0, aee_buf_size);

			// write crash info back to control buffer
			if(ATF_LAST_LOG_MAGIC_NO == local_flag){
				p_atf_log_ctrl->info.atf_crash_log_size = ATF_CRASH_LAST_LOG_SIZE;
				p_atf_log_ctrl->info.atf_crash_flag = ATF_LAST_LOG_MAGIC_NO;
			}
			else if(ATF_CRASH_MAGIC_NO == local_flag){
				p_atf_log_ctrl->info.atf_crash_log_size = ATF_CRASH_LAST_LOG_SIZE + ATF_EXCEPT_BUF_SIZE;
				p_atf_log_ctrl->info.atf_crash_flag = ATF_CRASH_MAGIC_NO;
				INFO("p_atf_log_ctrl->info.atf_crash_flag = ATF_CRASH_MAGIC_NO\n");
			}
		}
		else{
			/* Can't pass ATF log sanity, dump entire atf log buffer */
			mt_log_sanity = 0;
			ptr_atf_crash_buf_size = &p_atf_log_ctrl->info.atf_crash_log_size;
			ptr_atf_crash_flag = &p_atf_log_ctrl->info.atf_crash_flag;
			flush_dcache_range(start, size);

			/* Don't modify Buffer and register information, just dump */
			return;
		}
	}

	p_atf_log_ctrl->info.atf_buf_addr = start + ATF_LOG_CTRL_BUF_SIZE;
	p_atf_log_ctrl->info.atf_buf_size = size - ATF_LOG_CTRL_BUF_SIZE - aee_buf_size -
									   ATF_CRASH_LAST_LOG_SIZE - ATF_EXCEPT_BUF_SIZE;
	p_atf_log_ctrl->info.atf_write_pos = p_atf_log_ctrl->info.atf_buf_addr;
	p_atf_log_ctrl->info.atf_read_pos = p_atf_log_ctrl->info.atf_buf_addr;
	p_atf_log_ctrl->info.atf_buf_lock.lock = 0;
	p_atf_log_ctrl->info.atf_crash_log_addr = (start + ATF_LOG_CTRL_BUF_SIZE) +
			(size - ATF_LOG_CTRL_BUF_SIZE - aee_buf_size -
			ATF_CRASH_LAST_LOG_SIZE - ATF_EXCEPT_BUF_SIZE);
	INFO("mt_log_setup\n");
	INFO(" -mt_log_buf_start: 0x%x\n", mt_log_buf_start);
	INFO(" -mt_log_buf_size: 0x%x\n", mt_log_buf_size);
	INFO(" -buf_addr: 0x%x\n", p_atf_log_ctrl->info.atf_buf_addr);
	INFO(" -buf_size: 0x%x\n", p_atf_log_ctrl->info.atf_buf_size);
	INFO(" -write_pos: 0x%x\n", p_atf_log_ctrl->info.atf_write_pos);
	INFO(" -read_pos: 0x%x\n", p_atf_log_ctrl->info.atf_read_pos);
	INFO(" -atf_buf_lock: 0x%x\n", p_atf_log_ctrl->info.atf_buf_lock.lock);
	INFO(" -log_buf_end : 0x%x\n", mt_log_buf_end);
	INFO(" -ATF_EXCEPT_BUF_SIZE_PER_CPU : 0x%x\n", ATF_EXCEPT_BUF_SIZE_PER_CPU);
	INFO(" -ATF_EXCEPT_BUF_SIZE : 0x%x\n", ATF_EXCEPT_BUF_SIZE);
	INFO(" -PLATFORM_CORE_COUNT : 0x%x\n", PLATFORM_CORE_COUNT);

	for(i=0; i<PLATFORM_CORE_COUNT; i++) {
		p_atf_log_ctrl->info.atf_except_write_pos_per_cpu[i] = (start+(size-aee_buf_size)-ATF_EXCEPT_BUF_SIZE)+ i*ATF_EXCEPT_BUF_SIZE_PER_CPU;
		INFO(" -atf_except_write_pos_per_cpu[%d]: 0x%lx\n", i,
			p_atf_log_ctrl->info.atf_except_write_pos_per_cpu[i]);
	}

	INFO(" -crash_flag : 0x%x\n", p_atf_log_ctrl->info.atf_crash_flag);
	INFO(" -crash_log_addr : 0x%x\n", p_atf_log_ctrl->info.atf_crash_log_addr);
	INFO(" -crash_log_size : 0x%x\n", p_atf_log_ctrl->info.atf_crash_log_size);

	bl31_log_service_register(&mt_log_lock_acquire, &mt_log_write,
		&mt_log_lock_release);
	mt_log_sanity = 1;
	ptr_atf_crash_buf_size = &p_atf_log_ctrl->info.atf_crash_log_size;
	ptr_atf_crash_flag = &p_atf_log_ctrl->info.atf_crash_flag;
	flush_dcache_range(start, size);
}


#define MT_LOG_SECURE_OS_BUFFER_MAX_LENGTH 120
#define TBASE_TAG "TBASE"
static unsigned char mt_log_secure_os_buf[MT_LOG_SECURE_OS_BUFFER_MAX_LENGTH+1] = {0};
static unsigned int mt_log_secure_os_pos = 0;

void mt_log_secure_os_print(int c)
{
	mt_log_secure_os_buf[mt_log_secure_os_pos] = c;

	//Force to flush the buffer if find end of line
	if( c == '\n' )
	{
		mt_log_secure_os_buf[mt_log_secure_os_pos+1] = '\0';
#if CFG_MICROTRUST_TEE_SUPPORT
	NOTICE("[%s]%s", "uTos", mt_log_secure_os_buf);
#else
	NOTICE("[%s]%s", TBASE_TAG, mt_log_secure_os_buf);
#endif
		mt_log_secure_os_pos = 0;
		return;
	}

	mt_log_secure_os_pos++;

	//Already reach the end of buffer, force to flush the buffer
	if( mt_log_secure_os_pos == MT_LOG_SECURE_OS_BUFFER_MAX_LENGTH )
	{
		mt_log_secure_os_buf[mt_log_secure_os_pos] = '\0';
#if CFG_MICROTRUST_TEE_SUPPORT
	NOTICE("[%s]%s", "uTos", mt_log_secure_os_buf);
#else

	NOTICE("[%s]%s", TBASE_TAG, mt_log_secure_os_buf);
#endif
		mt_log_secure_os_pos = 0;
	}
}

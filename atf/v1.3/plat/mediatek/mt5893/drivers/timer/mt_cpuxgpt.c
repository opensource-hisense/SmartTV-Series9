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

#include <stdint.h>
#include <arch_helpers.h>
#include <mmio.h>
#include <mt_cpuxgpt.h>
#include <stdint.h>
#include <platform.h>
#include <debug.h>


#define CPUXGPT_BASE	0x10078000
#define INDEX_BASE		(CPUXGPT_BASE+0x0674)
#define CTL_BASE		(CPUXGPT_BASE+0x0670)

uint64_t normal_time_base;
uint64_t atf_time_base;

static unsigned int __read_cpuxgpt(unsigned int reg_index)
{
	unsigned int value = 0;

	mmio_write_32(INDEX_BASE, reg_index);
	value = mmio_read_32(CTL_BASE);
	return value;
}

static void __write_cpuxgpt(unsigned int reg_index, unsigned int value)
{
	mmio_write_32(INDEX_BASE, reg_index);
	mmio_write_32(CTL_BASE, value);
}

static void set_cpuxgpt_clk(void)
{
	unsigned int freq = 0x16E3600; // 24MHz
	unsigned long voff = 0;
	// reset CNTVOFF_EL2 to 0, so the virtual counter will align with physical counter
	__asm__ __volatile__("MSR	 CNTVOFF_EL2, %0" : : "r" (voff));
	// set CNTFRQ to 24MHz
	__asm__ __volatile__("MSR	 CNTFRQ_EL0, %0" : : "r" (freq));
}

static void enable_cpuxgpt(void)
{
	unsigned int tmp = 0;

	tmp = __read_cpuxgpt(INDEX_CTL_REG);
	tmp |= EN_CPUXGPT;
	__write_cpuxgpt(INDEX_CTL_REG, tmp);

	INFO("CPUxGPT reg(%x)\n", __read_cpuxgpt(INDEX_CTL_REG));
}

void setup_syscnt(void)
{
	/* set cpuxgpt as free run mode */
	/* set cpuxgpt to Mhz clock */
	set_cpuxgpt_clk();
	/* enable cpuxgpt */
	enable_cpuxgpt();
}

void sched_clock_init(uint64_t normal_base, uint64_t atf_base)
{
	normal_time_base = normal_base;
	atf_time_base = atf_base;
}

uint64_t sched_clock(void)
{
	uint64_t cval;

	cval = (((read_cntpct_el0() - atf_time_base)*1000)/
		SYS_COUNTER_FREQ_IN_MHZ) + normal_time_base;
	return cval;
}

/*
  * Return: 0 - Trying to disable the CPUXGPT control bit,
  * and not allowed to disable it.
  * Return: 1 - reg_addr is not realted to disable the control bit.
  */
unsigned char check_cpuxgpt_write_permission(unsigned int reg_addr,
	unsigned int reg_value)
{
	unsigned int idx;
	unsigned int ctl_val;

	if (reg_addr == CTL_BASE) {
		idx = mmio_read_32(INDEX_BASE);

		/* idx 0: CPUXGPT system control */
		if (idx == 0) {
			ctl_val = mmio_read_32(CTL_BASE);
			if (ctl_val & 1) {
				/*
				 * if enable bit already set,
				 * then bit 0 is not allow to set as 0
				 */
				if (!(reg_value & 1))
					return 0;
			}
		}
	}
	return 1;
}


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

#include <gicv2.h>
#include <gic_v2.h>
#include <plat_arm.h>
#include <platform.h>
#include <platform_def.h>
#include <debug.h>

const unsigned int g0_interrupt_array[] = {
	PLAT_ARM_G0_IRQS
};

gicv2_driver_data_t arm_gic_data = {
	.gicd_base = BASE_GICD_BASE,
	.gicc_base = BASE_GICC_BASE,
	.g0_interrupt_num = ARRAY_SIZE(g0_interrupt_array),
	.g0_interrupt_array = g0_interrupt_array,
};

void plat_mt_gic_driver_init(void)
{
	gicv2_driver_init(&arm_gic_data);
}

void plat_mt_gic_init(void)
{
	gicv2_distif_init();
	gicv2_pcpu_distif_init();
	gicv2_cpuif_enable();
}

void plat_mt_gic_cpuif_enable(void)
{
	gicv2_cpuif_enable();
}

void plat_mt_gic_cpuif_disable(void)
{
	gicv2_cpuif_disable();
}

void plat_mt_gic_pcpu_init(void)
{
	gicv2_pcpu_distif_init();
}

uint64_t plat_mt_irq_dump_status(uint32_t irq)
{
	uint32_t dist_base;
	unsigned int bit;
	uint32_t result;
	uint64_t rc = 0;

	INFO("[ATF GIC dump] irq = %d\n", irq);

	dist_base = arm_gic_data.gicd_base;

	/* get mask */
	bit = 1 << (irq % 32);
	result = ((mmio_read_32(dist_base + GICD_ISENABLER + irq / 32 * 4) & bit)?1:0);
	INFO("[ATF GIC dump] enable = %x\n", result);
	rc |= result;

	/*get group*/
	bit = 1 << (irq % 32);
	//0x1:irq,0x0:fiq
	result = ((mmio_read_32(dist_base + GICD_IGROUPR + irq / 32 * 4) & bit)?1:0);
	INFO("[ATF GIC dump] group = %x (0x1:irq,0x0:fiq)\n", result);
	rc |=  result << 1;

	/* get priority */
	bit = 0xff << ((irq % 4)*8);
	result = ((mmio_read_32(dist_base + GICD_IPRIORITYR + irq / 4 * 4) & bit) >> ((irq % 4)*8));
	INFO("[ATF GIC dump] priority = %x\n", result);
	rc |= result << 2;

	/* get sensitivity */
	bit = 0x3 << ((irq % 16)*2);
	//edge:0x2, level:0x1
	result = ((mmio_read_32(dist_base + GICD_ICFGR + irq / 16 * 4) & bit) >> ((irq % 16)*2));
	INFO("[ATF GIC dump] sensitivity = %x (edge:0x1, level:0x0)\n", result>>1);
	rc |= result << 10;

	/* get pending status */
	bit = 1 << (irq % 32);
	result = ((mmio_read_32(dist_base + GICD_ISPENDR + irq / 32 * 4) & bit)?1:0);
	INFO("[ATF GIC dump] pending status = %x\n", result);
	rc |= result << 11;

	/* get active status */
	bit = 1 << (irq % 32);
	result = ((mmio_read_32(dist_base + GICD_ISACTIVER + irq / 32 * 4) & bit)?1:0);
	INFO("[ATF GIC dump] active status = %x\n", result);
	rc |= result << 12;

	/* get polarity */
	bit = 1 << (irq % 32);
	//0x0: high, 0x1:low
	result = ((mmio_read_32(INT_POL_CTL0 + (irq-32) / 32 * 4) & bit)?1:0);
	INFO("[ATF GIC dump] polarity = %x (0x0: high, 0x1:low)\n", result);
	rc |= result << 13;

	return rc;
}


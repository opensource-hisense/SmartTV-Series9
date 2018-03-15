/*
 * CPU kernel entry/exit control
 *
 * Copyright (C) 2013 ARM Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

//#include <asm/cpu_ops.h>
#include <asm/io.h>
#include <asm/smp_plat.h>
#include <linux/errno.h>
#include <linux/of.h>
#include <linux/string.h>
#include <linux/delay.h>
#include <linux/soc/mediatek/platform.h>

void bim_set_bit(unsigned char *addr, unsigned int bitmask)
{
	unsigned int regval32;

	regval32 = readl(addr);
	regval32 |= bitmask;
	writel(regval32, addr);
}

void bim_clear_bit(unsigned char *addr, unsigned int bitmask)
{
	unsigned int regval32;

	regval32 = readl(addr);
	regval32 &= ~bitmask;
	writel(regval32, addr);
}

void bim_wait_bit_high(unsigned char *addr, unsigned int bitmask)
{
	unsigned int regval32;

	do {
		regval32 = readl(addr);
	} while (!(regval32 & bitmask));
}

void bim_wait_bit_low(unsigned char *addr, unsigned int bitmask)
{
	unsigned int regval32;

	do {
		regval32 = readl(addr);
	} while (regval32 & bitmask);
}

static void __wait_cpu_wfi(unsigned int cpu,
                __le64 __iomem * map_addr)
{
    unsigned int regval32, loops = 0;
    unsigned char *reg_addr;

    reg_addr = map_addr;

    while (loops < 1000) {
        regval32 = readl(reg_addr + 0x34);
        if (regval32 & (1 << cpu))
            break;
        loops++;
        udelay(1000);
    }

    if (loops >= 1000)
        printk(KERN_ERR "CPU%u is not in WFI/WFE mode 0x%x\n", cpu,
               regval32);
}

#if defined(CONFIG_MACH_MT5891) || defined(CONFIG_MACH_MT5886) || defined(CONFIG_MACH_MT5863)
static void __ca53_cpu_power_off(unsigned char *cpu_power_addr)
{
	bim_set_bit(cpu_power_addr, CLAMP | SLPD_CLAMP);
	udelay(20);

	bim_set_bit(cpu_power_addr, MEM_CKISO);
	udelay(20);

	bim_set_bit(cpu_power_addr, MEM_PD);
	udelay(20);
	bim_wait_bit_high(cpu_power_addr, MEM_PD_ACK);

	bim_set_bit(cpu_power_addr, PWR_RST_EN);
	udelay(20);

	bim_set_bit(cpu_power_addr, PWR_CLK_DIS);
	udelay(20);

	bim_clear_bit(cpu_power_addr, PWR_ON);
	udelay(20);
	bim_wait_bit_low(cpu_power_addr, PWR_ON_ACK);

	bim_clear_bit(cpu_power_addr, PWR_ON_2ND);
	udelay(20);
	bim_wait_bit_low(cpu_power_addr, PWR_ON_2ND_ACK);
}

static void __ca53_cpu_power_on(unsigned char *cpu_power_addr)
{
	// check if cpu already in ON state
	if (!(readl(cpu_power_addr) & PWR_RST_EN))
		return;

	bim_set_bit(cpu_power_addr, PWR_ON);
	udelay(20);
	bim_wait_bit_high(cpu_power_addr, PWR_ON_ACK);

	bim_set_bit(cpu_power_addr, PWR_ON_2ND);
	udelay(20);
	bim_wait_bit_high(cpu_power_addr, PWR_ON_2ND_ACK);

	bim_clear_bit(cpu_power_addr, CLAMP | SLPD_CLAMP);
	udelay(20);

	bim_clear_bit(cpu_power_addr, MEM_PD);
	udelay(20);
	bim_wait_bit_low(cpu_power_addr, MEM_PD_ACK);

	bim_clear_bit(cpu_power_addr, MEM_CKISO);
	udelay(20);

	bim_clear_bit(cpu_power_addr, PWR_CLK_DIS);
	udelay(20);

	bim_clear_bit(cpu_power_addr, PWR_RST_EN);
}

static void __ca53_cpu_power_control(unsigned int cpu, unsigned int en)
{
	__le64 __iomem *map_addr;
	unsigned int val, offset;
	unsigned char *cpu_power_addr;

	if (cpu == 1) {
		offset = 0x8;
	} else if (cpu == 2) {
		offset = 0xc;
	} else if (cpu == 3) {
		offset = 0x10;
	} else {
		printk("cpu %d does not support power control\n", cpu);
		return;
	}

	map_addr = ioremap(0xf0008400, 0x40);
	if (!map_addr) {
		printk("ioremap failed.\n");
		return;
	}

	cpu_power_addr = map_addr;

	if (en == 1) {
		__ca53_cpu_power_on(cpu_power_addr + offset);
	} else {
		// check if cpu is in wfi
		__wait_cpu_wfi(cpu, map_addr);
		__ca53_cpu_power_off(cpu_power_addr + offset);
	}

	iounmap(map_addr);
}

#elif defined(CONFIG_ARCH_MT5893)
static void __ca73_cpu_power_on(unsigned char *cpu_power_addr, unsigned int cpu) {
    bim_set_bit(cpu_power_addr, PWR_ON << (3 * cpu));
    udelay(20);
    bim_wait_bit_high(cpu_power_addr, PWR_ACK << (3 * cpu));
}

static void __ca73_cpu_power_off(unsigned char *cpu_power_addr, unsigned int cpu) {
    bim_clear_bit(cpu_power_addr, PWR_ON << (3 * cpu));
    udelay(20);
    bim_wait_bit_low(cpu_power_addr, PWR_ACK << (3 * cpu));
}

static void __ca73_cpu_power_control(unsigned int cpu, unsigned int en)
{
    unsigned int val, offset;
    unsigned char *cpu_power_addr;

    cpu_power_addr = pBimIoMap + 0x400;

    if (en == 1) {
        __ca73_cpu_power_on(cpu_power_addr + REG_SMPC_PWR_CTL, cpu);
    } else {
        // check if cpu is in wfi
        __wait_cpu_wfi(cpu, cpu_power_addr);
        __ca73_cpu_power_off(cpu_power_addr + REG_SMPC_PWR_CTL, cpu);
    }
}
#endif

void cpu_power_control(unsigned int cpu, unsigned int en)
{
#if defined(CONFIG_MACH_MT5891) || defined(CONFIG_MACH_MT5886) || defined(CONFIG_MACH_MT5863)
	__ca53_cpu_power_control(cpu, en);
#elif defined(CONFIG_ARCH_MT5893)
    __ca73_cpu_power_control(cpu, en);
#endif
}

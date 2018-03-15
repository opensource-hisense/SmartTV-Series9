#ifndef __ASMARM_ARCH_SCU_H
#define __ASMARM_ARCH_SCU_H

#ifdef CONFIG_HAVE_ARM_SCU
#define SCU_PM_NORMAL	0
#define SCU_PM_DORMANT	2
#define SCU_PM_POWEROFF	3

#ifndef __ASSEMBLER__

#include <asm/cputype.h>

static inline bool scu_a9_has_base(void)
{
	return read_cpuid_part_number() == ARM_CPU_PART_CORTEX_A9;
}

static inline unsigned long scu_a9_get_base(void)
{
	unsigned long pa;

	asm("mrc p15, 4, %0, c15, c0, 0" : "=r" (pa));

	return pa;
}

unsigned int scu_get_core_count(void __iomem *);
int scu_power_mode(void __iomem *, unsigned int);

#ifdef CONFIG_SMP
void scu_enable(void __iomem *scu_base);
#else
static inline void scu_enable(void __iomem *scu_base) {}
#endif

#endif
#else
static inline unsigned int scu_get_core_count(void __iomem *scu_base)
{
	unsigned int ncores = 0;
#if defined(CONFIG_ARCH_MT5882) || defined(CONFIG_ARCH_MT5883) || defined(CONFIG_ARCH_MT5891) || defined(CONFIG_ARCH_MT5893) || defined(CONFIG_ARCH_MT5886) || defined(CONFIG_ARCH_MT5863)
	__asm__ ("MRC	p15, 1, %0, c9, c0, 2" : "=r" (ncores));
		ncores = (ncores >> 24);
#else

	__asm__ ("MRC   p15, 1, %0, c9, c0, 4" : "=r" (ncores));
#endif
	return (ncores & 0x03) + 1;
}
#endif

#endif

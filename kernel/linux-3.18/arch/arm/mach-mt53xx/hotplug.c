/*
 *  linux/arch/arm/mach-realview/hotplug.c
 *
 *  Copyright (C) 2002 ARM Ltd.
 *  All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/smp.h>
#include <linux/completion.h>

#include <mach/hardware.h>
#include <asm/delay.h>
#include <asm/smp_scu.h>
#include <asm/cp15.h>
#include <asm/cacheflush.h>
#include <asm/io.h>
#include <asm/smp_plat.h>

#include <mach/mt53xx_linux.h>
#include <mach/x_hal_io.h>

#if 1
#define debug_printk(log...)	do {if (!disable_printk_in_caller) {	\
	printk(log); }														\
} while(0)
#else
#define debug_printk(log,...)	do {} while(0)
#endif

extern volatile int pen_release;
//static DECLARE_COMPLETION(cpu_killed);
extern void __disable_dcache__inner_flush_dcache_L1__inner_clean_dcache_L2(void);	//definition in mt_cache_v7.S
extern void cpu_power_control(unsigned int cpu, unsigned int on);
extern char TZ_GCPU_FlushInvalidateDCache(void);

static inline void cpu_enter_lowpower(unsigned int cpu)
{
#if !defined(CONFIG_ARCH_MT5891) && !defined(CONFIG_ARCH_MT5893) && !defined(CONFIG_ARCH_MT5886) && !defined(CONFIG_ARCH_MT5863)
	unsigned int v;

#endif
	//HOTPLUG_INFO("cpu_enter_lowpower\n");
	TZ_GCPU_FlushInvalidateDCache();

	{
		__disable_dcache__inner_flush_dcache_L1__inner_clean_dcache_L2
		    ();

		udelay(2000);
		__asm__ __volatile__("clrex");	// Execute a CLREX instruction
#if !defined(CONFIG_ARCH_MT5891) && !defined(CONFIG_ARCH_MT5893) && !defined(CONFIG_ARCH_MT5886) && !defined(CONFIG_ARCH_MT5863)
		asm volatile ("	mrc	p15, 0, %0, c1, c0, 1\n" "	bic	%0, %0, %3\n"	// ACTLR.SMP
			      "	mcr	p15, 0, %0, c1, c0, 1\n":"=&r" (v)
			      :"r"(0), "Ir"(CR_C), "Ir"(0x40)
			      :"cc");
#endif
		isb();
	}
}

static inline void cpu_leave_lowpower(unsigned int cpu)
{
	unsigned int v;

#ifdef CONFIG_HAVE_ARM_SCU
	isb();
	dmb();
	dsb();
	isb();
	scu_power_mode(scu_base_addr(), SCU_PM_NORMAL);
#endif
	isb();
	dmb();
	dsb();
	isb();
	asm volatile ("	mrc	p15, 0, %0, c1, c0, 1\n" "	orr	%0, %0, %2\n"	// ACTLR.SMP
		      "	mcr	p15, 0, %0, c1, c0, 1\n" "	mrc	p15, 0, %0, c1, c0, 0\n" "	orr	%0, %0, %1\n"	// enable D cache
		      "	mcr	p15, 0, %0, c1, c0, 0\n":"=&r" (v)
		      :"Ir"(CR_C), "Ir"(0x40)
		      :"cc");
}

static inline void platform_do_lowpower(unsigned int cpu)
{
#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5882)||defined(CONFIG_ARCH_MT5865) || defined(CONFIG_ARCH_MT5891) || defined(CONFIG_ARCH_MT5893) || defined(CONFIG_ARCH_MT5886) || defined(CONFIG_ARCH_MT5863)
	//printk("(yjdbg) platform_do_lowpower: %d, ", cpu);
	cpu = cpu_logical_map(cpu);
	//printk("%d\n", cpu);
#endif
	/*
	 * there is no power-control hardware on this platform, so all
	 * we can do is put the core into WFI; this is safe as the calling
	 * code will have already disabled interrupts
	 */
	for (;;) {
		/*
		 * here's the WFI
		 */
		asm(".word	0xe320f003\n":
		:
		:"memory", "cc");

		if (pen_release == cpu) {
			/*
			 * OK, proper wakeup, we're done
			 */
			break;
		}

		/*
		 * getting here, means that we have come out of WFI without
		 * having been woken up - this shouldn't happen
		 *
		 * The trouble is, letting people know about this is not really
		 * possible, since we are currently running incoherently, and
		 * therefore cannot safely call printk() or anything else
		 */
#ifdef DEBUG
		printk("CPU%u: spurious wakeup call\n", cpu);
#endif
	}
}

int volatile _yjdbg_hotplug = 0;
int volatile _yjdbg_delay = 1;
int disable_printk_in_caller = 1;
int platform_cpu_kill(unsigned int cpu)
{
#if defined(CONFIG_ARCH_MT5890) ||defined(CONFIG_ARCH_MT5882) ||defined(CONFIG_ARCH_MT5865) || defined(CONFIG_ARCH_MT5891) || defined(CONFIG_ARCH_MT5893) || defined(CONFIG_ARCH_MT5886) || defined(CONFIG_ARCH_MT5863)
	debug_printk("(yjdbg) platform_cpu_kill: %d, ", cpu);
	cpu = cpu_logical_map(cpu);
	debug_printk("%d\n", cpu);

	if (cpu == 0) {
		printk(KERN_NOTICE "we should not turn off CPU%u\n", cpu);
		return 0;
	}

	if (!(((cpu >= 0) && (cpu < 4)) || ((cpu >= 256) && (cpu < 258)))) {
		printk(KERN_NOTICE "we can't handle CPU%u\n", cpu);
		return 0;
	}

	cpu_power_control(cpu, 0);

	if (_yjdbg_hotplug & 0x04) {
		int i = 0;
		for (i = 0; i < _yjdbg_delay; i++) {
			udelay(2000);
		}
	}

	if ((_yjdbg_hotplug & 0x02))
		debug_printk(KERN_NOTICE "CPU%u is killed.\n", cpu);
#endif // CONFIG_ARCH_MT5890

	return 1;
}

/*
 * platform-specific code to shutdown a CPU
 *
 * Called with IRQs disabled
 */
void __cpuinitdata platform_cpu_die(unsigned int cpu)	// access .cpuinit.data:pen_release; TODO
{
	unsigned int phyCpu;
#ifdef DEBUG
	unsigned int this_cpu = hard_smp_processor_id();
#endif

#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5882)||defined(CONFIG_ARCH_MT5865) || defined(CONFIG_ARCH_MT5891) || defined(CONFIG_ARCH_MT5893) || defined(CONFIG_ARCH_MT5886) || defined(CONFIG_ARCH_MT5863)
	debug_printk("(yjdbg) platform_cpu_die: %d, ", cpu);
	phyCpu = cpu;
	cpu = cpu_logical_map(cpu);
	debug_printk("%d\n", cpu);
#endif

#ifdef DEBUG
	if (cpu != this_cpu) {
		printk(KERN_CRIT
		       "Eek! platform_cpu_die running on %u, should be %u\n",
		       this_cpu, cpu);
		BUG();
	}
#endif

	//printk(KERN_NOTICE "CPU%u: shutdown-2\n", cpu);
	//complete(&cpu_killed);

	/*
	 * we're ready for shutdown now, so do it
	 */
	cpu_enter_lowpower(phyCpu);
	platform_do_lowpower(cpu);

	/*
	 * bring this CPU back into the world of cache
	 * coherency, and then restore interrupts
	 */
	cpu_leave_lowpower(phyCpu);
}

int platform_cpu_disable(unsigned int cpu)
{
	/*
	 * we don't allow CPU 0 to be shutdown (it is still too special
	 * e.g. clock tick interrupts)
	 */
	return cpu == 0 ? -EPERM : 0;
}

extern void __cpuinit platform_smp_prepare_cpus_wakeup(void);
extern void __cpuinit platform_smp_prepare_cpus_boot(void);
void hotplug_sw_init(void);

void __ref mt53xx_cpu_boot(unsigned int cpu)
{
#if defined(CONFIG_ARCH_MT5890) ||defined(CONFIG_ARCH_MT5882) ||defined(CONFIG_ARCH_MT5865) || defined(CONFIG_ARCH_MT5891) || defined(CONFIG_ARCH_MT5893) || defined(CONFIG_ARCH_MT5886) || defined(CONFIG_ARCH_MT5863)
	printk("(yjdbg) mt53xx_cpu_boot: %d\n", cpu);

#if 0				// def CONFIG_ARCH_MT5890
	cpu = cpu_logical_map(cpu);
#endif

	platform_smp_prepare_cpus_boot();

	if (!(((cpu >= 0) && (cpu < 4)) || ((cpu >= 256) && (cpu < 258)))) {
		printk(KERN_NOTICE "we can't handle CPU%u\n", cpu);
		return;
	}

	cpu_power_control(cpu, 1);
    hotplug_sw_init();

	printk(KERN_NOTICE "we turn on CPU%u\n", cpu);

	platform_smp_prepare_cpus_wakeup();
#endif // CONFIG_ARCH_MT5890
}



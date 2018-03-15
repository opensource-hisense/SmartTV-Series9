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

#if defined(CONFIG_ARCH_MT5891) || defined(CONFIG_ARCH_MT5886) || defined(CONFIG_ARCH_MT5893)
static void __iomem *mt53xx_cpu_debug[4];
#endif

void init_cpu_dbgpcsr(void) {
#if defined(CONFIG_ARCH_MT5891) || defined(CONFIG_ARCH_MT5886) || defined(CONFIG_ARCH_MT5893)
    int i;
    for (i = 0; i < 4; i++) {
        mt53xx_cpu_debug[i] = ioremap(MT53XX_DEBUG_PA_CPU(i), SZ_4K);
        writel(CPU_EDLAR_KEY, mt53xx_cpu_debug[i] + CPU_EDLAR);
        writel(0x0, mt53xx_cpu_debug[i] + CPU_OSLAR);
    }
#endif
}
EXPORT_SYMBOL(init_cpu_dbgpcsr);

// kernel always has this function, but you should add correct implementation to each IC.
void show_cpu_dbgpcsr(void) {
    #if defined(CONFIG_ARCH_MT5891) || defined(CONFIG_ARCH_MT5886) || defined(CONFIG_ARCH_MT5893)
    unsigned int dbgpcsrlo, dbgpcsrhi;
    int i;

    for_each_possible_cpu(i) {
        dbgpcsrlo = readl(mt53xx_cpu_debug[i] + CPU_DBGPCSRlo);
        dbgpcsrhi = readl(mt53xx_cpu_debug[i] + CPU_DBGPCSRhi);
        pr_err("CPU%d PC: 0x%x_%x\n", i, dbgpcsrhi, dbgpcsrlo);
    }
    #endif
}
EXPORT_SYMBOL(show_cpu_dbgpcsr);

static int panic_event(struct notifier_block *this, unsigned long event,
     void *ptr)
{
    printk("mt53xx panic_event\n");
    // fill the reboot code in pdwnc register
    #if defined(CC_CMDLINE_SUPPORT_BOOTREASON)
    #if defined(CONFIG_MACH_MT5890) || defined(CONFIG_ARCH_MT5891) || defined(CONFIG_ARCH_MT5893) || defined(CONFIG_ARCH_MT5886) || defined(CONFIG_ARCH_MT5863)
    __raw_writel((__raw_readl(pPdwncIoMap + REG_RW_RESRV5) & 0xFFFFFF00) | 0x01,pPdwncIoMap + REG_RW_RESRV5);
    #endif
    #endif

    show_cpu_dbgpcsr();

    return NOTIFY_DONE;
}

static struct notifier_block panic_block = {
    .notifier_call  = panic_event,
};

static int __init mt53xx_panic_setup(void)
{
    printk("mt53xx_panic_setup\n");
    init_cpu_dbgpcsr();
    /* Setup panic notifier */
    atomic_notifier_chain_register(&panic_notifier_list, &panic_block);

    return 0;
}
postcore_initcall(mt53xx_panic_setup);

int (*query_tvp_enabled) (void) = NULL;
EXPORT_SYMBOL(query_tvp_enabled);

// Print imprecise abort debug message according to BIM debug register
int mt53xx_imprecise_abort_report(void)
{
	u32 iobus_tout = __bim_readl(REG_IOBUS_TOUT_DBG);
	u32 dramf_tout = __bim_readl(REG_DRAMIF_TOUT_DBG);
	int print = 0;

	if (iobus_tout & IOBUS_TOUT) {
		print = 1;
		printk(KERN_ALERT "IOBUS %s addr 0xf00%05x timeout\n",
		       (iobus_tout & IOBUS_TOUT_WRITE) ? "write" : "read",
		       iobus_tout & IOBUS_TOUT_ADR_MSK);
	}

    #if !defined(CONFIG_MACH_MT5893) // Husky has no this register
	if (dramf_tout & 0x10) {
		print = 1;
		printk(KERN_ALERT "DRAM %s addr 0x%08x timeout\n",
		       (dramf_tout & 0x8) ? "write" : "read",
		       __bim_readl(REG_DRAMIF_TOUT_ADDR));
	}
    #endif
    
	//printk(KERN_ERR "query_tvp_enabled=0x%x,query_tvp_enabled=0x%x\n",(unsigned int)query_tvp_enabled,query_tvp_enabled());
	if (query_tvp_enabled && query_tvp_enabled()) {
		//printk(KERN_ALERT "MT53xx unknown imprecise abort\n");
		return 1;
	} else {
		if (!print)
			printk(KERN_ALERT
			       "MT53xx unknown imprecise abort\n");
	}
	return 0;
}


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
#if 0
//#include <asm/cpu_ops.h>
#include <asm/io.h>
#include <asm/smp_plat.h>
#include <linux/errno.h>
#include <linux/of.h>
#include <linux/string.h>
#include <linux/delay.h>
#include <linux/soc/mediatek/hardware.h>
//#include <linux/soc/mediatek/platform.h>
#include <linux/soc/mediatek/mt53xx_linux.h>
#include <asm/irq.h>
#endif

#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/bootmem.h>
#include <linux/slab.h>
#include <linux/seq_file.h>
#include <linux/sched.h>
#include <linux/dma-mapping.h>
#include <linux/soc/mediatek/hardware.h>
#include <linux/soc/mediatek/mt53xx_linux.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <asm/setup.h>
//#include <asm/mach-types.h>
#include <asm/pgtable.h>
#include <asm/page.h>
#include <asm/uaccess.h>
#include <linux/irq.h>
#include <asm/system_misc.h>
#include <linux/delay.h>

typedef void (*hotplug_sw_init_fn)(void);
hotplug_sw_init_fn hp_cb = NULL;

void hotplug_sw_init_register(hotplug_sw_init_fn cb)
{
#if defined(CONFIG_ARCH_MT5893)
    if (IS_IC_MT5893_ES1())
        hp_cb = cb;
#endif
}
EXPORT_SYMBOL(hotplug_sw_init_register);

void hotplug_sw_init(void)
{
#if defined(CONFIG_ARCH_MT5893)
    // we need delay for a while, when ROM code overwrite "three lines" code in SRAM
    if (IS_IC_MT5893_ES1()) {
        udelay(1000);

        if(hp_cb)
        {
            printk(KERN_NOTICE "hp sw init...\n");
            hp_cb();
        }
    }
#endif
}
EXPORT_SYMBOL(hotplug_sw_init);

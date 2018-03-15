/*
 * linux/arch/arm/mach-mt53xx/include/mach/mt53xx_linux.h
 *
 * Copyright (c) 2010-2012 MediaTek Inc.
 * $Author: dtvbm11 $
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 *
 */

#ifndef __MT53XX_LINUX_H__
#define __MT53XX_LINUX_H__

#include <linux/irq.h>

extern spinlock_t mt53xx_bim_lock;

/// Size of channel A DRAM size.
extern unsigned int mt53xx_cha_mem_size;

void mt53xx_get_reserved_area(unsigned long *start, unsigned long *size);


/* MT53xx family IC Check */
extern unsigned int mt53xx_get_ic_version(void);

#define IS_IC_MT5396()        ((mt53xx_get_ic_version() >> 16)==0x5396U)
#define IS_IC_MT5398()        ((mt53xx_get_ic_version() >> 16)==0x5398U)
#define IS_IC_MT5880()        ((mt53xx_get_ic_version() >> 16)==0x5880U)
#define IS_IC_MT5860()        ((mt53xx_get_ic_version() >> 16)==0x5860U)
#define IS_IC_MT5890()        ((mt53xx_get_ic_version() >> 16)==0x5890U)
#define IS_IC_MT5861()        ((mt53xx_get_ic_version() >> 16)==0x5861U)
#define IS_IC_MT5882()        ((mt53xx_get_ic_version() >> 16)==0x5882U)
#define IS_IC_MT5885()        ((mt53xx_get_ic_version() >> 16)==0x5885U)
#define IS_IC_MT5883()        ((mt53xx_get_ic_version() >> 16)==0x5883U)
#define IS_IC_MT5886()        ((mt53xx_get_ic_version() >> 16)==0x5886U)
#define IS_IC_MT5891()        ((mt53xx_get_ic_version() >> 16)==0x5891U)
#define IS_IC_MT5893()        ((mt53xx_get_ic_version() >> 16)==0x5893U)
#define IS_IC_MT5863()        ((mt53xx_get_ic_version() >> 16)==0x5863U)

#define IC_VER_5398_AA        0x53980000         // 5398 AA
#define IC_VER_5398_AB        0x53980001         // 5398 AB
#define IC_VER_5398_AC        0x53980002         // 5398 AC

#define IC_VER_5860_AA        0x58600000         // 5880 AA
#define IC_VER_5860_AB        0x58600001         // 5880 AB
#define IC_VER_5860_AC        0x58600002         // 5880 AC

#define IC_VER_MT5890_AA        0x58900001         // 5890 AA
#define IC_VER_MT5890_AB        0x58900002         // 5890 AB
#define IC_VER_MT5890_AC        0x58900003         // 5890 AC
#define IC_VER_MT5890_AD        0x58900004         // 5890 AD

#define IC_VER_MT5861_AA        0x58610001         // 5861 AA
#define IC_VER_MT5861_AB        0x58610002         // 5861 AB
#define IC_VER_MT5861_AC        0x58610003         // 5861 AC
#define IC_VER_MT5861_AD        0x58610004         // 5861 AD

#define IC_VER_MT5882_AA        0x58820001         // 5882 AA
#define IC_VER_MT5882_AB        0x58820002         // 5882 AB
#define IC_VER_MT5882_AC        0x58820003         // 5882 AC
#define IC_VER_MT5882_AD        0x58820004         // 5882 AD

#define IC_VER_MT5886_AA        0x58820001         // 5886 AA
#define IC_VER_MT5886_AB        0x58820002         // 5886 AB
#define IC_VER_MT5886_AC        0x58820003         // 5886 AC
#define IC_VER_MT5886_AD        0x58820004         // 5886 AD

#define IC_VER_MT5891_AA        0x58910001         // 5891 AA
#define IC_VER_MT5891_AB        0x58910002         // 5891 AB
#define IC_VER_MT5891_AC        0x58910003         // 5891 AC
#define IC_VER_MT5891_AD        0x58910004         // 5891 AD

#define IC_VER_MT5893_AA        0x58930001         // 5893 AA
#define IC_VER_MT5893_AB        0x58930002         // 5893 AB
#define IC_VER_MT5893_AC        0x58930003         // 5893 AC
#define IC_VER_MT5893_AD        0x58930004         // 5893 AD

#define IC_VER_MT5863_AA        0x58630001         // 5863 AA
#define IC_VER_MT5863_AB        0x58630002         // 5863 AB
#define IC_VER_MT5863_AC        0x58630003         // 5863 AC
#define IC_VER_MT5863_AD        0x58630004         // 5863 AD

#define IS_IC_MT5860_E2IC()   ((mt53xx_get_ic_version())==IC_VER_5860_AB)

#define IS_IC_MT5890_ES1()    ((mt53xx_get_ic_version())==IC_VER_MT5890_AA)
#define IS_IC_MT5890_MPS1()   ((mt53xx_get_ic_version())==IC_VER_MT5890_AB)
#define IS_IC_MT5890_ES3()    ((mt53xx_get_ic_version())==IC_VER_MT5890_AC)
#define IS_IC_MT5890_ES4()    ((mt53xx_get_ic_version())==IC_VER_MT5890_AD)

#define IS_IC_MT5861_ES1()    ((mt53xx_get_ic_version())==IC_VER_MT5861_AA)
#define IS_IC_MT5861_ES2()    ((mt53xx_get_ic_version())==IC_VER_MT5861_AB)

#define IS_IC_MT5882_ES1()    ((mt53xx_get_ic_version())==IC_VER_MT5882_AA)
#define IS_IC_MT5882_ES2()    ((mt53xx_get_ic_version())==IC_VER_MT5882_AB)

#define IS_IC_MT5886_ES1()    ((mt53xx_get_ic_version())==IC_VER_MT5886_AA)
#define IS_IC_MT5886_ES2()    ((mt53xx_get_ic_version())==IC_VER_MT5886_AB)
#define IS_IC_MT5886_ES3()    ((mt53xx_get_ic_version())==IC_VER_MT5886_AC)
#define IS_IC_MT5886_ES4()    ((mt53xx_get_ic_version())==IC_VER_MT5886_AD)

#define IS_IC_MT5891_ES1()    ((mt53xx_get_ic_version())==IC_VER_MT5891_AA)
#define IS_IC_MT5891_ES2()    ((mt53xx_get_ic_version())==IC_VER_MT5891_AB)
#define IS_IC_MT5891_ES3()    ((mt53xx_get_ic_version())==IC_VER_MT5891_AC)
#define IS_IC_MT5891_ES4()    ((mt53xx_get_ic_version())==IC_VER_MT5891_AD)

#define IS_IC_MT5893_ES1()    ((mt53xx_get_ic_version())==IC_VER_MT5893_AA)
#define IS_IC_MT5893_ES2()    ((mt53xx_get_ic_version())==IC_VER_MT5893_AB)
#define IS_IC_MT5893_ES3()    ((mt53xx_get_ic_version())==IC_VER_MT5893_AC)
#define IS_IC_MT5893_ES4()    ((mt53xx_get_ic_version())==IC_VER_MT5893_AD)

#define IS_IC_MT5863_ES1()    ((mt53xx_get_ic_version())==IC_VER_MT5863_AA)
#define IS_IC_MT5863_ES2()    ((mt53xx_get_ic_version())==IC_VER_MT5863_AB)
#define IS_IC_MT5863_ES3()    ((mt53xx_get_ic_version())==IC_VER_MT5863_AC)
#define IS_IC_MT5863_ES4()    ((mt53xx_get_ic_version())==IC_VER_MT5863_AD)


extern void bim_irq_ack(struct irq_data *data);

#endif /* __MT53XX_LINUX_H__ */


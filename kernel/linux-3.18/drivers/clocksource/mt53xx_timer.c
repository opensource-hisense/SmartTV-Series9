/*
 * Copyright 2012 Simon Arlott
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/bitops.h>
#include <linux/clockchips.h>
#include <linux/clocksource.h>
#include <linux/interrupt.h>
#include <linux/irqreturn.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/slab.h>
#include <linux/string.h>

#include <asm/irq.h>

//----------------------------------------------------------------------------
// Timer registers
#define REG_PDWNC_ACTIMER_LOW 0x0
#define REG_PDWNC_ACTIMER_HIGH 0x4

#define REG_RW_TIMER0_LLMT  0x0060	// RISC Timer 0 Low Limit Register
#define REG_T0LMT           0x0060	// RISC Timer 0 Low Limit Register
#define REG_RW_TIMER0_LOW   0x0064	// RISC Timer 0 Low Register
#define REG_T0              0x0064	// RISC Timer 0 Low Register
#define REG_RW_TIMER0_HLMT  0x0180	// RISC Timer 0 High Limit Register
#define REG_RW_TIMER0_HIGH  0x0184	// RISC Timer 0 High Register

#define REG_RW_TIMER1_LLMT  0x0068	// RISC Timer 1 Low Limit Register
#define REG_T1LMT           0x0068	// RISC Timer 1 Low Limit Register
#define REG_RW_TIMER1_LOW   0x006c	// RISC Timer 1 Low Register
#define REG_T1              0x006c	// RISC Timer 1 Low Register
#define REG_RW_TIMER1_HLMT  0x0188	// RISC Timer 1 High Limit Register
#define REG_RW_TIMER1_HIGH  0x018c	// RISC Timer 1 High Register

#define REG_RW_TIMER2_LLMT  0x0070	// RISC Timer 2 Low Limit Register
#define REG_T2LMT           0x0070	// RISC Timer 2 Low Limit Register
#define REG_RW_TIMER2_LOW   0x0074	// RISC Timer 2 Low Register
#define REG_T2              0x0074	// RISC Timer 2 Low Register
#define REG_RW_TIMER2_HLMT  0x0190	// RISC Timer 2 High Limit Register
#define REG_RW_TIMER2_HIGH  0x0194	// RISC Timer 2 High Register

#define REG_RW_TIMER_CTRL   0x0078	// RISC Timer Control Register
#define REG_TCTL            0x0078	// RISC Timer Control Register
#define TMR0_CNTDWN_EN  (1U << 0)	// Timer 0 enable to count down
#define TCTL_T0EN       (1U << 0)	// Timer 0 enable
#define TMR0_AUTOLD_EN  (1U << 1)	// Timer 0 audo load enable
#define TCTL_T0AL       (1U << 1)	// Timer 0 auto-load enable
#define TMR1_CNTDWN_EN  (1U << 8)	// Timer 1 enable to count down
#define TCTL_T1EN       (1U << 8)	// Timer 1 enable
#define TMR1_AUTOLD_EN  (1U << 9)	// Timer 1 audo load enable
#define TCTL_T1AL       (1U << 9)	// Timer 1 auto-load enable
#define TMR2_CNTDWN_EN  (1U << 16)	// Timer 2 enable to count down
#define TCTL_T2EN       (1U << 16)	// Timer 2 enable
#define TMR2_AUTOLD_EN  (1U << 17)	// Timer 2 audo load enable
#define TCTL_T2AL       (1U << 17)	// Timer 2 auto-load enable
#define TMR_CNTDWN_EN(x) (1U << (x*8))
#define TMR_AUTOLD_EN(x) (1U << (1+(x*8)))

#define REG_BIM_INTR_CLEAR  0x1B8

struct mt53xx_timer {
	void __iomem *control;
	void __iomem *compare;
	void __iomem *intr_clr;
	void __iomem *val_low;
	void __iomem *val_high;
	int match_mask;
	struct clock_event_device evt;
	struct irqaction act;
};

static void __iomem *system_clock_lo __read_mostly;
static void __iomem *system_clock_hi __read_mostly;
static void __iomem *pdwnc_clock_lo __read_mostly;
static void __iomem *pdwnc_clock_hi __read_mostly;

static void mt53xx_time_set_mode(enum clock_event_mode mode,
				 struct clock_event_device *evt_dev)
{
	unsigned long ctrl;
	struct mt53xx_timer *timer = container_of(evt_dev,
						  struct mt53xx_timer, evt);

	ctrl = readl_relaxed(timer->control);
	switch (mode) {
	case CLOCK_EVT_MODE_PERIODIC:
		ctrl |= (TCTL_T0EN | TCTL_T0AL);
		break;
	case CLOCK_EVT_MODE_ONESHOT:
		ctrl &= ~TCTL_T0AL;
		ctrl |= TCTL_T0EN;
		break;
	case CLOCK_EVT_MODE_UNUSED:
	case CLOCK_EVT_MODE_SHUTDOWN:
	case CLOCK_EVT_MODE_RESUME:
		ctrl &= ~(TCTL_T0EN | TCTL_T0AL);
		break;
	default:
		WARN(1, "%s: unhandled event mode %d\n", __func__, mode);
		break;
	}
	writel_relaxed(ctrl, timer->control);
}

static int mt53xx_time_set_next_event(unsigned long event,
				      struct clock_event_device *evt_dev)
{
	struct mt53xx_timer *timer = container_of(evt_dev,
						  struct mt53xx_timer, evt);
	writel_relaxed(readl_relaxed(timer->val_low) + event, timer->compare);
	writel_relaxed(readl_relaxed(timer->control) | TCTL_T0EN,
		       timer->control);
	return 0;
}

static void mt53xx_timer_clear_intr(int irq, void *dev_id)
{
#ifdef CC_NO_BIM_INTR
	struct mt53xx_timer *timer = dev_id;

	//if (irq == VECTOR_T0) {
	writel_relaxed(0x1, timer->intr_clr);
	//} else if (irq == VECTOR_T1) {
	//      writel_relaxed(0x2, timer->intr_clr);
	//} else if (irq == VECTOR_T2) {
	//      writel_relaxed(0x4, timer->intr_clr);
	//}
#endif /* CC_NO_BIM_INTR */
}

static irqreturn_t mt53xx_time_interrupt(int irq, void *dev_id)
{
	struct mt53xx_timer *timer = dev_id;
	void (*event_handler) (struct clock_event_device *);

	event_handler = ACCESS_ONCE(timer->evt.event_handler);
	if (event_handler)
		event_handler(&timer->evt);

	mt53xx_timer_clear_intr(irq, dev_id);

	return IRQ_HANDLED;
}

cycle_t mt53xx_clocksource_read(struct clocksource *c)
{
	unsigned long time_lo1;
	unsigned long time_hi1, time_hi2;
	u64 ret;

	do
	{
		time_hi1 = readl_relaxed(pdwnc_clock_hi);
		time_lo1 = readl_relaxed(pdwnc_clock_lo);
		time_hi2 = readl_relaxed(pdwnc_clock_hi);
	} while (time_hi1 != time_hi2);

	ret = (((unsigned long long)time_hi1 << 32) + (unsigned long long)time_lo1);
	return (cycle_t) ret;
}

static void __init mt53xx_timer_init(struct device_node *node)
{
	void __iomem *base;
	void __iomem *pdwnc_ti_base;
	u32 freq;
	int irq;
	struct mt53xx_timer *timer;

	base = of_iomap(node, 0);
	if (!base)
		panic("Can't remap registers");

	pdwnc_ti_base = of_iomap(node, 1); // check if support pdwnc ac counter

	if (of_property_read_u32(node, "clock-frequency", &freq))
		panic("Can't read clock-frequency");

	system_clock_lo = base + REG_RW_TIMER0_LOW;
	system_clock_hi = base + REG_RW_TIMER0_HIGH;

	if (pdwnc_ti_base) {
		printk("support pdwnc ac counter\n");
		pdwnc_clock_lo = pdwnc_ti_base + REG_PDWNC_ACTIMER_LOW;
		pdwnc_clock_hi = pdwnc_ti_base + REG_PDWNC_ACTIMER_HIGH;
		printk("support pdwnc ac counter: 0x%x,0x%x\n", pdwnc_clock_lo, pdwnc_clock_hi);
		clocksource_mmio_init(base + REG_RW_TIMER0_LOW, node->name,
				      freq, 300, 56, mt53xx_clocksource_read); // keep rating as 300, still use arch_sys_counter
	} else {
		printk("not support pdwnc ac counter\n");
		clocksource_mmio_init(base + REG_RW_TIMER0_LOW, node->name,
				      freq, 300, 32, clocksource_mmio_readl_up);
	}

	irq = irq_of_parse_and_map(node, 0);

	if (irq <= 0)
		panic("Can't parse IRQ");

	timer = kzalloc(sizeof(*timer), GFP_KERNEL);
	if (!timer)
		panic("Can't allocate timer struct\n");

	//printk("mt53xx_timer dbg: base: 0x%x irq %d freq: %d\n", base, irq, freq);

	timer->control = base + REG_RW_TIMER_CTRL;
	timer->compare = base + REG_RW_TIMER0_LLMT;
	timer->intr_clr = base + REG_BIM_INTR_CLEAR;
	timer->val_low = base + REG_RW_TIMER0_LOW;
	timer->val_high = base + REG_RW_TIMER0_HIGH;
	timer->match_mask = BIT(0);
	timer->evt.name = node->name;
	timer->evt.rating = 300;
	timer->evt.features = CLOCK_EVT_FEAT_PERIODIC | CLOCK_EVT_FEAT_ONESHOT;
	timer->evt.set_mode = mt53xx_time_set_mode;
	timer->evt.set_next_event = mt53xx_time_set_next_event;
	timer->evt.cpumask = cpumask_of(0);
	timer->act.name = node->name;
	timer->act.flags = IRQF_TIMER | IRQF_SHARED;
	timer->act.dev_id = timer;
	timer->act.handler = mt53xx_time_interrupt;

	if (setup_irq(irq, &timer->act))
		panic("Can't set up timer IRQ\n");

	clockevents_config_and_register(&timer->evt, freq, 0xf, 0xffffffff);

	pr_info("mt53xx: system timer (irq = %d)\n", irq);
}

CLOCKSOURCE_OF_DECLARE(mt53xx, "mediatek,mt53xx-system-timer",
		       mt53xx_timer_init);

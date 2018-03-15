/*
 * clock scaling for the MT53xx
 *
 * Code specific to PKUnity SoC and UniCore ISA
 *
 *	Maintained by GUAN Xue-tao <gxt@mprc.pku.edu.cn>
 *	Copyright (C) 2001-2010 Guan Xuetao
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/clk.h>
#include <linux/cpufreq.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/pm_opp.h>
#include <linux/regulator/consumer.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/of_device.h>

extern void __iomem *pCkgenIoMap;
extern void __iomem *p_hpcpll_iomap;

#define CPU_WRITE32(offset, _val_)     (*((volatile unsigned int*)(pCkgenIoMap + offset)) = (_val_))
#define CPU_READ32(offset)             (*((volatile unsigned int*)(pCkgenIoMap + offset)))

#define HPCPLL_WRITE32(offset, _val_)     (*((volatile unsigned int*)(p_hpcpll_iomap + offset)) = (_val_))
#define HPCPLL_READ32(offset)             (*((volatile unsigned int*)(p_hpcpll_iomap + offset)))


struct mt53xx_dvfs_data {
    struct device *dev;
    struct regulator *proc_reg;
    struct cpufreq_frequency_table *freq_table;
    struct notifier_block opp_nb;
    struct mutex lock;
    struct list_head list_head;
    unsigned int transition_latency;
};

static LIST_HEAD(dvfs_info_list);
static struct mt53xx_dvfs_data *dvfs_info;

#define VOLT_TOL		(10000)

//=== Start of CLK control ===
#define CKGEN_ANA_PLLGP_CFG0 (0x5e0)
#define CKGEN_ANA_PLLGP_CFG2 (0x5e8)
#define CKGEN_ANA_PLLGP_CFG3 (0x5ec)

void mt53xx_set_hw_cpupll(unsigned int target_freq)
{
    unsigned int u4TargetFreq_CW = 0, u4StepFreq_CW = 0, freq_factor;

    freq_factor = target_freq / 1000 / 12;

    HPCPLL_WRITE32(0x0, (HPCPLL_READ32(0x0) & 0x83DFFFFF) | 0x10200000);

    CPU_WRITE32(CKGEN_ANA_PLLGP_CFG2, CPU_READ32(CKGEN_ANA_PLLGP_CFG2) & ~(1U << 1));
    CPU_WRITE32(CKGEN_ANA_PLLGP_CFG2, CPU_READ32(CKGEN_ANA_PLLGP_CFG2) | (0xFFFFFFU << 8) | (1U << 0));
    CPU_WRITE32(CKGEN_ANA_PLLGP_CFG3, freq_factor << 24);
    // for reading updated clk, we update CFG0 at the same time
    CPU_WRITE32(CKGEN_ANA_PLLGP_CFG0, CPU_READ32(CKGEN_ANA_PLLGP_CFG0) & ~(0x7F000000U) | (freq_factor << 24));
    udelay(2);
    CPU_WRITE32(CKGEN_ANA_PLLGP_CFG2, CPU_READ32(CKGEN_ANA_PLLGP_CFG2) | (1U << 1));    
}

static unsigned int mt53xx_get_hw_cpupll(void) {
    unsigned int freq;

    freq = ((CPU_READ32(CKGEN_ANA_PLLGP_CFG0) >> 24) & 0x7F) * 12 * 1000;
    return freq;
}
//=== End of CLK control ===

struct device *get_mt_cpu_device(unsigned cpu)
{
    return dvfs_info->dev;
}

static int mtk_cpufreq_set_voltage(struct mt53xx_dvfs_data *info, int vcpu)
{
	int ret = 0;

	ret = regulator_set_voltage(info->proc_reg, vcpu, vcpu + VOLT_TOL);
			
	return ret;
}

static int mtk_cpufreq_opp_notifier(struct notifier_block *nb,
unsigned long event, void *data)
{
	struct dev_pm_opp *opp = data;
//	struct mt53xx_dvfs_data *info = container_of(nb, struct mt53xx_dvfs_data, opp_nb);
	unsigned long freq, volt;
	int ret = 0;

//	printk("[MT53xx-cpufreq]: %s %d\n", __func__,event);
	if (event == OPP_EVENT_ADJUST_VOLTAGE) {
		rcu_read_lock();
		freq = dev_pm_opp_get_freq(opp)/1000;
		rcu_read_unlock();

		mutex_lock(&dvfs_info->lock);
		if (mt53xx_get_hw_cpupll() == freq) {
			rcu_read_lock();
			volt = dev_pm_opp_get_voltage(opp);
			rcu_read_unlock();
#if 1
			printk("volt: %d , freq: %d \n", volt,freq);
			
#endif
			ret = mtk_cpufreq_set_voltage(dvfs_info, volt);
			if (ret)
				dev_err(dvfs_info->dev, "failed to scale voltage: %d\n", ret);
		}
		mutex_unlock(&dvfs_info->lock);
	}

	return notifier_from_errno(ret);
}

int mt53xx_cpufreq_verify_speed(struct cpufreq_policy *policy)
{
	return 0;
}

static unsigned int mt53xx_cpufreq_getspeed(unsigned int cpu)
{
	return mt53xx_get_hw_cpupll();	
}

static int mt53xx_cpufreq_target(struct cpufreq_policy *policy,
			 unsigned int target_freq,
			 unsigned int relation)
{
#if 1
    struct dev_pm_opp *opp;
	unsigned int vol;
	long freq;
	int old_vol;

	freq = target_freq * 1000; //kHz to Hz, for opp table

    rcu_read_lock();
	opp = dev_pm_opp_find_freq_floor(dvfs_info->dev, &freq);
    if (IS_ERR(opp)) {
        rcu_read_unlock();
        dev_err(dvfs_info->dev,
            "failed to find valid OPP for %u KHZ\n",
            target_freq);
        return PTR_ERR(opp);
    }

	mutex_lock(&dvfs_info->lock);
	vol = dev_pm_opp_get_voltage(opp);
	old_vol = regulator_get_voltage(dvfs_info->proc_reg);
//	printk("vol: %d uV, old_vol: %d uV\n", vol,old_vol);

	if(vol > old_vol)//set voltage before freq
	{
		mtk_cpufreq_set_voltage(dvfs_info, vol);
		mdelay(10); //wait voltage stable time
		mt53xx_set_hw_cpupll(target_freq);
		policy->cur = mt53xx_get_hw_cpupll();
	}
	else//set freq before voltage
	{
		mt53xx_set_hw_cpupll(target_freq);
		policy->cur = mt53xx_get_hw_cpupll();
		mdelay(10);//wait freq stable time
		mtk_cpufreq_set_voltage(dvfs_info, vol);
	}
	mutex_unlock(&dvfs_info->lock);
	rcu_read_unlock();
#else
    struct dev_pm_opp *opp;
	long freq;
	unsigned int vol;

	freq = target_freq * 1000; //kHz to Hz, for opp table
	
	rcu_read_lock();
	opp = dev_pm_opp_find_freq_floor(dvfs_info->dev, &freq);
    if (IS_ERR(opp)) {
        rcu_read_unlock();
        dev_err(dvfs_info->dev,
            "failed to find valid OPP for %u KHZ\n",
            target_freq);
        return PTR_ERR(opp);
    }
	
	mutex_lock(&dvfs_info->lock);
	vol = dev_pm_opp_get_voltage(opp);
	if(target_freq > mt53xx_get_hw_cpupll())
	{
		mtk_cpufreq_set_voltage(dvfs_info, vol);
		mdelay(10); //wait voltage stable time
		mt53xx_set_hw_cpupll(target_freq);
		policy->cur = mt53xx_get_hw_cpupll();
	}
	else
	{
		mt53xx_set_hw_cpupll(target_freq);
		policy->cur = mt53xx_get_hw_cpupll();
		mdelay(10);//wait freq stable time
		mtk_cpufreq_set_voltage(dvfs_info, vol);
	}
	mutex_unlock(&dvfs_info->lock);
	rcu_read_unlock();
#endif

	return 0;
}

static int __init mt53xx_cpufreq_init(struct cpufreq_policy *policy)
{
	cpufreq_generic_init(policy, dvfs_info->freq_table,
            dvfs_info->transition_latency);

	return 0;
}

static int mt53xx_cpufreq_exit(struct cpufreq_policy *policy)
{
    return 0;
}

static struct cpufreq_driver mt53xx_cpufreq_driver = {
	.flags		= CPUFREQ_STICKY,
	.verify		= mt53xx_cpufreq_verify_speed,
	.target		= mt53xx_cpufreq_target,
	.get		= mt53xx_cpufreq_getspeed,
	.init		= mt53xx_cpufreq_init,
    .exit       = mt53xx_cpufreq_exit,
	.name		= "mediatek-cpufreq",
};


static const struct of_device_id mt53xx_cpufreq_match[] = {
    {
        .compatible = "mediatek,mt53xx-cpufreq",
    },
    {},
};
MODULE_DEVICE_TABLE(of, mt53xx_cpufreq_match);

static int mt53xx_cpufreq_probe(struct platform_device *pdev)
{
	int ret = 0;
    struct device_node *np;
	struct regulator *proc_reg = ERR_PTR(-ENODEV);
	struct srcu_notifier_head *opp_srcu_head;

    dvfs_info = devm_kzalloc(&pdev->dev, sizeof(*dvfs_info), GFP_KERNEL);
    if (!dvfs_info) {
        ret = -ENOMEM;
        goto err_put_node;
    }

    dvfs_info->dev = &pdev->dev;
    mutex_init(&dvfs_info->lock);

    // get clock transition
    np = of_cpu_device_node_get(0);
    if (!np) {
        dev_err(dvfs_info->dev, "No cpu node found");
        ret = -ENODEV;
        goto err_put_node;
    }

    if (of_property_read_u32(np, "clock-latency",
                &dvfs_info->transition_latency))
        dvfs_info->transition_latency = CPUFREQ_ETERNAL;

    of_node_put(np);

    ret = of_init_opp_table(dvfs_info->dev);
    if (ret) {
        dev_err(dvfs_info->dev, "failed to init OPP table: %d\n", ret);
        goto err_put_node;
    }

    ret = dev_pm_opp_init_cpufreq_table(dvfs_info->dev, &dvfs_info->freq_table);
    if (ret) {
        dev_err(dvfs_info->dev,
            "failed to init cpufreq table: %d\n", ret);
        goto err_put_node;
    }

	proc_reg = regulator_get_optional(dvfs_info->dev, "vcpu");
	if (IS_ERR(proc_reg)) {
		dev_err(dvfs_info->dev,
			"%s: failed to get vcpu regulator for cpu\n", __func__);
		ret = -1;
		regulator_put(proc_reg);
	goto err_put_node;
	}
	dvfs_info->proc_reg = proc_reg;

	opp_srcu_head = dev_pm_opp_get_notifier(dvfs_info->dev);
	if (IS_ERR(opp_srcu_head)) {
		ret = PTR_ERR(opp_srcu_head);
		goto err_free_table;
	}
	dvfs_info->opp_nb.notifier_call = mtk_cpufreq_opp_notifier;
	ret = srcu_notifier_chain_register(opp_srcu_head, &dvfs_info->opp_nb);
	if (ret)
		goto err_free_table;

	list_add(&dvfs_info->list_head, &dvfs_info_list);
    ret = cpufreq_register_driver(&mt53xx_cpufreq_driver);
    if (ret) {
        dev_err(dvfs_info->dev,
            "%s: failed to register cpufreq driver\n", __func__);
        goto err_free_table;
    }

    return 0;

err_free_table:
    dev_pm_opp_free_cpufreq_table(dvfs_info->dev, &dvfs_info->freq_table);
err_put_node:
	return ret;
}

static struct platform_driver mt53xx_cpufreq_platdrv = {
    .driver = {
        .name   = "mt53xx-cpufreq",
        .owner  = THIS_MODULE,
        .of_match_table = mt53xx_cpufreq_match,
    },
    .probe = mt53xx_cpufreq_probe,
};
//module_platform_driver(mt53xx_cpufreq_platdrv); // use this function, cpufreq is loading earlier than clk driver
#ifdef CONFIG_PROC_FS

/***************************
 * show var
 ****************************/
static int mt_cpufreq_var_dump_proc_show(struct seq_file *m, void *v)
{
	u32 rdata = 0;

	rdata = mt53xx_get_hw_cpupll(); 

	if (rdata != 0)
		seq_printf(m, "CPU target freq is %d KHz\n", rdata);
	else
		seq_printf(m, "mt53xx_get_hw_cpupll fail\n");

	rdata = regulator_get_voltage(dvfs_info->proc_reg); /* unit uv: micro voltage */
	if (rdata != 0)
		seq_printf(m, "CPU target volt is %d uV\n", rdata);
	else
		seq_printf(m, "reg_vcpu read current voltage fail\n");

    return 0;
}

/***************************
 * show current voltage
 ****************************/
static int mt_cpufreq_cur_volt_proc_show(struct seq_file *m, void *v)
{
	u32 rdata = 0;

	rdata = regulator_get_voltage(dvfs_info->proc_reg); /* unit uv: micro voltage */

	if (rdata != 0)
		seq_printf(m, "%d\n", rdata);
	else
		seq_printf(m, "reg_vcpu read current voltage fail\n");

    return 0;
}

/***********************
 * set current voltage
 ************************/
static ssize_t mt_cpufreq_cur_volt_proc_write(struct file *file, const char __user *buffer,
                       size_t count, loff_t *data)
{
    char desc[32];
    int len = 0;

    int volt = 0;

    len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
    if (copy_from_user(desc, buffer, len))
        return 0;
    desc[len] = '\0';

    if (kstrtoint(desc, 0, &volt) == 0) {
        regulator_set_voltage(dvfs_info->proc_reg, volt, volt + VOLT_TOL);
    } else
        dev_warn(dvfs_info->dev,"bad argument_1!! argument should be \"0\"\n");

    return count;
}

#define PROC_FOPS_RW(name)                              \
    static int mt_ ## name ## _proc_open(struct inode *inode, struct file *file)    \
{                                           \
    return single_open(file, mt_ ## name ## _proc_show, PDE_DATA(inode));       \
}                                           \
static const struct file_operations mt_ ## name ## _proc_fops = {           \
    .owner        = THIS_MODULE,                        \
    .open          = mt_ ## name ## _proc_open,                 \
    .read          = seq_read,                          \
    .llseek      = seq_lseek,                           \
    .release        = single_release,                   \
    .write        = mt_ ## name ## _proc_write,                 \
}

#define PROC_FOPS_RO(name)                              \
    static int mt_ ## name ## _proc_open(struct inode *inode, struct file *file)    \
{                                           \
    return single_open(file, mt_ ## name ## _proc_show, PDE_DATA(inode));       \
}                                           \
static const struct file_operations mt_ ## name ## _proc_fops = {           \
    .owner        = THIS_MODULE,                        \
    .open          = mt_ ## name ## _proc_open,                 \
    .read          = seq_read,                          \
    .llseek      = seq_lseek,                           \
    .release        = single_release,                   \
}

#define PROC_ENTRY(name)    {__stringify(name), &mt_ ## name ## _proc_fops}

PROC_FOPS_RW(cpufreq_cur_volt);
PROC_FOPS_RO(cpufreq_var_dump);

static int mt_gpufreq_create_procfs(void)
{
    struct proc_dir_entry *dir = NULL;
    int i;

    struct pentry {
        const char *name;
        const struct file_operations *fops;
    };

    const struct pentry entries[] = {
        PROC_ENTRY(cpufreq_cur_volt),
        PROC_ENTRY(cpufreq_var_dump),
    };


    dir = proc_mkdir("cpufreq", NULL);

    if (!dir) {
        dev_err("fail to create /proc/cpufreq @ %s()\n", __func__);
        return -ENOMEM;
    }

    for (i = 0; i < ARRAY_SIZE(entries); i++) {
        if (!proc_create
            (entries[i].name, S_IRUGO | S_IWUSR | S_IWGRP, dir, entries[i].fops))
            dev_err("@%s: create /proc/cpufreq/%s failed\n", __func__,
                    entries[i].name);
    }

    return 0;
}
#endif
static int __init mt53xx_cpufreq_driver_init(void)
{
	int err;

    printk("mt53xx_cpufreq_driver_init\n");

    err = platform_driver_register(&mt53xx_cpufreq_platdrv);
	    if (err)
	        return err;	

#ifdef CONFIG_PROC_FS
	mt_gpufreq_create_procfs();
#endif
	return 0;
}

late_initcall(mt53xx_cpufreq_driver_init);


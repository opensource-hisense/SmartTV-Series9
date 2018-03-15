/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <linux/kthread.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/jiffies.h>
#include <linux/fs.h>
#include <linux/seq_file.h>
#include <linux/input.h>
#include <linux/sched.h>
#include <linux/sched/rt.h>
#include <linux/kthread.h>
#include <linux/delay.h>

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_address.h>
#endif

#include <linux/uaccess.h>
#include <linux/regulator/consumer.h>

#include "mtk_gpufreq.h"

/* #define BRING_UP */

#define DRIVER_NOT_READY    -1

/***************************
 * Define for random test
 ****************************/
/* #define MT_GPU_DVFS_RANDOM_TEST */

/***************************
 * Operate Point Definition
 ****************************/
#define GPUOP(khz, volt, idx)   \
{               \
    .gpufreq_khz = khz, \
    .gpufreq_volt = volt,   \
    .gpufreq_idx = idx, \
}

/**************************
 * GPU DVFS OPP table setting
 ***************************/
#define GPU_DVFS_FREQ0      (696000) /* KHz */
#define GPU_DVFS_FREQ1      (528000) /* KHz */

#define GPUFREQ_LAST_FREQ_LEVEL (GPU_DVFS_FREQ1)

#define GPU_DVFS_VOLT0  (112500)    /* mV x 100 */
#define GPU_DVFS_VOLT1  (100000)    /* mV x 100 */

#define GPU_DVFS_PTPOD_DISABLE_VOLT GPU_DVFS_VOLT1

/* register val -> mV */
#define GPU_VOLT_TO_MV(volt)            ((((volt)*625)/100+700)*100)

/*
 * LOG
 */
#if 1
#define TAG  "[Power/gpufreq] "

#define gpufreq_err(fmt, args...)   \
    pr_err(fmt, ##args)
#define gpufreq_warn(fmt, args...)  \
    pr_warn(fmt, ##args)
#define gpufreq_info(fmt, args...)  \
    pr_debug(fmt, ##args)
#define gpufreq_dbg(fmt, args...)   \
    pr_debug(fmt, ##args)
#define gpufreq_ver(fmt, args...)   \
    pr_debug(fmt, ##args)
#else
#define gpufreq_err(fmt, args...)
#define gpufreq_warn(fmt, args...)
#define gpufreq_info(fmt, args...)
#define gpufreq_dbg(fmt, args...)
#define gpufreq_ver(fmt, args...)
#endif

//#define SOFT_FREQ_HOPPING

static sampler_func g_pFreqSampler;
static sampler_func g_pVoltSampler;

static gpufreq_ptpod_update_notify g_pGpufreq_ptpod_update_notify;


/***************************
 * GPU DVFS OPP Table
 ****************************/
static struct mt_gpufreq_table_info mt_gpufreq_opp_tbl_e1_0[] = {
    GPUOP(GPU_DVFS_FREQ0, GPU_DVFS_VOLT0, 0),
    GPUOP(GPU_DVFS_FREQ1, GPU_DVFS_VOLT1, 1),
};

/**************************
 * enable GPU DVFS count
 ***************************/
static int g_gpufreq_dvfs_disable_count;

static unsigned int g_cur_gpu_freq = GPU_DVFS_FREQ1;    /* initial value*/
static unsigned int g_cur_gpu_volt = GPU_DVFS_VOLT1;    /* initial value*/
static unsigned int g_cur_gpu_idx = 0xFF;
static unsigned int g_cur_gpu_OPPidx = 0xFF;

static bool mt_gpufreq_ready;

/* In default settiing, freq_table[0] is max frequency, freq_table[num-1] is min frequency,*/
static unsigned int g_gpufreq_max_id;

static bool mt_gpufreq_dbg;
static bool mt_gpufreq_pause;
static bool mt_gpufreq_keep_max_frequency_state;
static bool mt_gpufreq_keep_opp_frequency_state;
#if 1
static unsigned int mt_gpufreq_keep_opp_frequency;
#endif
static unsigned int mt_gpufreq_keep_opp_index;
static bool mt_gpufreq_fixed_freq_volt_state;
static unsigned int mt_gpufreq_fixed_frequency;
static unsigned int mt_gpufreq_fixed_voltage;

#if 1
static unsigned int mt_gpufreq_volt_enable;
#endif
static unsigned int mt_gpufreq_volt_enable_state;

static bool mt_gpufreq_opp_max_frequency_state;
static unsigned int mt_gpufreq_opp_max_frequency;
static unsigned int mt_gpufreq_opp_max_index;

static unsigned int mt_gpufreq_dvfs_table_type;

#ifdef SOFT_FREQ_HOPPING
static struct task_struct *mfg_dfs_task;
static unsigned int g_target_gpu_freq = GPU_DVFS_FREQ1;
static int mfg_pll_freq_step = 6000 /* KHz */;
#endif
static int mfg_pll_freq_delay = 500;/*us*/


/* static DEFINE_SPINLOCK(mt_gpufreq_lock); */
static DEFINE_MUTEX(mt_gpufreq_lock);

static unsigned int mt_gpufreqs_num;
static struct mt_gpufreq_table_info *mt_gpufreqs;
static struct mt_gpufreq_table_info *mt_gpufreqs_default;

static struct mt_gpufreq_pmic_t *mt_gpufreq_pmic;

static bool mt_gpufreq_ptpod_disable;
static int mt_gpufreq_ptpod_disable_idx;

static void mt_gpufreq_clock_switch(unsigned int freq_new);
static void mt_gpufreq_volt_switch(unsigned int volt_old, unsigned int volt_new);
static void mt_gpufreq_set(unsigned int freq_old, unsigned int freq_new,
               unsigned int volt_old, unsigned int volt_new);
static unsigned int _mt_gpufreq_get_cur_volt(void);
static unsigned int _mt_gpufreq_get_cur_freq(void);

extern void __iomem *pCkgenPllGpIoMap;
#define CKGEN_REG_PLL_GROUP_CFG3 (0x0C)

#define GPU_WRITE32(offset, _val_)     (*((volatile unsigned int*)(pCkgenPllGpIoMap + offset)) = (_val_))
#define GPU_READ32(offset)             (*((volatile unsigned int*)(pCkgenPllGpIoMap + offset)))
static void mt53xx_hw_gpupll(unsigned int target_freq)
{
    unsigned int val, freq_factor;
    val = GPU_READ32(CKGEN_REG_PLL_GROUP_CFG3);
    freq_factor = target_freq / 1000 / 6;
    GPU_WRITE32(CKGEN_REG_PLL_GROUP_CFG3, (val & 0xFFF01FFF) | (freq_factor << 13));
    GPU_WRITE32(CKGEN_REG_PLL_GROUP_CFG3, GPU_READ32(CKGEN_REG_PLL_GROUP_CFG3) & ~(1U << 20));
    GPU_WRITE32(CKGEN_REG_PLL_GROUP_CFG3, GPU_READ32(CKGEN_REG_PLL_GROUP_CFG3) | (1U << 20));
    g_cur_gpu_freq = target_freq;
    gpufreq_dbg("set gpu freq %dMHz\n", target_freq/1000);
}

#ifdef SOFT_FREQ_HOPPING
static int mfg_freq_switch_func(void *unused)
{
    int freq_diff;
    while (!kthread_should_stop()) {
        set_current_state(TASK_INTERRUPTIBLE);
        gpufreq_dbg("mfg_freq_switch_func g_target_gpu_freq:%d\n",g_target_gpu_freq);
        mutex_lock(&mt_gpufreq_lock);
        freq_diff = g_target_gpu_freq - g_cur_gpu_freq;
        if (freq_diff) {
            if (abs(freq_diff) > mfg_pll_freq_step) {
                if (freq_diff > 0)
                    mt53xx_hw_gpupll(g_cur_gpu_freq + mfg_pll_freq_step);
                else
                    mt53xx_hw_gpupll(g_cur_gpu_freq - mfg_pll_freq_step);
            } else {
                mt53xx_hw_gpupll(g_target_gpu_freq);
            }
            mutex_unlock(&mt_gpufreq_lock);
            udelay(mfg_pll_freq_delay);
        } else {
            mutex_unlock(&mt_gpufreq_lock);
            schedule();
        }
    }
    return 0;
}
#endif


/*************************************************************************************
 * Check GPU DVFS Efuse
 **************************************************************************************/
static unsigned int mt_gpufreq_get_dvfs_table_type(void)
{
    return 0;
}

/**************************************
 * Random seed generated for test
 ***************************************/
#ifdef MT_GPU_DVFS_RANDOM_TEST
static int mt_gpufreq_idx_get(int num)
{
    int random = 0, mult = 0, idx;

    random = jiffies & 0xF;

    while (1) {
        if ((mult * num) >= random) {
            idx = (mult * num) - random;
            break;
        }
        mult++;
    }
    return idx;
}
#endif


/* Set frequency and voltage at driver probe function */
static void mt_gpufreq_set_initial(void)
{
    unsigned int cur_volt = 0, cur_freq = 0;
    int i = 0;

    mutex_lock(&mt_gpufreq_lock);

    cur_volt = _mt_gpufreq_get_cur_volt();
    cur_freq = _mt_gpufreq_get_cur_freq();

    for (i = 0; i < mt_gpufreqs_num; i++) {
        if (cur_volt >= mt_gpufreqs[i].gpufreq_volt) {
            mt_gpufreq_set(cur_freq, mt_gpufreqs[i].gpufreq_khz,
                       cur_volt, mt_gpufreqs[i].gpufreq_volt);
            g_cur_gpu_OPPidx = i;
            gpufreq_dbg("init_idx = %d\n", g_cur_gpu_OPPidx);
            break;
        }
    }

    /* Not found, set to LPM */
    if (i == mt_gpufreqs_num) {
        gpufreq_err
            ("Set to LPM since GPU idx not found according to current Vcore = %d mV\n",
             cur_volt / 100);
        g_cur_gpu_OPPidx = mt_gpufreqs_num - 1;
        gpufreq_err
            ("mt_gpufreq_set_initial freq index = %d mV\n", g_cur_gpu_OPPidx);
        mt_gpufreq_set(cur_freq, mt_gpufreqs[g_cur_gpu_OPPidx].gpufreq_khz,
                   cur_volt, mt_gpufreqs[g_cur_gpu_OPPidx].gpufreq_volt);
    }

    g_cur_gpu_freq = mt_gpufreqs[g_cur_gpu_OPPidx].gpufreq_khz;
    g_cur_gpu_volt = mt_gpufreqs[g_cur_gpu_OPPidx].gpufreq_volt;
    g_cur_gpu_idx = mt_gpufreqs[g_cur_gpu_OPPidx].gpufreq_idx;


    mutex_unlock(&mt_gpufreq_lock);
}

/* Set VGPU enable/disable when GPU clock be switched on/off */
unsigned int mt_gpufreq_voltage_enable_set(unsigned int enable)
{
    int ret = 0;
    //unsigned int delay = 0;

    mutex_lock(&mt_gpufreq_lock);

    if (mt_gpufreq_ready == false) {
        gpufreq_dbg("GPU DVFS not ready!\n");
        ret = -ENOSYS;  
        goto SET_EXIT;
    }
    
    if (mt_gpufreq_ptpod_disable == true) {
        if (enable == 0) {
            gpufreq_info("mt_gpufreq_ptpod_disable == true\n");
            ret = DRIVER_NOT_READY;
            goto SET_EXIT;
        }
    }
    
    if (enable == 1) {
        ret = regulator_enable(mt_gpufreq_pmic->reg_vgpu);
        if (ret != 0) {
            gpufreq_err("Failed to enable reg-vgpu: %d\n", ret);
            goto SET_EXIT;
        }
    } else {
        ret = regulator_disable(mt_gpufreq_pmic->reg_vgpu);
        if (ret != 0) {
            gpufreq_err("Failed to disable reg-vgpu: %d\n", ret);
            goto SET_EXIT;
        }
    }

    mt_gpufreq_volt_enable_state = enable;
    gpufreq_dbg("mt_gpufreq_voltage_enable_set, enable = %x\n", enable);
    //TODO:need this?
#if 0
    delay = GPU_DVFS_VOLT_SETTLE_TIME(0, g_cur_gpu_volt);
    udelay(delay);
#endif
SET_EXIT:
    mutex_unlock(&mt_gpufreq_lock);
    return ret;
}
EXPORT_SYMBOL(mt_gpufreq_voltage_enable_set);

/************************************************
 * DVFS enable API for PTPOD
 *************************************************/

void mt_gpufreq_enable_by_ptpod(void)
{
    if (mt_gpufreq_ready == false) {
        gpufreq_warn("@%s: GPU DVFS not ready!\n", __func__);
        return;
    }

    mt_gpufreq_voltage_enable_set(0);

    mt_gpufreq_ptpod_disable = false;
    gpufreq_info("mt_gpufreq enabled by ptpod\n");

    /* pmic auto mode: the variance of voltage is wide but saves more power. */
    regulator_set_mode(mt_gpufreq_pmic->reg_vgpu, REGULATOR_MODE_NORMAL);
    if (regulator_get_mode(mt_gpufreq_pmic->reg_vgpu) != REGULATOR_MODE_NORMAL)
        gpufreq_err("Vgpu should be REGULATOR_MODE_NORMAL(%d), but mode = %d\n",
            REGULATOR_MODE_NORMAL, regulator_get_mode(mt_gpufreq_pmic->reg_vgpu));
}
EXPORT_SYMBOL(mt_gpufreq_enable_by_ptpod);

/************************************************
 * DVFS disable API for PTPOD
 *************************************************/
void mt_gpufreq_disable_by_ptpod(void)
{
    int i = 0, target_idx = 0;

    if (mt_gpufreq_ready == false) {
        gpufreq_warn("@%s: GPU DVFS not ready!\n", __func__);
        return;
    }

    mt_gpufreq_ptpod_disable = true;
    gpufreq_info("mt_gpufreq disabled by ptpod\n");

    for (i = 0; i < mt_gpufreqs_num; i++) {
        /* VBoot = 0.85v for PTPOD */
        target_idx = i;
        if (mt_gpufreqs_default[i].gpufreq_volt <= GPU_DVFS_PTPOD_DISABLE_VOLT)
            break;
    }

    /* pmic PWM mode: the variance of voltage is narrow but consumes more power. */
    regulator_set_mode(mt_gpufreq_pmic->reg_vgpu, REGULATOR_MODE_FAST);

    if (regulator_get_mode(mt_gpufreq_pmic->reg_vgpu) != REGULATOR_MODE_FAST)
        gpufreq_err("Vgpu should be REGULATOR_MODE_FAST(%d), but mode = %d\n",
            REGULATOR_MODE_FAST, regulator_get_mode(mt_gpufreq_pmic->reg_vgpu));


    mt_gpufreq_ptpod_disable_idx = target_idx;

    mt_gpufreq_voltage_enable_set(1);
    mt_gpufreq_target(target_idx);
}
EXPORT_SYMBOL(mt_gpufreq_disable_by_ptpod);


bool mt_gpufreq_IsPowerOn(void)
{
    return (mt_gpufreq_volt_enable_state == 1);
}
EXPORT_SYMBOL(mt_gpufreq_IsPowerOn);


/************************************************
 * API to switch back default voltage setting for GPU PTPOD disabled
 *************************************************/
void mt_gpufreq_restore_default_volt(void)
{
    int i;

    if (mt_gpufreq_ready == false) {
        gpufreq_warn("@%s: GPU DVFS not ready!\n", __func__);
        return;
    }

    mutex_lock(&mt_gpufreq_lock);

    for (i = 0; i < mt_gpufreqs_num; i++) {
        mt_gpufreqs[i].gpufreq_volt = mt_gpufreqs_default[i].gpufreq_volt;
        gpufreq_dbg("@%s: mt_gpufreqs[%d].gpufreq_volt = %d\n", __func__, i,
                mt_gpufreqs[i].gpufreq_volt);
    }

    mt_gpufreq_volt_switch(g_cur_gpu_volt, mt_gpufreqs[g_cur_gpu_OPPidx].gpufreq_volt);

    g_cur_gpu_volt = mt_gpufreqs[g_cur_gpu_OPPidx].gpufreq_volt;

    mutex_unlock(&mt_gpufreq_lock);
}
EXPORT_SYMBOL(mt_gpufreq_restore_default_volt);

/* Set voltage because PTP-OD modified voltage table by PMIC wrapper */
unsigned int mt_gpufreq_update_volt(unsigned int pmic_volt[], unsigned int array_size)
{
    int i;          /* , idx; */
    /* unsigned long flags; */
    unsigned volt = 0;
    unsigned int volt_old = 0;

    if (mt_gpufreq_ready == false) {
        gpufreq_warn("@%s: GPU DVFS not ready!\n", __func__);
        return DRIVER_NOT_READY;
    }

    if (array_size > mt_gpufreqs_num) {
        gpufreq_err("mt_gpufreq_update_volt: array_size = %d, Over-Boundary!\n", array_size);
        return 0;   /*-ENOSYS;*/
    }

    volt_old = mt_gpufreqs[g_cur_gpu_OPPidx].gpufreq_volt;
    mutex_lock(&mt_gpufreq_lock);
    for (i = 0; i < array_size; i++) {
        volt = GPU_VOLT_TO_MV(pmic_volt[i]);
        if ((volt > 70000) && (volt < 135000)) {    /* between 950mv~1300mv */
            mt_gpufreqs[i].gpufreq_volt = volt;
            gpufreq_dbg("@%s: mt_gpufreqs[%d].gpufreq_volt = %d\n", __func__, i,
                    mt_gpufreqs[i].gpufreq_volt);
        } else {
            gpufreq_err("@%s: index[%d]._volt = %d Over-Boundary\n", __func__, i, volt);
        }
    }

//    g_cur_gpu_volt = mt_gpufreqs[g_cur_gpu_OPPidx].gpufreq_volt;
#ifndef MTK_GPU_SPM
    gpufreq_err("@%s: index[%d] old volt = %d, new volt = %d\n", __func__, g_cur_gpu_OPPidx, volt_old,mt_gpufreqs[g_cur_gpu_OPPidx].gpufreq_volt);
    mt_gpufreq_volt_switch(volt_old, mt_gpufreqs[g_cur_gpu_OPPidx].gpufreq_volt );
#endif
    if (g_pGpufreq_ptpod_update_notify != NULL)
        g_pGpufreq_ptpod_update_notify();
    mutex_unlock(&mt_gpufreq_lock);

    return 0;
}
EXPORT_SYMBOL(mt_gpufreq_update_volt);

unsigned int mt_gpufreq_get_dvfs_table_num(void)
{
    return mt_gpufreqs_num;
}
EXPORT_SYMBOL(mt_gpufreq_get_dvfs_table_num);

unsigned int mt_gpufreq_get_freq_by_idx(unsigned int idx)
{
    if (mt_gpufreq_ready == false) {
        gpufreq_warn("@%s: GPU DVFS not ready!\n", __func__);
        return DRIVER_NOT_READY;
    }

    if (idx < mt_gpufreqs_num) {
        gpufreq_dbg("@%s: idx = %d, frequency= %d\n", __func__, idx,
                mt_gpufreqs[idx].gpufreq_khz);
        return mt_gpufreqs[idx].gpufreq_khz;
    }

    gpufreq_dbg("@%s: idx = %d, NOT found! return 0!\n", __func__, idx);
    return 0;
}
EXPORT_SYMBOL(mt_gpufreq_get_freq_by_idx);

unsigned int mt_gpufreq_get_volt_by_idx(unsigned int idx)
{
    if (mt_gpufreq_ready == false) {
        gpufreq_warn("@%s: GPU DVFS not ready!\n", __func__);
        return DRIVER_NOT_READY;
    }

    if (idx < mt_gpufreqs_num) {
        gpufreq_dbg("@%s: idx = %d, voltage= %d\n", __func__, idx,
                mt_gpufreqs[idx].gpufreq_volt);
        return mt_gpufreqs[idx].gpufreq_volt;
    }

    gpufreq_dbg("@%s: idx = %d, NOT found! return 0!\n", __func__, idx);
    return 0;
}
EXPORT_SYMBOL(mt_gpufreq_get_volt_by_idx);

unsigned int mt_gpufreq_set_max_freq(unsigned int max_freq)
{
    int i;
    int found = 0;

    if (max_freq == 0) {
        mt_gpufreq_opp_max_frequency_state = false;
    } else {
        for (i = 0; i < mt_gpufreqs_num; i++) {
            if (mt_gpufreqs[i].gpufreq_khz <= max_freq) {
                mt_gpufreq_opp_max_index = i;
                found = 1;
                break;
            }
        }

        if (found == 1) {
            mt_gpufreq_opp_max_frequency_state = true;
            mt_gpufreq_opp_max_frequency = mt_gpufreqs[mt_gpufreq_opp_max_index].gpufreq_khz;
            mt_gpufreq_voltage_enable_set(1);
            mt_gpufreq_target(mt_gpufreq_opp_max_index);
        }else {
            gpufreq_warn("@%s: the args max freq:%d is less than the lowest freq allowed!\n", __func__,max_freq);
            return 0;
        }
    }
    return 1;
}
EXPORT_SYMBOL(mt_gpufreq_set_max_freq);

/***********************************************
 * register frequency table to gpufreq subsystem
 ************************************************/
static int mt_setup_gpufreqs_table(struct mt_gpufreq_table_info *freqs, int num)
{
    int i = 0;

    mt_gpufreqs = kzalloc((num) * sizeof(*freqs), GFP_KERNEL);
    mt_gpufreqs_default = kzalloc((num) * sizeof(*freqs), GFP_KERNEL);
    if ((mt_gpufreqs == NULL) || (mt_gpufreqs_default == NULL))
        return -ENOMEM;

    for (i = 0; i < num; i++) {
        mt_gpufreqs[i].gpufreq_khz = freqs[i].gpufreq_khz;
        mt_gpufreqs[i].gpufreq_volt = freqs[i].gpufreq_volt;
        mt_gpufreqs[i].gpufreq_idx = freqs[i].gpufreq_idx;

        mt_gpufreqs_default[i].gpufreq_khz = freqs[i].gpufreq_khz;
        mt_gpufreqs_default[i].gpufreq_volt = freqs[i].gpufreq_volt;
        mt_gpufreqs_default[i].gpufreq_idx = freqs[i].gpufreq_idx;

        gpufreq_dbg("freqs[%d].gpufreq_khz = %u\n", i, freqs[i].gpufreq_khz);
        gpufreq_dbg("freqs[%d].gpufreq_volt = %u\n", i, freqs[i].gpufreq_volt);
        gpufreq_dbg("freqs[%d].gpufreq_idx = %u\n", i, freqs[i].gpufreq_idx);
    }

    mt_gpufreqs_num = num;

    gpufreq_info("@%s: g_cur_gpu_freq = %d, g_cur_gpu_volt = %d\n", __func__, g_cur_gpu_freq,
            g_cur_gpu_volt);

    return 0;
}

/**************************************
 * check if maximum frequency is needed
 ***************************************/
static int mt_gpufreq_keep_max_freq(unsigned int freq_old, unsigned int freq_new)
{
    if (mt_gpufreq_keep_max_frequency_state == true)
        return 1;

    return 0;
}

/*****************************
 * set GPU DVFS status
 ******************************/
int mt_gpufreq_state_set(int enabled)
{
    if (enabled) {
        if (!mt_gpufreq_pause) {
            gpufreq_dbg("gpufreq already enabled\n");
            return 0;
        }

        /*****************
         * enable GPU DVFS
         ******************/
        g_gpufreq_dvfs_disable_count--;
        gpufreq_dbg("enable GPU DVFS: g_gpufreq_dvfs_disable_count = %d\n",
                g_gpufreq_dvfs_disable_count);

        /***********************************************
         * enable DVFS if no any module still disable it
         ************************************************/
        if (g_gpufreq_dvfs_disable_count <= 0)
            mt_gpufreq_pause = false;
        else
            gpufreq_warn("someone still disable gpufreq, cannot enable it\n");
    } else {
        /******************
         * disable GPU DVFS
         *******************/
        g_gpufreq_dvfs_disable_count++;
        gpufreq_dbg("disable GPU DVFS: g_gpufreq_dvfs_disable_count = %d\n",
                g_gpufreq_dvfs_disable_count);

        if (mt_gpufreq_pause) {
            gpufreq_dbg("gpufreq already disabled\n");
            return 0;
        }

        mt_gpufreq_pause = true;
    }

    return 0;
}
EXPORT_SYMBOL(mt_gpufreq_state_set);
#if 0
static unsigned int mt_gpufreq_dds_calc(unsigned int freq_khz, enum post_div_order_enum post_div_order)
{
    unsigned int dds = 0;

    dds = (((freq_khz * 4) / 1000) * 0x4000) / 26;

    gpufreq_dbg("@%s: request freq = %d, div_order = %d, dds = %x\n",
            __func__, freq_khz, post_div_order, dds);

    return dds;
}
#endif

static void mt_gpufreq_clock_switch(unsigned int freq_new)
{
    if (freq_new == g_cur_gpu_freq)
        return;

#ifdef SOFT_FREQ_HOPPING
    g_target_gpu_freq = freq_new;
    wake_up_process(mfg_dfs_task);
#else
    mt53xx_hw_gpupll(freq_new);
    udelay(mfg_pll_freq_delay);
#endif

    if (g_pFreqSampler != NULL)
        g_pFreqSampler(freq_new);

    gpufreq_dbg("mt_gpu_clock_switch, freq_new = %d (KHz)\n", freq_new);
}

static void mt_gpufreq_volt_switch(unsigned int volt_old, unsigned int volt_new)
{
    /*unsigned int delay_unit_us = 100; */

    gpufreq_dbg("@%s: volt_new = %d\n", __func__, volt_new);

    if (volt_new == g_cur_gpu_volt)
        return;

    //TODO: call regulator here 
//#if 1
//    if (volt_new > volt_old)
        regulator_set_voltage(mt_gpufreq_pmic->reg_vgpu,
                    volt_new*10, (volt_new*10) + 10000);
//    else
//        regulator_set_voltage(mt_gpufreq_pmic->reg_vgpu,
//                    volt_new*10, volt_old*10);
//#endif

    g_cur_gpu_volt = volt_new;

    if (g_pVoltSampler != NULL)
        g_pVoltSampler(volt_new);
}

static unsigned int _mt_gpufreq_get_cur_freq(void)
{
    return g_cur_gpu_freq;
}

static unsigned int _mt_gpufreq_get_cur_volt(void)
{
    unsigned int gpu_volt = 0;

    /* WARRNING: regulator_get_voltage prints uV */
    gpu_volt = regulator_get_voltage(mt_gpufreq_pmic->reg_vgpu) / 10;
    gpufreq_dbg("gpu_dvfs_get_cur_volt:[PMIC] volt = %d\n", gpu_volt);

    return gpu_volt;
}

/*****************************************
 * frequency ramp up and ramp down handler
 ******************************************/
/***********************************************************
 * [note]
 * 1. frequency ramp up need to wait voltage settle
 * 2. frequency ramp down do not need to wait voltage settle
 ************************************************************/
static void mt_gpufreq_set(unsigned int freq_old, unsigned int freq_new,
               unsigned int volt_old, unsigned int volt_new)
{
    gpufreq_dbg("mt_gpufreq_set set to old volt=%d, new volt=%d\n", volt_old, volt_new);
    gpufreq_dbg("mt_gpufreq_set set to old freq=%d, new freq=%d\n", freq_old, freq_new);

    if (freq_new > freq_old) {
        mt_gpufreq_volt_switch(volt_old, volt_new);
        mdelay(10);
        mt_gpufreq_clock_switch(freq_new);
    } else {
        mt_gpufreq_clock_switch(freq_new);
        mdelay(10);
        mt_gpufreq_volt_switch(volt_old, volt_new);
    }
}

/**********************************
 * gpufreq target callback function
 ***********************************/
/*************************************************
 * [note]
 * 1. handle frequency change request
 * 2. call mt_gpufreq_set to set target frequency
 **************************************************/
unsigned int mt_gpufreq_target(unsigned int idx)
{
    /* unsigned long flags; */
    unsigned int target_freq, target_volt, target_idx, target_OPPidx;

    mutex_lock(&mt_gpufreq_lock);

    if (mt_gpufreq_pause == true) {
        gpufreq_warn("GPU DVFS pause!\n");
        mutex_unlock(&mt_gpufreq_lock);
        return DRIVER_NOT_READY;
    }

    if (mt_gpufreq_ready == false) {
        gpufreq_warn("GPU DVFS not ready!\n");
        mutex_unlock(&mt_gpufreq_lock);
        return DRIVER_NOT_READY;
    }

    if (mt_gpufreq_volt_enable_state == 0) {
        gpufreq_dbg("mt_gpufreq_volt_enable_state == 0! return\n");
        mutex_unlock(&mt_gpufreq_lock);
        return DRIVER_NOT_READY;
    }
#ifdef MT_GPU_DVFS_RANDOM_TEST
    idx = mt_gpufreq_idx_get(5);
    gpufreq_dbg("@%s: random test index is %d !\n", __func__, idx);
#endif

    if (idx > (mt_gpufreqs_num - 1)) {
        mutex_unlock(&mt_gpufreq_lock);
        gpufreq_err("@%s: idx out of range! idx = %d\n", __func__, idx);
        return -1;
    }

    /**********************************
     * look up for the target GPU OPP
     ***********************************/
    target_freq = mt_gpufreqs[idx].gpufreq_khz;
    target_volt = mt_gpufreqs[idx].gpufreq_volt;
    target_idx = mt_gpufreqs[idx].gpufreq_idx;
    target_OPPidx = idx;

    gpufreq_dbg("@%s: begin, receive freq: %d, OPPidx: %d\n", __func__, target_freq,
            target_OPPidx);

    /**********************************
     * Check if need to keep max frequency
     ***********************************/
    if (mt_gpufreq_keep_max_freq(g_cur_gpu_freq, target_freq)) {
        target_freq = mt_gpufreqs[g_gpufreq_max_id].gpufreq_khz;
        target_volt = mt_gpufreqs[g_gpufreq_max_id].gpufreq_volt;
        target_idx = mt_gpufreqs[g_gpufreq_max_id].gpufreq_idx;
        target_OPPidx = g_gpufreq_max_id;
        gpufreq_dbg("Keep MAX frequency %d !\n", target_freq);
    }

    /************************************************
     * If /proc command keep opp frequency.
     *************************************************/
    if (mt_gpufreq_keep_opp_frequency_state == true) {
        target_freq = mt_gpufreqs[mt_gpufreq_keep_opp_index].gpufreq_khz;
        target_volt = mt_gpufreqs[mt_gpufreq_keep_opp_index].gpufreq_volt;
        target_idx = mt_gpufreqs[mt_gpufreq_keep_opp_index].gpufreq_idx;
        target_OPPidx = mt_gpufreq_keep_opp_index;
        gpufreq_dbg("Keep opp! opp frequency %d, opp voltage %d, opp idx %d\n", target_freq,
                target_volt, target_OPPidx);
    }

    /************************************************
     * If /proc command fix the frequency.
     *************************************************/
    if (mt_gpufreq_fixed_freq_volt_state == true) {
        target_freq = mt_gpufreq_fixed_frequency;
        target_volt = mt_gpufreq_fixed_voltage;
        target_idx = 0;
        target_OPPidx = 0;
        gpufreq_dbg("Fixed! fixed frequency %d, fixed voltage %d\n", target_freq,
                target_volt);
    }

    /************************************************
     * If /proc command keep opp max frequency.
     *************************************************/
    if (mt_gpufreq_opp_max_frequency_state == true) {
        if (target_freq > mt_gpufreq_opp_max_frequency) {
            target_freq = mt_gpufreqs[mt_gpufreq_opp_max_index].gpufreq_khz;
            target_volt = mt_gpufreqs[mt_gpufreq_opp_max_index].gpufreq_volt;
            target_idx = mt_gpufreqs[mt_gpufreq_opp_max_index].gpufreq_idx;
            target_OPPidx = mt_gpufreq_opp_max_index;

            gpufreq_dbg
                ("opp max freq! opp max frequency %d, opp max voltage %d, opp max idx %d\n",
                 target_freq, target_volt, target_OPPidx);
        }
    }

    /************************************************
     * DVFS keep at max freq when PTPOD initial
     *************************************************/
    if (mt_gpufreq_ptpod_disable == true) {
#if 1
        target_freq = mt_gpufreqs[mt_gpufreq_ptpod_disable_idx].gpufreq_khz;
        target_volt = GPU_DVFS_PTPOD_DISABLE_VOLT;
        target_idx = mt_gpufreqs[mt_gpufreq_ptpod_disable_idx].gpufreq_idx;
        target_OPPidx = mt_gpufreq_ptpod_disable_idx;
        gpufreq_dbg("PTPOD disable dvfs, mt_gpufreq_ptpod_disable_idx = %d\n",
                mt_gpufreq_ptpod_disable_idx);
#else
        mutex_unlock(&mt_gpufreq_lock);
        gpufreq_dbg("PTPOD disable dvfs, return\n");
        return 0;
#endif
    }

    /************************************************
     * target frequency == current frequency, skip it
     *************************************************/
    if (g_cur_gpu_freq == target_freq && g_cur_gpu_volt == target_volt) {
        mutex_unlock(&mt_gpufreq_lock);
        gpufreq_dbg("GPU frequency from %d KHz to %d KHz (skipped) due to same frequency\n",
                g_cur_gpu_freq, target_freq);
        return 0;
    }

    gpufreq_dbg("GPU current frequency %d KHz, target frequency %d KHz\n", g_cur_gpu_freq,
            target_freq);

    /******************************
     * set to the target frequency
     *******************************/
    mt_gpufreq_set(g_cur_gpu_freq, target_freq, g_cur_gpu_volt, target_volt);

    g_cur_gpu_idx = target_idx;
    g_cur_gpu_OPPidx = target_OPPidx;

    mutex_unlock(&mt_gpufreq_lock);

    return 0;
}
EXPORT_SYMBOL(mt_gpufreq_target);

/************************************************
 * return current GPU frequency index
 *************************************************/
unsigned int mt_gpufreq_get_cur_freq_index(void)
{
    gpufreq_dbg("current GPU frequency OPP index is %d\n", g_cur_gpu_OPPidx);
    return g_cur_gpu_OPPidx;
}
EXPORT_SYMBOL(mt_gpufreq_get_cur_freq_index);

/************************************************
 * return current GPU frequency
 *************************************************/
unsigned int mt_gpufreq_get_cur_freq(void)
{
    gpufreq_dbg("current GPU frequency is %d MHz\n", g_cur_gpu_freq / 1000);
    return g_cur_gpu_freq;
}
EXPORT_SYMBOL(mt_gpufreq_get_cur_freq);

/************************************************
 * return current GPU voltage
 *************************************************/
unsigned int mt_gpufreq_get_cur_volt(void)
{
#if 0
    return g_cur_gpu_volt;
#else
    return _mt_gpufreq_get_cur_volt();
#endif
}
EXPORT_SYMBOL(mt_gpufreq_get_cur_volt);

/************************************************
 * register / unregister ptpod update GPU volt CB
 *************************************************/
void mt_gpufreq_update_volt_registerCB(gpufreq_ptpod_update_notify pCB)
{
    g_pGpufreq_ptpod_update_notify = pCB;
}
EXPORT_SYMBOL(mt_gpufreq_update_volt_registerCB);

/************************************************
 * register / unregister set GPU freq CB
 *************************************************/
void mt_gpufreq_setfreq_registerCB(sampler_func pCB)
{
    g_pFreqSampler = pCB;
}
EXPORT_SYMBOL(mt_gpufreq_setfreq_registerCB);

/************************************************
 * register / unregister set GPU volt CB
 *************************************************/
void mt_gpufreq_setvolt_registerCB(sampler_func pCB)
{
    g_pVoltSampler = pCB;
}
EXPORT_SYMBOL(mt_gpufreq_setvolt_registerCB);

static int mt_gpufreq_pm_restore_early(struct device *dev)
{
    int i = 0;
    int found = 0;

    g_cur_gpu_freq = _mt_gpufreq_get_cur_freq();

    for (i = 0; i < mt_gpufreqs_num; i++) {
        if (g_cur_gpu_freq == mt_gpufreqs[i].gpufreq_khz) {
            g_cur_gpu_idx = mt_gpufreqs[i].gpufreq_idx;
            g_cur_gpu_volt = mt_gpufreqs[i].gpufreq_volt;
            g_cur_gpu_OPPidx = i;
            found = 1;
            gpufreq_dbg("match g_cur_gpu_OPPidx: %d\n", g_cur_gpu_OPPidx);
            break;
        }
    }

    if (found == 0) {
        g_cur_gpu_idx = mt_gpufreqs[0].gpufreq_idx;
        g_cur_gpu_volt = mt_gpufreqs[0].gpufreq_volt;
        g_cur_gpu_OPPidx = 0;
        gpufreq_err("gpu freq not found, set parameter to max freq\n");
    }

    gpufreq_dbg("GPU freq SW/HW: %d/%d\n", g_cur_gpu_freq, _mt_gpufreq_get_cur_freq());
    gpufreq_dbg("g_cur_gpu_OPPidx: %d\n", g_cur_gpu_OPPidx);

    return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id mt_gpufreq_of_match[] = {
    {.compatible = "mediatek,mt5893-gpufreq",},
    { /* sentinel */ },
};
#endif
MODULE_DEVICE_TABLE(of, mt_gpufreq_of_match);
static int mt_gpufreq_pdrv_probe(struct platform_device *pdev)
{
#ifdef CONFIG_OF
    struct device_node *node;

    node = of_find_matching_node(NULL, mt_gpufreq_of_match);
    if (!node)
        gpufreq_err("@%s: find GPU node failed\n", __func__);

    /* alloc PMIC regulator */
    mt_gpufreq_pmic = kzalloc(sizeof(struct mt_gpufreq_pmic_t), GFP_KERNEL);
    if (mt_gpufreq_pmic == NULL)
        return -ENOMEM;


    mt_gpufreq_pmic->reg_vgpu = regulator_get(&pdev->dev, "vgpu");
    if (IS_ERR(mt_gpufreq_pmic->reg_vgpu)) {
        dev_err(&pdev->dev, "cannot get reg-vgpu\n");
        return PTR_ERR(mt_gpufreq_pmic->reg_vgpu);
    }
#endif

    mt_gpufreq_dvfs_table_type = mt_gpufreq_get_dvfs_table_type();

    /**********************
     * setup gpufreq table
     ***********************/
    gpufreq_info("setup gpufreqs table\n");

    switch (mt_gpufreq_dvfs_table_type) {
    case 0:
        mt_setup_gpufreqs_table(mt_gpufreq_opp_tbl_e1_0,
                    ARRAY_SIZE(mt_gpufreq_opp_tbl_e1_0));
        break;
    default:
        mt_setup_gpufreqs_table(mt_gpufreq_opp_tbl_e1_0,
                    ARRAY_SIZE(mt_gpufreq_opp_tbl_e1_0));
        break;
    }

    /**********************
     * setup initial frequency
     ***********************/
    mt_gpufreq_set_initial();

#ifdef SOFT_FREQ_HOPPING
    mfg_dfs_task = kthread_create(mfg_freq_switch_func, NULL, "mfg-dfs");
    if (IS_ERR(mfg_dfs_task)) {
        mfg_dfs_task = NULL;
        gpufreq_err("@%s: kthread_create mfg_freq_switch_func fail\n", __func__);
        return -1;
    }
#endif

    gpufreq_info("GPU current frequency = %dKHz\n", _mt_gpufreq_get_cur_freq());
    gpufreq_info("Current Vcore = %dmV\n", _mt_gpufreq_get_cur_volt() / 100);
    gpufreq_info("g_cur_gpu_freq = %d, g_cur_gpu_volt = %d\n", g_cur_gpu_freq, g_cur_gpu_volt);
    gpufreq_info("g_cur_gpu_idx = %d, g_cur_gpu_OPPidx = %d\n", g_cur_gpu_idx,
             g_cur_gpu_OPPidx);

    mt_gpufreq_ready = true;

    return 0;
}

/***************************************
 * this function should never be called
 ****************************************/
static int mt_gpufreq_pdrv_remove(struct platform_device *pdev)
{
#ifdef SOFT_FREQ_HOPPING
    if (mfg_dfs_task)
    {
        kthread_stop(mfg_dfs_task);
    }
#endif
    return 0;
}

static const struct dev_pm_ops mt_gpufreq_pm_ops = {
    .suspend = NULL,
    .resume = NULL,
    .restore_early = mt_gpufreq_pm_restore_early,
};
static struct platform_driver mt_gpufreq_pdrv = {
    .probe = mt_gpufreq_pdrv_probe,
    .remove = mt_gpufreq_pdrv_remove,
    .driver = {
           .name = "gpufreq",
           .pm = &mt_gpufreq_pm_ops,
           .owner = THIS_MODULE,
#ifdef CONFIG_OF
           .of_match_table = mt_gpufreq_of_match,
#endif
           },
};


#ifdef CONFIG_PROC_FS
/* #if 0 */
/*
 * PROC
 */

/***************************
 * show current debug status
 ****************************/
static int mt_gpufreq_dbg_proc_show(struct seq_file *m, void *v)
{
    if (mt_gpufreq_dbg)
        seq_puts(m, "gpufreq debug enabled\n");
    else
        seq_puts(m, "gpufreq debug disabled\n");

    return 0;
}

/***********************
 * enable debug message
 ************************/
static ssize_t mt_gpufreq_dbg_proc_write(struct file *file, const char __user *buffer,
                       size_t count, loff_t *data)
{
    char desc[32];
    int len = 0;

    int debug = 0;

    len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
    if (copy_from_user(desc, buffer, len))
        return 0;
    desc[len] = '\0';

    if (kstrtoint(desc, 0, &debug) == 0) {
        if (debug == 0)
            mt_gpufreq_dbg = 0;
        else if (debug == 1)
            mt_gpufreq_dbg = 1;
        else
            gpufreq_warn("bad argument!! should be 0 or 1 [0: disable, 1: enable]\n");
    } else
        gpufreq_warn("bad argument!! should be 0 or 1 [0: disable, 1: enable]\n");

    return count;
}

/******************************
 * show current GPU DVFS stauts
 *******************************/
static int mt_gpufreq_state_proc_show(struct seq_file *m, void *v)
{
    if (!mt_gpufreq_pause)
        seq_puts(m, "GPU DVFS enabled\n");
    else
        seq_puts(m, "GPU DVFS disabled\n");

    return 0;
}

/****************************************
 * set GPU DVFS stauts by sysfs interface
 *****************************************/
static ssize_t mt_gpufreq_state_proc_write(struct file *file,
                       const char __user *buffer, size_t count, loff_t *data)
{
    char desc[32];
    int len = 0;

    int enabled = 0;

    len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
    if (copy_from_user(desc, buffer, len))
        return 0;
    desc[len] = '\0';

    if (kstrtoint(desc, 0, &enabled) == 0) {
        if (enabled == 1) {
            mt_gpufreq_keep_max_frequency_state = false;
            mt_gpufreq_state_set(1);
        } else if (enabled == 0) {
            /* Keep MAX frequency when GPU DVFS disabled. */
            mt_gpufreq_keep_max_frequency_state = true;
            mt_gpufreq_voltage_enable_set(1);
            mt_gpufreq_target(g_gpufreq_max_id);
            mt_gpufreq_state_set(0);
        } else
            gpufreq_warn("bad argument!! argument should be \"1\" or \"0\"\n");
    } else
        gpufreq_warn("bad argument!! argument should be \"1\" or \"0\"\n");

    return count;
}

/********************
 * show GPU OPP table
 *********************/
static int mt_gpufreq_opp_dump_proc_show(struct seq_file *m, void *v)
{
    int i = 0;

    for (i = 0; i < mt_gpufreqs_num; i++) {
        seq_printf(m, "[%d] ", i);
        seq_printf(m, "freq = %d, ", mt_gpufreqs[i].gpufreq_khz);
        seq_printf(m, "volt = %d, ", mt_gpufreqs[i].gpufreq_volt);
        seq_printf(m, "idx = %d\n", mt_gpufreqs[i].gpufreq_idx);
    }

    return 0;
}

/***************************
 * show current specific frequency status
 ****************************/
static int mt_gpufreq_opp_freq_proc_show(struct seq_file *m, void *v)
{
    if (mt_gpufreq_keep_opp_frequency_state) {
        seq_puts(m, "gpufreq keep opp frequency enabled\n");
        seq_printf(m, "freq = %d\n", mt_gpufreqs[mt_gpufreq_keep_opp_index].gpufreq_khz);
        seq_printf(m, "volt = %d\n", mt_gpufreqs[mt_gpufreq_keep_opp_index].gpufreq_volt);
    } else
        seq_puts(m, "gpufreq keep opp frequency disabled\n");
    return 0;
}

/***********************
 * enable specific frequency
 ************************/
static ssize_t mt_gpufreq_opp_freq_proc_write(struct file *file, const char __user *buffer,
                          size_t count, loff_t *data)
{
    char desc[32];
    int len = 0;

    int i = 0;
    int fixed_freq = 0;
    int found = 0;

    len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
    if (copy_from_user(desc, buffer, len))
        return 0;
    desc[len] = '\0';

    if (kstrtoint(desc, 0, &fixed_freq) == 0) {
        if (fixed_freq == 0) {
            mt_gpufreq_keep_opp_frequency_state = false;
        } else {
            for (i = 0; i < mt_gpufreqs_num; i++) {
                if (fixed_freq == mt_gpufreqs[i].gpufreq_khz) {
                    mt_gpufreq_keep_opp_index = i;
                    found = 1;
                    break;
                }
            }

            if (found == 1) {
                mt_gpufreq_keep_opp_frequency_state = true;
                mt_gpufreq_keep_opp_frequency = fixed_freq;

                mt_gpufreq_voltage_enable_set(1);
                mt_gpufreq_target(mt_gpufreq_keep_opp_index);
            }

        }
    } else
        gpufreq_warn("bad argument!! please provide the fixed frequency\n");

    return count;
}

/***************************
 * show current specific frequency status
 ****************************/
static int mt_gpufreq_opp_max_freq_proc_show(struct seq_file *m, void *v)
{
    if (mt_gpufreq_opp_max_frequency_state) {
        seq_puts(m, "gpufreq opp max frequency enabled\n");
        seq_printf(m, "freq = %d\n", mt_gpufreqs[mt_gpufreq_opp_max_index].gpufreq_khz);
        seq_printf(m, "volt = %d\n", mt_gpufreqs[mt_gpufreq_opp_max_index].gpufreq_volt);
    } else
        seq_puts(m, "gpufreq opp max frequency disabled\n");

    return 0;
}

/***********************
 * enable specific frequency
 ************************/
static ssize_t mt_gpufreq_opp_max_freq_proc_write(struct file *file, const char __user *buffer,
                          size_t count, loff_t *data)
{
    char desc[32];
    int len = 0;

    int i = 0;
    int max_freq = 0;
    int found = 0;

    len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
    if (copy_from_user(desc, buffer, len))
        return 0;
    desc[len] = '\0';

    if (kstrtoint(desc, 0, &max_freq) == 0) {
        if (max_freq == 0) {
            mt_gpufreq_opp_max_frequency_state = false;
        } else {
            for (i = 0; i < mt_gpufreqs_num; i++) {
                if (mt_gpufreqs[i].gpufreq_khz <= max_freq) {
                    mt_gpufreq_opp_max_index = i;
                    found = 1;
                    break;
                }
            }

            if (found == 1) {
                mt_gpufreq_opp_max_frequency_state = true;
                mt_gpufreq_opp_max_frequency =
                    mt_gpufreqs[mt_gpufreq_opp_max_index].gpufreq_khz;

                mt_gpufreq_voltage_enable_set(1);
                mt_gpufreq_target(mt_gpufreq_opp_max_index);
            }
        }
    } else
        gpufreq_warn("bad argument!! please provide the maximum limited frequency\n");

    return count;
}

/********************
 * show variable dump
 *********************/
static int mt_gpufreq_var_dump_proc_show(struct seq_file *m, void *v)
{
    seq_printf(m, "g_cur_gpu_freq = %d, g_cur_gpu_volt = %d\n", mt_gpufreq_get_cur_freq(),
           mt_gpufreq_get_cur_volt());
    seq_printf(m, "g_cur_gpu_idx = %d, g_cur_gpu_OPPidx = %d\n", g_cur_gpu_idx,
           g_cur_gpu_OPPidx);

    seq_printf(m, "_mt_gpufreq_get_cur_freq = %d\n", _mt_gpufreq_get_cur_freq());
    seq_printf(m, "mt_gpufreq_volt_enable_state = %d\n", mt_gpufreq_volt_enable_state);
    seq_printf(m, "mt_gpufreq_dvfs_table_type = %d\n", mt_gpufreq_dvfs_table_type);
    seq_printf(m, "mt_gpufreq_ptpod_disable_idx = %d\n", mt_gpufreq_ptpod_disable_idx);

    return 0;
}

/***************************
 * show current voltage enable status
 ****************************/
static int mt_gpufreq_volt_enable_proc_show(struct seq_file *m, void *v)
{
    if (mt_gpufreq_volt_enable)
        seq_puts(m, "gpufreq voltage enabled\n");
    else
        seq_puts(m, "gpufreq voltage disabled\n");

    return 0;
}

/***********************
 * enable specific frequency
 ************************/
static ssize_t mt_gpufreq_volt_enable_proc_write(struct file *file, const char __user *buffer,
                         size_t count, loff_t *data)
{
    char desc[32];
    int len = 0;

    int enable = 0;

    len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
    if (copy_from_user(desc, buffer, len))
        return 0;
    desc[len] = '\0';

    if (kstrtoint(desc, 0, &enable) == 0) {
        if (enable == 0) {
            mt_gpufreq_voltage_enable_set(0);
            mt_gpufreq_volt_enable = false;
        } else if (enable == 1) {
            mt_gpufreq_voltage_enable_set(1);
            mt_gpufreq_volt_enable = true;
        } else
            gpufreq_warn("bad argument!! should be 0 or 1 [0: disable, 1: enable]\n");
    } else
        gpufreq_warn("bad argument!! should be 0 or 1 [0: disable, 1: enable]\n");

    return count;
}

/***************************
 * show current specific frequency status
 ****************************/
static int mt_gpufreq_fixed_freq_volt_proc_show(struct seq_file *m, void *v)
{
    if (mt_gpufreq_fixed_freq_volt_state) {
        seq_puts(m, "gpufreq fixed frequency enabled\n");
        seq_printf(m, "fixed frequency = %d\n", mt_gpufreq_fixed_frequency);
        seq_printf(m, "fixed voltage = %d\n", mt_gpufreq_fixed_voltage);
    } else
        seq_puts(m, "gpufreq fixed frequency disabled\n");
    return 0;
}

/***********************
 * enable specific frequency
 ************************/
static void _mt_gpufreq_fixed_freq(int fixed_freq)
{
    /* freq (KHz) */
    if ((fixed_freq >= GPUFREQ_LAST_FREQ_LEVEL)
        && (fixed_freq <= GPU_DVFS_FREQ0)) {
        gpufreq_dbg("@ %s, mt_gpufreq_clock_switch1 fix frq = %d, fix volt = %d, volt = %d\n",
            __func__, mt_gpufreq_fixed_frequency, mt_gpufreq_fixed_voltage, g_cur_gpu_volt);
        mt_gpufreq_fixed_freq_volt_state = true;
        mt_gpufreq_fixed_frequency = fixed_freq;
        mt_gpufreq_fixed_voltage = g_cur_gpu_volt;
        mt_gpufreq_voltage_enable_set(1);
        gpufreq_dbg("@ %s, mt_gpufreq_clock_switch2 fix frq = %d, fix volt = %d, volt = %d\n",
            __func__, mt_gpufreq_fixed_frequency, mt_gpufreq_fixed_voltage, g_cur_gpu_volt);
        mt_gpufreq_clock_switch(mt_gpufreq_fixed_frequency);
    }
}

static void _mt_gpufreq_fixed_volt(int fixed_volt)
{
        gpufreq_dbg("@ %s, mt_gpufreq_volt_switch1 fix frq = %d, fix volt = %d, volt = %d\n",
            __func__, mt_gpufreq_fixed_frequency, mt_gpufreq_fixed_voltage, g_cur_gpu_volt);
        mt_gpufreq_fixed_freq_volt_state = true;
        mt_gpufreq_fixed_frequency = g_cur_gpu_freq;
        mt_gpufreq_fixed_voltage = fixed_volt * 100;
        mt_gpufreq_voltage_enable_set(1);
        gpufreq_dbg("@ %s, mt_gpufreq_volt_switch2 fix frq = %d, fix volt = %d, volt = %d\n",
            __func__, mt_gpufreq_fixed_frequency, mt_gpufreq_fixed_voltage, g_cur_gpu_volt);
        mt_gpufreq_volt_switch(g_cur_gpu_volt, mt_gpufreq_fixed_voltage);
        g_cur_gpu_volt = mt_gpufreq_fixed_voltage;
}

static ssize_t mt_gpufreq_fixed_freq_volt_proc_write(struct file *file, const char __user *buffer,
                             size_t count, loff_t *data)
{
    char desc[32];
    int len = 0;

    int fixed_freq = 0;
    int fixed_volt = 0;

    len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
    if (copy_from_user(desc, buffer, len))
        return 0;
    desc[len] = '\0';

    if (sscanf(desc, "%d %d", &fixed_freq, &fixed_volt) == 2) {
        if ((fixed_freq == 0) && (fixed_volt == 0)) {
            mt_gpufreq_fixed_freq_volt_state = false;
            mt_gpufreq_fixed_frequency = 0;
            mt_gpufreq_fixed_voltage = 0;
        } else {
            g_cur_gpu_freq = _mt_gpufreq_get_cur_freq();
            
            if (fixed_freq > g_cur_gpu_freq) {
                _mt_gpufreq_fixed_volt(fixed_volt);
                _mt_gpufreq_fixed_freq(fixed_freq);
            } else {
                _mt_gpufreq_fixed_freq(fixed_freq);
                _mt_gpufreq_fixed_volt(fixed_volt);
            }
        }
    } else
        gpufreq_warn("bad argument!! should be [enable fixed_freq fixed_volt]\n");

    return count;
}
/***************************
 * show current voltage
 ****************************/
static int mt_gpufreq_cur_volt_proc_show(struct seq_file *m, void *v)
{
	u32 rdata = 0;

	rdata = regulator_get_voltage(mt_gpufreq_pmic->reg_vgpu); /* unit uv: micro voltage */

	if (rdata != 0)
		seq_printf(m, "%d\n", rdata);
	else
		seq_printf(m, "reg_vgpu read current voltage fail\n");

    return 0;
}

/***********************
 * set current voltage
 ************************/
static ssize_t mt_gpufreq_cur_volt_proc_write(struct file *file, const char __user *buffer,
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
        regulator_set_voltage(mt_gpufreq_pmic->reg_vgpu,
                    volt, volt + 10000);
    } else
        gpufreq_warn("bad argument_1!! argument should be \"0\"\n");

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

PROC_FOPS_RW(gpufreq_dbg);
PROC_FOPS_RW(gpufreq_state);
PROC_FOPS_RO(gpufreq_opp_dump);
PROC_FOPS_RW(gpufreq_opp_freq);
PROC_FOPS_RW(gpufreq_opp_max_freq);
PROC_FOPS_RO(gpufreq_var_dump);
PROC_FOPS_RW(gpufreq_volt_enable);
PROC_FOPS_RW(gpufreq_fixed_freq_volt);
PROC_FOPS_RW(gpufreq_cur_volt);

static int mt_gpufreq_create_procfs(void)
{
    struct proc_dir_entry *dir = NULL;
    int i;

    struct pentry {
        const char *name;
        const struct file_operations *fops;
    };

    const struct pentry entries[] = {
        PROC_ENTRY(gpufreq_dbg),
        PROC_ENTRY(gpufreq_state),
        PROC_ENTRY(gpufreq_opp_dump),
        PROC_ENTRY(gpufreq_opp_freq),
        PROC_ENTRY(gpufreq_opp_max_freq),
        PROC_ENTRY(gpufreq_var_dump),
        PROC_ENTRY(gpufreq_volt_enable),
        PROC_ENTRY(gpufreq_fixed_freq_volt),
        PROC_ENTRY(gpufreq_cur_volt),
    };


    dir = proc_mkdir("gpufreq", NULL);

    if (!dir) {
        gpufreq_err("fail to create /proc/gpufreq @ %s()\n", __func__);
        return -ENOMEM;
    }

    for (i = 0; i < ARRAY_SIZE(entries); i++) {
        if (!proc_create
            (entries[i].name, S_IRUGO | S_IWUSR | S_IWGRP, dir, entries[i].fops))
            gpufreq_err("@%s: create /proc/gpufreq/%s failed\n", __func__,
                    entries[i].name);
    }

    return 0;
}
#endif              /* CONFIG_PROC_FS */

/**********************************
 * mediatek gpufreq initialization
 ***********************************/
static int __init mt_gpufreq_init(void)
{
    int ret = 0;

    gpufreq_info("@%s\n", __func__);

#ifdef CONFIG_PROC_FS

    /* init proc */
    if (mt_gpufreq_create_procfs())
        goto out;

#endif              /* CONFIG_PROC_FS */

    /* register platform device/driver */
#if !defined(CONFIG_OF)
    ret = platform_device_register(&mt_gpufreq_pdev);
    if (ret) {
        gpufreq_err("fail to register gpufreq device @ %s()\n", __func__);
        goto out;
    }
#endif
    ret = platform_driver_register(&mt_gpufreq_pdrv);
    if (ret) {
        gpufreq_err("fail to register gpufreq driver @ %s()\n", __func__);
#if !defined(CONFIG_OF)
        platform_device_unregister(&mt_gpufreq_pdev);
#endif
    }

out:
    return ret;
}

static void __exit mt_gpufreq_exit(void)
{
    platform_driver_unregister(&mt_gpufreq_pdrv);
#if !defined(CONFIG_OF)
    platform_device_unregister(&mt_gpufreq_pdev);
#endif
}

module_init(mt_gpufreq_init);
module_exit(mt_gpufreq_exit);

MODULE_DESCRIPTION("MediaTek GPU Frequency Scaling driver");
MODULE_LICENSE("GPL");



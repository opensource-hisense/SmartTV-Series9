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

/**
 * @file    mt_tc.c
 * @brief   
 *
 */

/*=============================================================
 * Include files
 *=============================================================
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/io.h>
/* system includes */
#include "mtk_tc.h"
#include "mtk_ptp.h"


/*=============================================================
 * Local function definition
 *=============================================================
 */
#define TS_LOW_CRITERIAL 3193 //MT5890/5861: 4096*0.682*3.2/2.8=3192  / MT8135:4096*0.682*1.8/1.5=3352(3350)
#define TS_4CH 1
#define TS_ENUM_MAX 4

#define THERMAL_DEBUG_LOG 1
static int mtktscpu_debug_log = 0;
/*=============================================================
 * Macro definition
 *=============================================================
 */
#define tscpu_dprintk(fmt, args...)   \
		do {									\
			if (mtktscpu_debug_log) { 			   \
				pr_err("Power/CPU_Thermal" fmt, ##args); \
			}									\
		} while (0)


/*=============================================================
 *Local variable definition
 *=============================================================
 */
static int TS_CAL_Done = false;
static __s32 g_x_roomt[TS_ENUM_MAX] = { 0 };
static __s32 g_gain = 0;;
static __s32 g_o_slope_sign = 0; // Buffer Gain 3.2 calibration slop (1.xx>=1.65, O_SLOPE_SIGN=0, 1.xx<1.65, O_SLOPE_SIGN=1)
static __s32 g_o_slope = 0; // Buffer Gain 3.2 calibration value
static __s32 g_degc_cali = 0;  // Environment tempature (Unit: degree C) * 2
static __s32 g_adc_ge = 0;  // ADC Gain Error + 512 (Uint: 1/4096) 
static __s32 g_adc_oe = 0;  // ADC Offset Error + 512  (Uint: 1/4096)
static __s32 g_o_vtsmcu1 = 0; // TSU0 Offset (Uint: 1/4096)
static __s32 g_o_vtsmcu2 = 0; // TSU1 Offset (Uint: 1/4096)
static __s32 g_o_vtsmcu3 = 0; // TSU2 Offset (Uint: 1/4096)
#if TS_4CH
static __s32 g_o_vtsmcu4 = 0; // TSU2 Offset (Uint: 1/4096)
#endif
static __s32 g_adc_cali_en = 0; // Gain and Offset calibration
static __s32 g_id = 0;
static __s32 g_ge = 0;  // ADC Real Gain Error  (Uint: 1*10000/4096) 
static __s32 g_oe = 0;  // ADC Real Offset Error  (Uint: 1*10000/4096) 


#define tc_read(addr)	__raw_readl(addr)
#define RG_TEMPSPARE1_GPU_TEMP 0xffff<<0//[15:0]
#define RG_TEMPSPARE1_CPU_TEMP 0xffff<<16//[31:16]

/**
 * @brief vDrvThermal_Cal_Prepare
 * Get thermal sensor coefficient from eFuse
 * @param  void
 * @retval void
 */
static void vDrvThermal_Cal_Prepare(void)
{
	unsigned int temp0, temp1;
	#if TS_4CH	
	unsigned int temp2;
	#endif

	#if 1
	temp0 = tc_read(EFUSE_TS1_ADDR);
	temp1 = tc_read(EFUSE_TS2_ADDR);
	#if TS_4CH
	temp2 = tc_read(EFUSE_TS3_ADDR);
	#endif	
	#else
	temp0 = 0;
	temp1 = 0;
	#if TS_4CH
	temp2 = 0;
	#endif
	#endif
	
	#if THERMAL_DEBUG_LOG
	tscpu_dprintk("[Power/CPU_Thermal] [Thermal calibration] Reg(0x100086C8)=0x%x, Reg(0x100086CC)=0x%x Reg(0x100086D0)=0x%x\n", temp0, temp1, temp2);
	#endif
	
	g_adc_ge = (temp1 & 0xFFC00000)>>22;
	g_adc_oe = (temp1 & 0x003FF000)>>12;
	
	g_o_vtsmcu1 = (temp0 & 0x03FE0000)>>17;
	g_o_vtsmcu2 = (temp0 & 0x0001FF00)>>8;
	g_o_vtsmcu3 = (temp1 & 0x000001FF);
	#if TS_4CH
	g_o_vtsmcu4 = (temp2 & 0x000001FF);
	#endif
	
	g_degc_cali = (temp0 & 0x0000007E)>>1;
	g_adc_cali_en = (temp0 & 0x00000001);
	g_o_slope_sign = (temp0 & 0x00000080)>>7;
	g_o_slope = (temp0 & 0xFC000000)>>26;
	
	g_id = (temp1 & 0x00000200)>>9;
	if(g_id==0)
	{
		g_o_slope=0;
	}

	if(g_adc_cali_en == 1)
	{
		//thermal_enable = true;        
	}
	else
	{
		#if 0//(defined(CC_MT5891)||defined(CC_MT5863)) // U**
		g_adc_ge = 512;
		g_adc_oe = 512;
		g_degc_cali = 50;
		g_o_slope = 2;	// 165+2 = 167 1.67	
		g_o_slope_sign = 0;
		g_o_vtsmcu1 = 205;
		g_o_vtsmcu2 = 205;
		g_o_vtsmcu3 = 205;
		#if TS_4CH
		g_o_vtsmcu4 = 205;		
		#endif
		#else
		g_adc_ge = 512;
		g_adc_oe = 512;
		g_degc_cali = 40;
		g_o_slope = 0;
		g_o_slope_sign = 0;
		g_o_vtsmcu1 = 260;
		g_o_vtsmcu2 = 260;
		g_o_vtsmcu3 = 260;
		#if TS_4CH
		g_o_vtsmcu4 = 260;		
		#endif		
		#endif
	}

	#if THERMAL_DEBUG_LOG	
	tscpu_dprintk("== [Power/CPU_Thermal] [Thermal calibration] == \n g_adc_ge = 0x%x ,\n g_adc_oe = 0x%x,\n g_degc_cali = 0x%x,\n g_adc_cali_en = 0x%x,\n", 
		g_adc_ge, g_adc_oe, g_degc_cali, g_adc_cali_en);
	tscpu_dprintk("g_o_slope = 0x%x,\n g_o_slope_sign = 0x%x,\n g_id = 0x%x\n", 
		g_o_slope, g_o_slope_sign, g_id);
	#if TS_4CH
	tscpu_dprintk("[Thermal calibration] g_o_vtsmcu1 = 0x%x,\n g_o_vtsmcu2 = 0x%x,\n g_o_vtsmcu3 = 0x%x \n g_o_vtsmcu4 = 0x%x \n",
		g_o_vtsmcu1, g_o_vtsmcu2, g_o_vtsmcu3, g_o_vtsmcu4);
	#else
	tscpu_dprintk("[Thermal calibration] g_o_vtsmcu1 = 0x%x,\n g_o_vtsmcu2 = 0x%x,\n g_o_vtsmcu3 = 0x%x \n",
		g_o_vtsmcu1, g_o_vtsmcu2, g_o_vtsmcu3);
	#endif
	#endif
}

static void vDrvThermal_Cal_Prepare_2(void)
{
	int format_1, format_2, format_3, format_4= 0;

	//  [FT] ADC_GE[9:0] = GE*4096 + 512 (round to integer)-(1)
	//  [FT] ADC_OE[9:0] = OE*4096 + 512 (round to integer)-(2)
	g_ge = ((g_adc_ge - 512) * 10000 ) / 4096; // ge * 10000
	g_oe = (g_adc_oe - 512);
	g_gain = (10000 + g_ge); // gain * 10000

	// [FT] O_VTSMCU1=Y_VTS1-3192
	// Ideal ADC Value * ADC_Gain + ADC offset  = ADC Value.... (3)
	// Ideal ADC Value * ADC_Gain = ADC Value - ADC offset
	// format_X = Ideal ADC Value * ADC_Gain = (g_o_vtsmcu1 + TS_LOW_CRITERIAL) - (g_oe)
	format_1 = (g_o_vtsmcu1 + TS_LOW_CRITERIAL - g_oe);
	format_2 = (g_o_vtsmcu2 + TS_LOW_CRITERIAL - g_oe);
	format_3 = (g_o_vtsmcu3 + TS_LOW_CRITERIAL - g_oe);
	#if TS_4CH
	format_4 = (g_o_vtsmcu4 + TS_LOW_CRITERIAL - g_oe);
	#else
	format_4 = (g_o_vtsmcu3 + TS_LOW_CRITERIAL - g_oe);
	#endif

	// TSUV_S1 / 2.8V = Ideal ADC Value / 4096 .... (2)
	// g_x_roomt[0]=TSUV_S1 / 2.8V = Ideal ADC Value / 4096 = format_X/ADC_Gain/4096
	g_x_roomt[0] = (((format_1 * 10000) / 4096) * 10000) / g_gain; // x_roomt * 10000
	g_x_roomt[1] = (((format_2 * 10000) / 4096) * 10000) / g_gain; // x_roomt * 10000
	g_x_roomt[2] = (((format_3 * 10000) / 4096) * 10000) / g_gain; // x_roomt * 10000
	g_x_roomt[3] = (((format_4 * 10000) / 4096) * 10000) / g_gain; // x_roomt * 10000

	#if THERMAL_DEBUG_LOG
	tscpu_dprintk("[Power/CPU_Thermal] [Thermal calibration] g_ge = %d, g_oe = %d, g_gain = %d, g_x_roomt1 = %d, g_x_roomt2 = %d, g_x_roomt3 = %d, g_x_roomt4 = %d\n",
		g_ge, g_oe, g_gain, g_x_roomt[0], g_x_roomt[1], g_x_roomt[2], g_x_roomt[3]);
	#endif

	// vDrvGetEFuse_ThermalSesnorData();
}

void get_thermal_slope_intercept(struct TS_PTPOD *ts_info, enum thermal_bank_name ts_bank)
{
	unsigned int temp0, temp1, temp2;
	struct TS_PTPOD ts_ptpod;
	__s32 x_roomt;

	tscpu_dprintk("get_thermal_slope_intercept\n");

	if(TS_CAL_Done == false)
	{
		TS_CAL_Done = true;
		vDrvThermal_Cal_Prepare();
		vDrvThermal_Cal_Prepare_2();
	}
	/* chip dependent */

	/*
	*Husky
	*Bank0: CPU	(TSMCU2 + TSMCU4)
	*Bank1: GPU	(TSMCU1 + TSMCU3)
	*/

	/*
	*   If there are two or more sensors in a bank, choose the sensor calibration value of
	*   the dominant sensor. You can observe it in the thermal doc provided by Thermal DE.
	*   For example,
	*   Bank 1 is for SOC + GPU. Observe all scenarios related to GPU tests to
	*   determine which sensor is the highest temperature in all tests.
	*   Then, It is the dominant sensor.
	*   (Confirmed by Thermal DE Alfred Tsai)
	*/

	switch (ts_bank) {
	case THERMAL_BANK0:
		x_roomt = g_x_roomt[1];
		break;
	case THERMAL_BANK1:
		x_roomt = g_x_roomt[0];
		break;
	default:		/*choose high temp */
		x_roomt = g_x_roomt[1];
		break;
	}

	/*
	*   The equations in this function are confirmed by Thermal DE Alfred Tsai.
	*   Don't have to change until using next generation thermal sensors.
	 */

	/* Ycurr = [(Tcurr - DEGC_cali/2)*(1612+O_slope*10)/10*(18/15)*(1/10000)+X_roomtabb]*Gain*4096 + OE */

	temp0 = (10000 * 100000 / g_gain) * 28 / 32;

	if (g_o_slope_sign == 0)
		temp1 = (temp0 * 10) / (1650 + g_o_slope * 10);
	else
		temp1 = (temp0 * 10) / (1650 - g_o_slope * 10);

	ts_ptpod.ts_MTS = temp1;

	temp0 = (g_degc_cali * 10 / 2);
	temp1 = ((10000 * 100000 / 4096 / g_gain) * g_oe + x_roomt * 10) * 28 / 32;

	if (g_o_slope_sign == 0)
		temp2 = temp1 * 100 / (1650 + g_o_slope * 10);
	else
		temp2 = temp1 * 100 / (1650 - g_o_slope * 10);

	ts_ptpod.ts_BTS = (temp0 + temp2 - 210) * 4 / 10;

	ts_info->ts_MTS = ts_ptpod.ts_MTS;
	ts_info->ts_BTS = ts_ptpod.ts_BTS;
	tscpu_dprintk("ts_MTS=%d, ts_BTS=%d\n", ts_ptpod.ts_MTS, ts_ptpod.ts_BTS);
}


int get_immediate_ts_cpu_wrap(void)
{
	unsigned int curr_temp;

	curr_temp = ((tc_read(PTP_TEMPSPARE1)&0xffff0000)>>16)*100;
	tscpu_dprintk("get_immediate_ts1_wrap curr_temp=%d\n", curr_temp);

	return curr_temp;
}

int get_immediate_ts_gpu_wrap(void)
{
	unsigned int curr_temp;

	curr_temp = (tc_read(PTP_TEMPSPARE1)&0xffff)*100;
	tscpu_dprintk("get_immediate_ts1_wrap curr_temp=%d\n", curr_temp);

	return curr_temp;
}


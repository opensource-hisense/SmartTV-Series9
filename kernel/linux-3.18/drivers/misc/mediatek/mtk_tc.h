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


#define CONFIG_THERMAL

enum thermal_bank_name {
	THERMAL_BANK0     = 0, /*CPU (TS_MCU1) (TS1)*/
	THERMAL_BANK1     = 1, /*GPU (TS_MCU2) (TS2)*/
	THERMAL_BANK_NUM
};

struct TS_PTPOD {
	unsigned int ts_MTS;
	unsigned int ts_BTS;
};

extern void get_thermal_slope_intercept(struct TS_PTPOD *ts_info, enum thermal_bank_name ts_bank);
extern int get_immediate_ts_cpu_wrap(void);
extern int get_immediate_ts_gpu_wrap(void);


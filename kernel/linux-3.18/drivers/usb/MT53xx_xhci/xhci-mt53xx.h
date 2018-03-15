/*
 * Copyright (c) 2015 MediaTek Inc.
 * Author:
 *  Cheng Yuan <cheng.yuan@mediatek.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */


#include <linux/usb.h>
#include "xhci.h"
#include "xhci-mtk.h"
#define VBUS_ON 1
#define VBUS_OFF 0

void SSUSB_Vbushandler(uint8_t bPortNum, uint8_t bOn);


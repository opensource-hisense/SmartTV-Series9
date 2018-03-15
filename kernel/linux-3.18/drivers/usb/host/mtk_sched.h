/*-----------------------------------------------------------------------------
 *\drivers\usb\host\mtk_sched.h
 *
 * MT53xx USB driver
 *
 * Copyright (c) 2008-2012 MediaTek Inc.
 * $Author: zhendong.wang $
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
 *---------------------------------------------------------------------------*/
#ifndef __MTK_EP_QUEUE_H__
#define __MTK_EP_QUEUE_H__

extern uint16_t mtk_get_fifo(MGC_LinuxCd *pThis, uint16_t size);
extern int mtk_put_fifo(MGC_LinuxCd *pThis, uint16_t addr, int size);
extern void mtk_dump_fifo_node(MGC_LinuxCd *pThis);
extern uint8_t mtk_select_endpoint(MGC_LinuxCd *pThis, struct urb *pUrb);

#endif

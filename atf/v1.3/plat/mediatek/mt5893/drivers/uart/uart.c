/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to MediaTek Inc. and/or its licensors. Without
 * the prior written permission of MediaTek inc. and/or its licensors, any
 * reproduction, modification, use or disclosure of MediaTek Software, and
 * information contained herein, in whole or in part, shall be strictly
 * prohibited.
 *
 * MediaTek Inc. (C) 2010. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER
 * ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 * NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH
 * RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 * INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES
 * TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
 * RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
 * OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK
 * SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE
 * RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S
 * ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE
 * RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE
 * MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE
 * CHARGE PAID BY RECEIVER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek
 * Software") have been modified by MediaTek Inc. All revisions are subject to
 * any receiver's applicable license agreements with MediaTek Inc.
 */
#include <stdint.h>
#include <assert.h>     //for assert()
#include <console.h>  //for console_init, console_putc, console_getc
#include "typedefs.h"
#include "platform_def.h"


#define RS232_READ32(off)       DRV_Reg32((RS232_BASE)+(off))
#define RS232_WRITE32(off, val) DRV_WriteReg32((RS232_BASE)+(off), (val))

#define REG_U0_OUTPORT          (0x0000)
#define REG_U0_STATUS           (0x004c)
    #define STATUS_TXBUF        (0x1f00)



static unsigned long IsOutputToUARTFlag=1;


void PutUARTByte (const char c)
{
    while ((RS232_READ32(REG_U0_STATUS) & STATUS_TXBUF) != STATUS_TXBUF)
    { }
    
    RS232_WRITE32(REG_U0_OUTPORT, (UINT32)(c));
    if (c == '\n')
    {
        RS232_WRITE32(REG_U0_OUTPORT, (UINT32)'\r');
    }
}

int GetUARTBytes(u8 *buf, u32 size, u32 tmo_ms)
{
    return 0;
}

void console_init(unsigned long base_addr)
{
    //do nothing
}

int console_putc(int c)
{
    if(IsOutputToUARTFlag){
        PutUARTByte (c);
    }
    return c;
}

int console_getc(void)
{
    unsigned char c = 0;
    if(IsOutputToUARTFlag){
        GetUARTBytes(&c, 1, 10);
    }
    return c;
}
void set_uart_flag(void)
{
    IsOutputToUARTFlag=1;
}
void clear_uart_flag(void)
{
    IsOutputToUARTFlag=0;
}

void pdwnc_trigger_shutdown(void)
{
	DRV_WriteReg32(GIC_DIST_BASE + 0x0,0);//disable GIC to block the interrupt path to CPU.
	// notify uP to prepare for dram-self-refresh mode
	DRV_WriteReg32(PDWNC_BASE+0x904, (DRV_Reg32(PDWNC_BASE+0x904) | 0x01));
}


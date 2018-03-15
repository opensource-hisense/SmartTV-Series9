/*----------------------------------------------------------------------------*
 * Copyright Statement:                                                       *
 *                                                                            *
 *   This software/firmware and related documentation ("MediaTek Software")   *
 * are protected under international and related jurisdictions'copyright laws *
 * as unpublished works. The information contained herein is confidential and *
 * proprietary to MediaTek Inc. Without the prior written permission of       *
 * MediaTek Inc., any reproduction, modification, use or disclosure of        *
 * MediaTek Software, and information contained herein, in whole or in part,  *
 * shall be strictly prohibited.                                              *
 * MediaTek Inc. Copyright (C) 2010. All rights reserved.                     *
 *                                                                            *
 *   BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND     *
 * AGREES TO THE FOLLOWING:                                                   *
 *                                                                            *
 *   1)Any and all intellectual property rights (including without            *
 * limitation, patent, copyright, and trade secrets) in and to this           *
 * Software/firmware and related documentation ("MediaTek Software") shall    *
 * remain the exclusive property of MediaTek Inc. Any and all intellectual    *
 * property rights (including without limitation, patent, copyright, and      *
 * trade secrets) in and to any modifications and derivatives to MediaTek     *
 * Software, whoever made, shall also remain the exclusive property of        *
 * MediaTek Inc.  Nothing herein shall be construed as any transfer of any    *
 * title to any intellectual property right in MediaTek Software to Receiver. *
 *                                                                            *
 *   2)This MediaTek Software Receiver received from MediaTek Inc. and/or its *
 * representatives is provided to Receiver on an "AS IS" basis only.          *
 * MediaTek Inc. expressly disclaims all warranties, expressed or implied,    *
 * including but not limited to any implied warranties of merchantability,    *
 * non-infringement and fitness for a particular purpose and any warranties   *
 * arising out of course of performance, course of dealing or usage of trade. *
 * MediaTek Inc. does not provide any warranty whatsoever with respect to the *
 * software of any third party which may be used by, incorporated in, or      *
 * supplied with the MediaTek Software, and Receiver agrees to look only to   *
 * such third parties for any warranty claim relating thereto.  Receiver      *
 * expressly acknowledges that it is Receiver's sole responsibility to obtain *
 * from any third party all proper licenses contained in or delivered with    *
 * MediaTek Software.  MediaTek is not responsible for any MediaTek Software  *
 * releases made to Receiver's specifications or to conform to a particular   *
 * standard or open forum.                                                    *
 *                                                                            *
 *   3)Receiver further acknowledge that Receiver may, either presently       *
 * and/or in the future, instruct MediaTek Inc. to assist it in the           *
 * development and the implementation, in accordance with Receiver's designs, *
 * of certain softwares relating to Receiver's product(s) (the "Services").   *
 * Except as may be otherwise agreed to in writing, no warranties of any      *
 * kind, whether express or implied, are given by MediaTek Inc. with respect  *
 * to the Services provided, and the Services are provided on an "AS IS"      *
 * basis. Receiver further acknowledges that the Services may contain errors  *
 * that testing is important and it is solely responsible for fully testing   *
 * the Services and/or derivatives thereof before they are used, sublicensed  *
 * or distributed. Should there be any third party action brought against     *
 * MediaTek Inc. arising out of or relating to the Services, Receiver agree   *
 * to fully indemnify and hold MediaTek Inc. harmless.  If the parties        *
 * mutually agree to enter into or continue a business relationship or other  *
 * arrangement, the terms and conditions set forth herein shall remain        *
 * effective and, unless explicitly stated otherwise, shall prevail in the    *
 * event of a conflict in the terms in any agreements entered into between    *
 * the parties.                                                               *
 *                                                                            *
 *   4)Receiver's sole and exclusive remedy and MediaTek Inc.'s entire and    *
 * cumulative liability with respect to MediaTek Software released hereunder  *
 * will be, at MediaTek Inc.'s sole discretion, to replace or revise the      *
 * MediaTek Software at issue.                                                *
 *                                                                            *
 *   5)The transaction contemplated hereunder shall be construed in           *
 * accordance with the laws of Singapore, excluding its conflict of laws      *
 * principles.  Any disputes, controversies or claims arising thereof and     *
 * related thereto shall be settled via arbitration in Singapore, under the   *
 * then current rules of the International Chamber of Commerce (ICC).  The    *
 * arbitration shall be conducted in English. The awards of the arbitration   *
 * shall be final and binding upon both parties and shall be entered and      *
 * enforceable in any court of competent jurisdiction.                        *
 *----------------------------------------------------------------------------*
 */

/*
 ** @file dram_suspend.s
 *  provides functions to enter dram self-refresh mode.
 *  Used in Nucleus AP only.
 *
 */

/*
 *  Entering DRAM suspend mode (and never return)
 *
 *  @return  Will not return from this function.
 */
//@@;extern void mt53xx_dram_suspend(void);

//@@;#define DRAM_READ32(offset)           IO_READ32(DRAM_BASE, (offset)))
//@@;#define DRAM_BASE                   (IO_VIRT + 0x07000)
#define DRAM_BASE               (0xF0000000 + 0x07000)
#define DMARB_CHA_BASE          (0xF0000000 + 0x06000 + 0x150)
#define BIM_BASE                (0xF0000000 + 0x8000)
#define RS232_BASE              (0xF0000000 + 0xC000)
#define DRAM_CHB_BASE           (0xF0000000 + 0x0F000)
#define DMARB_CHB_BASE          (0xF0000000 + 0x10000 + 0x150)
#define DRAM_CHC_BASE           (0xF0000000 + 0x15000)
#define DMARB_CHC_BASE          (0xF0000000 + 0x11000 + 0x150)
#define PDWNC_BASE              (0xF0000000 + 0x28000 + 0x900)
#define CKGEN_BASE              (0xF0000000 + 0xd000)
#define REG_RW_TIMER2_LOW       (0x74)
#define REG_U0_OUTPORT          (0x0000)

    .global mt53xx_dram_suspend
    .global mt53xx_dram_suspend_sz

mt53xx_dram_suspend:
      dsb sy
      isb

   // @invalidate I+ D TLBS
   
			tlbi vmalle1is
			
    //@ turn MMU and I/D cache off
 
  		mrs	x1, sctlr_el1
			bic	x1, x1, #1					// disable the MMU
			bic	x1, x1, #1 << 2			// clear SCTLR.C
			bic	x1, x1, #1 << 12		// clear SCTLR.I
			msr	sctlr_el1, x1			
						
    	dsb sy
    	isb
   // @ add delay before protect all agent from accessing dram
    ldr     x0, =0x5000
    bl      SuspendDelay
   // @ MID setting
    ldr     x4, =DMARB_CHA_BASE
    ldr     x0, =0
    str     x0, [x4, #0x208]
    str     x0, [x4, #0x218]
    str     x0, [x4, #0x228]
    str     x0, [x4, #0x238]
    str     x0, [x4, #0x248]

    ldr     x4, =DMARB_CHB_BASE
    str     x0, [x4, #0x208]
    str     x0, [x4, #0x218]
    str     x0, [x4, #0x228]
    str     x0, [x4, #0x238]
    str     x0, [x4, #0x248]

    ldr     x4, =DMARB_CHC_BASE
    str     x0, [x4, #0x208]
    str     x0, [x4, #0x218]
    str     x0, [x4, #0x228]
    str     x0, [x4, #0x238]
    str     x0, [x4, #0x248]

    ldr     x0, =0x5000
    bl      SuspendDelay

//@///////////////////////////////////////////////////////////////////////////////////
    mov     x0, #'A'
    bl      SuspendPrintChar
//@ vIO32Write4B((0xf0006130,vIO32Write4B(0xf0006130) | (1<<0)) @ //bit0:1
    ldr     x1, =0x0
ResetA:
    ldr    x4, =DMARB_CHA_BASE
    ldr     x0, [x4, #0x130]
    bic     x0, x0, #(1<<0)
    str     x0, [x4, #0x130]
    isb
    ldr     x0, [x4, #0x130]
    orr     x0, x0, #(1<<0)
    str     x0, [x4, #0x130]
    isb

//@ vIO32Read4B((0xf0006154) @ //bit0:1
CheckA:
    ldr     x4, =DMARB_CHA_BASE
    lsl     x0, x4, #0x4
    clz     x4, x0
    add     x4, x4, #68
    mov     x0, x4
    cmp     x4, #100
    b.ne    SuspendPrintChar //todo
    cmp     x4, #100
    bne     ResetA
    add     x1, x1, #0x1
    cmp     x1, #256
    bne     CheckA
    cmp     x4, #100
//    @ bne     NoReturn @ enable when debug

//@ Accessing DRAM_BASE. For CHA Only now. @ ddr.todo
    ldr     x4, =DRAM_BASE

//@ DRAM_WRITE32(0x04 ,DRAM_READ32(0x04) | (1<<26))@ //bit26,1:enable self-refresh
    ldr     x0, [x4, #0x4]
    orr     x0, x0, #(1<<26)
    str     x0, [x4, #0x4]
    isb

WhileLoopA:
    mov     x0, #'A'
    bl      SuspendPrintChar
    ldr     x0, [x4, #0x3b8]
    and     x0,x0,#0x00010000
    cmp     x0, #(1<<16)
    bne     WhileLoopA
//@///////////////////////////////////////////////////////////////////////////////////
    mov     x0, #'B'
    bl      SuspendPrintChar

//@@ vIO32Write4B((0xf0010130,vIO32Write4B(0xf0010130) | (1<<0)) @ //bit0:1
    ldr     x1, =0x0
ResetB:
    ldr     x4, =DMARB_CHB_BASE
    ldr     x0, [x4, #0x130]
    bic     x0, x0, #(1<<0)
    str     x0, [x4, #0x130]
    isb
    ldr     x0, [x4, #0x130]
    orr     x0, x0, #(1<<0)
    str     x0, [x4, #0x130]
    isb

//@ vIO32Read4B((0xf0010154) @ //bit0:1
CheckB:
    ldr     x4, =DMARB_CHB_BASE
    lsl     x0, x4, #0x4
    clz     x4, x0
    add     x4, x4, #68
    mov     x0, x4
    cmp     x4, #100
    b.ne    SuspendPrintChar //todo
    cmp     x4, #100
    bne     ResetB
    add     x1, x1, #0x1
    cmp     x1, #256
    bne     CheckB
    cmp     x4, #100
 //   @ bne     NoReturn @ enable when debug

//@ Accessing DRAM_CHB_BASE. For CHB Only now. @ ddr.todo
    ldr     x4, =DRAM_CHB_BASE

//@ DRAM_WRITE32(0x04 ,DRAM_READ32(0x04) | (1<<26))@ //bit26,1:enable self-refresh
    ldr     x0, [x4, #0x4]
    orr     x0, x0, #(1<<26)
    str     x0, [x4, #0x4]
    isb

WhileLoopB:
    mov     x0, #'B'
    bl      SuspendPrintChar
    ldr     x0, [x4, #0x3b8]
    and     x0,x0,#0x00010000
    cmp     x0, #(1<<16)
    bne     WhileLoopB
    
//@///////////////////////////////////////////////////////////////////////////////////
//@ vIO32Read4B(0xf00080f8) & 0x02000000 == 0x02000000  //0: SkipChannelC  1:not SkipChannelC
    ldr     x4, = BIM_BASE
    ldr     x0, [x4, #0xf8]
    and     x0,x0,#0x02000000
    cbz     x0, SkipChannelC  //todo
    //bne     SkipChannelC    
    mov     x0, #'C'
    bl      SuspendPrintChar

//@@ vIO32Write4B((0xf0011130,vIO32Write4B(0xf0011130) | (1<<0)) @ //bit0:1
    ldr     x1, =0x0
ResetC:
    ldr     x4, =DMARB_CHC_BASE
    ldr     x0, [x4, #0x130]
    bic     x0, x0, #(1<<0)
    str     x0, [x4, #0x130]
    isb
    ldr     x0, [x4, #0x130]
    orr     x0, x0, #(1<<0)
    str     x0, [x4, #0x130]
    isb

//@ vIO32Read4B((0xf0011154) @ //bit0:1
CheckC:
    ldr     x4, =DMARB_CHC_BASE
    lsl     x0, x4, #0x4
    clz     x4, x0
    add     x4, x4, #68
    mov     x0, x4
    cmp     x4, #100
    b.ne    SuspendPrintChar //todo
    cmp     x4, #100
    bne     ResetC
    add     x1, x1, #0x1
    cmp     x1, #256
    bne     CheckC
    cmp     x4, #100
//    @ bne     NoReturn @ enable when debug

//@ Accessing DRAM_CHC_BASE. For CHC Only now. @ ddr.todo
    ldr     x4, =DRAM_CHC_BASE

//@ DRAM_WRITE32(0x04 ,DRAM_READ32(0x04) | (1<<26))@ //bit26,1:enable self-refresh
    ldr     x0, [x4, #0x4]
    orr     x0, x0, #(1<<26)
    str     x0, [x4, #0x4]
    isb

WhileLoopC:
    mov     x0, #'C'
    bl      SuspendPrintChar
    ldr     x0, [x4, #0x3b8]
    and     x0,x0,#0x00010000
    cmp     x0, #(1<<16)
    bne     WhileLoopC

SkipChannelC:

//@///////////////////////////////////////////////////////////////////////////////////
//@ Print "Z\n"
    mov     x0, #'Z'
    bl      SuspendPrintChar
    mov     x0, #13
    bl      SuspendPrintChar
    mov     x0, #10
    bl      SuspendPrintChar

//@ vIO32Write4B(0xf0028904,u4IO32Read4B(0xf0028904) | 0x00000001) @ pdwnc.todo //use reserved bit0, set this bit
    ldr     x4, =PDWNC_BASE
    lsl     x0, x4,#0x4
    orr     x0, x0, #0x00000001
    str     w0, [x4,#0x904]

NoReturn:
    wfi
    B       NoReturn

SuspendPrintChar:
    ldr     x2, =RS232_BASE

//@ RS232_WRITE32(REG_U0_OUTPORT, (UINT32)(u4Char & 0xff));
    str     x0, [x2, #REG_U0_OUTPORT]
    ret				//RET      x30 //bx lr todo

SuspendDelay:

SuspLoop:
    sub x0, x0, #1
    cmp x0, #0
    bne SuspLoop
    ret				//RET      x30 //bx lr todo

mt53xx_dram_suspend_sz:
    .word   . - mt53xx_dram_suspend + 40  //@ +40 for possible LDR data.


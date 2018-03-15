/* Copyright Statement:
 * *
 * * This software/firmware and related documentation ("MediaTek Software") are
 * * protected under relevant copyright laws. The information contained herein
 * * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * * Without the prior written permission of MediaTek inc. and/or its licensors,
 * * any reproduction, modification, use or disclosure of MediaTek Software,
 * * and information contained herein, in whole or in part, shall be strictly prohibited.
 * *
 * * MediaTek Inc. (C) 2010. All rights reserved.
 * *
 * * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 * *
 * * The following software/firmware and/or related documentation ("MediaTek Software")
 * * have been modified by MediaTek Inc. All revisions are subject to any receiver's
 * * applicable license agreements with MediaTek Inc.
 * */

/******************************************************************************
 *
 *  this file contains the B3DS API definitions
 *
 ******************************************************************************/
#ifndef B3DS_API_H
#define B3DS_API_H

#include "l2c_api.h"

/* The version is used to pass PTS B3DS test in [IOPT/SDAS/BV-03-I ] */
#define B3DS_PROFILE_VERSION    0x100   /* Version 1.00 */

/* Bit map for B3DS roles */
#define B3DS_ROLE_3DD       0x01
#define B3DS_ROLE_3DG       0x02

#define B3DS_LT_ADDR                        0x01
#define B3DS_SET_CLK_NUM_CAP_TO_FILTER      0x7
#define B3DS_SET_CLK_LOCAL                  0x00

#define B3DS_CSB_PKT_TYPE                   0xFF1E  //BR only
#define B3DS_CSB_MIN_INTERVAL               0x0080  // 625us * 128slots = 80ms
#define B3DS_CSB_MAX_INTERVAL               0x0080
#define B3DS_CSB_SUPERVISION_TO             0x0280  // 625us * 640slots = 400ms

#define B3DS_CSB_DATA_NO_FRAGMENTATION      0x03

#define B3DS_SYNC_TRAIN_LEGACY_HOME         0x00
#define B3DS_SYNC_TRAIN_LEGACY_CINEMA       0x01
#define B3DS_SYNC_TRAIN_MIN_INTERVAL        0x0080  // 625us * 128slots = 80ms
#define B3DS_SYNC_TRAIN_MAX_INTERVAL        0x0080
#define B3DS_SYNC_TRAIN_TO                  0x0002EE00  //625us * 192000slots = 120s, 3DS: minimum 120s, Core Spec default 120s
#define B3DS_SYNC_TRAIN_MIN_TO              0x00000002

#define B3DS_EIR_ASSOCIAION_NOTIFICATION    0x01
#define B3DS_EIR_BATTERY_LEVEL_REPORTING    0x02
#define B3DS_EIR_STARTUP_SYNCHRONIZATION    0x04
#define B3DS_EIR_FACTORTY_TEST_MODE         0x80

#define B3DS_EIR_MAX_PATH_LOSS_THRESHOLD    0x64
#define B3DS_EIR_DEFAULT_INQ_TX_POWER       0x01

#define B3DS_LEGACY_EIR_FIXED_ID_0          0x0F
#define B3DS_LEGACY_EIR_FIXED_ID_1          0x00
#define B3DS_LEGACY_EIR_FIXED_DATA          0x00
#define B3DS_LEGACY_EIR_3D_SUPPORTED        0x01

#define B3DS_COMM_CHNL_OPCODE_CON_ANNO_MSG          0x00
#define B3DS_CON_ANNO_MSG_ASSOCIATION_NOTIFICATION  0x01
#define B3DS_CON_ANNO_MSG_BATTERY_LEVEL             0x02

#define B3DS_SET_2D_TIMEOUT                 4
#define B3DS_SET_2D_TIMEOUT_MS              (B3DS_SET_2D_TIMEOUT*1000)

#define B3DS_PERIOD_3D_24_HZ                41667

#define B3DS_PERIOD_INVALID                 0xFFFF

/*****************************************************************************
**  Type Definitions
*****************************************************************************/

#define B3DS_VEDIO_MODE_3D            0
#define B3DS_VEDIO_MODE_DUAL_VIEW     1
typedef UINT8 tB3DS_VEDIO_MODE;

#define B3DS_PERIOD_DYNAMIC_CALCULATED  0
#define B3DS_PERIOD_2D_MODE             1
#define B3DS_PERIOD_3D_MODE_48_HZ       2
#define B3DS_PERIOD_3D_MODE_50_HZ       3
#define B3DS_PERIOD_3D_MODE_59_94_HZ    4
#define B3DS_PERIOD_3D_MODE_60_HZ       5
#define B3DS_PERIOD_MAX                 B3DS_PERIOD_3D_MODE_60_HZ
typedef UINT8 tB3DS_PERIOD;

typedef void (tB3DS_CONNECT_ANNOUNCE_MSG_CB) (BD_ADDR bd_addr, UINT8 option, UINT8 battery_level);

// clayton: add for BCM
typedef void (tB3DS_SEND_FRAME_PEROID_CB) (uint16_t frame_period);

#if defined(MTK_LEGACY_B3DS_SUPPORT) && (MTK_LEGACY_B3DS_SUPPORT == TRUE)
typedef void (tB3DS_REF_PROTO_ASSOC_NOTIFY_EVT_CB) (UINT8 *addr);
#endif

/* This structure is used to register with B3DS profile
** It is passed as a parameter to B3DS_Register call.
*/
typedef struct
{
    tB3DS_CONNECT_ANNOUNCE_MSG_CB   *b3ds_con_anno_msg_cb;

    tB3DS_SEND_FRAME_PEROID_CB *b3ds_send_frame_period_cb;

#if defined(MTK_LEGACY_B3DS_SUPPORT) && (MTK_LEGACY_B3DS_SUPPORT == TRUE)
    tB3DS_REF_PROTO_ASSOC_NOTIFY_EVT_CB *b3ds_ref_proto_assoc_notify_ev_cb;
#endif
} tB3DS_REGISTER;

/*****************************************************************************
**  External Function Declarations
*****************************************************************************/
#ifdef __cplusplus
extern "C"
{
#endif

extern void B3DS_Register (tB3DS_REGISTER *p_register);
extern void B3DS_Deregister (void);
extern void B3DS_SetBroadcast (tB3DS_VEDIO_MODE vedio_mode, tB3DS_PERIOD period_mode, UINT32 delay);
extern void B3DS_SetBroadcastData(uint16_t delay, uint8_t dual_view, uint16_t right_open_offset, uint16_t right_close_offset, uint16_t left_open_offset, uint16_t left_close_offset);
extern void B3DS_ClockTick(UINT32 clock, UINT16 offset);

extern UINT8 B3DS_SetTraceLevel (UINT8 new_level);

extern void B3DS_Init (void);

extern void B3DS_ProcRefProtoAssocNotifyEvt(UINT8 *addr);
extern void B3DS_setOffsetData(UINT8 *offset_data,UINT16 offset_data_len);

#ifdef __cplusplus
}
#endif

#endif

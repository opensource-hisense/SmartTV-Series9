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

#pragma once
#ifdef HAS_BDROID_BUILDCFG
#include "bdroid_buildcfg.h"
#endif

/* **************************************************************************
 * **
 * ** General Config
 * **
 * *************************************************************************/
#if __STDC_VERSION__ < 199901L
#  ifndef FALSE
#    define FALSE 0
#  endif
#  ifndef TRUE
#    define TRUE (!FALSE)
#  endif
#else
#  include <stdbool.h>
#  ifndef FALSE
#    define FALSE  false
#  endif
#  ifndef TRUE
#    define TRUE   true
#  endif
#endif

#ifdef MTK_BLUEDROID_PATCH

#ifndef MTK_COMMON
#define MTK_COMMON                  TRUE
#endif

#ifndef BT_TRACE_VERBOSE
#define BT_TRACE_VERBOSE            FALSE
#endif

#ifndef SMP_DEBUG
#define SMP_DEBUG                   FALSE
#endif

#ifndef SDP_DEBUG_RAW
#define SDP_DEBUG_RAW               FALSE
#endif

/* **************************************************************************
 * **
 * ** MTK BT feature option
 * **
 * *************************************************************************/
/* MTK config in bt_stack.conf */
#ifndef MTK_STACK_CONFIG
#define MTK_STACK_CONFIG            TRUE
#endif

#if (MTK_STACK_CONFIG == TRUE)
#define MTK_STACK_CONFIG_VALUE_SEPARATOR        ","
#define MTK_STACK_CONFIG_PARTIAL_NAME_VALUE_MAX 1024

#define MTK_STACK_CONFIG_FPATH_LEN              1024
#define MTK_STACK_CONFIG_NUM_OF_HEXLIST         5
#define MTK_STACK_CONFIG_NUM_OF_HEXITEMS        260
#define MTK_STACK_CONFIG_NUM_OF_HEXROWITEMS     16
#define MTK_STACK_CONFIG_NUM_OF_HEXCOLUMNITEMS  16

/* Role switch & sniff subrating blacklist */
#ifndef MTK_STACK_CONFIG_BL
#define MTK_STACK_CONFIG_BL         TRUE
#endif

/* fwlog hcl hexlist */
#ifndef MTK_STACK_CONFIG_LOG
#define MTK_STACK_CONFIG_LOG        TRUE
#endif

/* Enable BT log on userdebug/eng Load w/o BTLogger */
#ifdef MTK_STACK_CONFIG_DEFAULT_OVERRIDE
#define MTK_STACK_CONFIG_DEFAULT_OVERRIDE   TRUE
#endif

#if !defined(MTK_LINUX_MW_STACK_LOG2FILE) && defined(MTK_LINUX)
#define MTK_LINUX_MW_STACK_LOG2FILE          TRUE
#define MTK_LINUX_STACK_LOG2FILE             FALSE
#elif !defined(MTK_LINUX_MW_STACK_LOG2FILE)
#define MTK_LINUX_MW_STACK_LOG2FILE          FALSE
#endif

#if !defined(MTK_LINUX_STACK_LOG2FILE) && defined(MTK_LINUX)
#define MTK_LINUX_STACK_LOG2FILE          TRUE
#elif !defined(MTK_LINUX_STACK_LOG2FILE)
#define MTK_LINUX_STACK_LOG2FILE          FALSE
#endif

#if !defined(MTK_LINUX_SNOOP_CONFIG) && defined(MTK_LINUX)
#define MTK_LINUX_SNOOP_CONFIG          TRUE
#elif !defined(MTK_LINUX_SNOOP_CONFIG)
#define MTK_LINUX_SNOOP_CONFIG          FALSE
#endif

#if !defined(MTK_LINUX_STACK_TRAC_CONFIG) && defined(MTK_LINUX)
#define MTK_LINUX_STACK_TRAC_CONFIG          TRUE
#elif !defined(MTK_LINUX_STACK_TRAC_CONFIG)
#define MTK_LINUX_STACK_TRAC_CONFIG          FALSE
#endif

#endif /* MTK_STACK_CONFIG */

#if !defined(MTK_LINUX)
/* Android A2DP Sink Config*/
#ifndef MTK_A2DP_SINK_SUPPORT
#define MTK_A2DP_SINK_SUPPORT TRUE
#endif

#if defined(MTK_A2DP_SINK_SUPPORT) && (MTK_A2DP_SINK_SUPPORT == TRUE)
#define USE_AUDIO_TRACK
#define MTK_PTS_AVRCP_CT_PTH_BV_01_C    FALSE
#endif

/* 3DS Related Config */
#ifndef MTK_B3DS_SUPPORT
#define MTK_B3DS_SUPPORT FALSE
#endif

#if defined(MTK_B3DS_SUPPORT) && (MTK_B3DS_SUPPORT == TRUE)
#ifndef MTK_LEGACY_B3DS_SUPPORT
#define MTK_LEGACY_B3DS_SUPPORT TRUE
#endif

#ifndef MTK_B3DS_FIX_COD
#define MTK_B3DS_FIX_COD TRUE
#endif

#ifndef MTK_B3DS_LEGACY_B3DS_SHORTEN_EIR
#define MTK_B3DS_LEGACY_B3DS_SHORTEN_EIR TRUE
#endif

#ifndef BTM_EIR_DEFAULT_FEC_REQUIRED
#define BTM_EIR_DEFAULT_FEC_REQUIRED FALSE
#endif

#ifndef L2CAP_UCD_INCLUDED
#define L2CAP_UCD_INCLUDED TRUE
#endif

#ifndef MTK_B3DS_DEMO
#define MTK_B3DS_DEMO TRUE
#endif

#ifndef BTM_DEFAULT_DISC_INTERVAL
#define BTM_DEFAULT_DISC_INTERVAL 0x0200
#endif

#ifndef MTK_B3DS_NOTIFY_UI
#define MTK_B3DS_NOTIFY_UI FALSE
#endif
#endif /* MTK_B3DS_SUPPORT */
#endif /* MTK_LINUX */

#if !defined(MTK_LINUX_SOCKET_PATH) && defined(MTK_LINUX)
#define MTK_LINUX_SOCKET_PATH       TRUE
#elif !defined(MTK_LINUX_SOCKET_PATH)
#define MTK_LINUX_SOCKET_PATH       FALSE
#endif

#if !defined(MTK_LINUX_OSI_SOCKET) && defined(MTK_LINUX)
#define MTK_LINUX_OSI_SOCKET        TRUE
#elif !defined(MTK_LINUX_OSI_SOCKET)
#define MTK_LINUX_OSI_SOCKET        FALSE
#endif

#if !defined(MTK_LINUX_EXPORT_API) && defined(MTK_LINUX)
#define MTK_LINUX_EXPORT_API        TRUE
#elif !defined(MTK_LINUX_EXPORT_API)
#define MTK_LINUX_EXPORT_API        FALSE
#endif

#if !defined(MTK_LINUX_ALARM) && defined(MTK_LINUX)
#define MTK_LINUX_ALARM        TRUE
#elif !defined(MTK_LINUX_ALARM)
#define MTK_LINUX_ALARM        FALSE
#endif


#if !defined(MTK_LINUX_AUDIO_COD) && defined(MTK_LINUX)
#define MTK_LINUX_AUDIO_COD         TRUE
#elif !defined(MTK_LINUX_AUDIO_COD)
#define MTK_LINUX_AUDIO_COD         FALSE
#endif

#if (MTK_LINUX_AUDIO_COD == TRUE)
#ifndef BTA_DM_COD
#define BTA_DM_COD {0x2C, 0x04, 0x14}
#endif
#endif

/* **************************************************************************
 * **
 * ** GAP
 * **
 * *************************************************************************/
#if !defined(MTK_LINUX_GAP) && defined(MTK_LINUX)
#define MTK_LINUX_GAP               TRUE
#else
#define MTK_LINUX_GAP               FALSE
#endif

#if (MTK_LINUX_GAP == TRUE)
#ifndef BTM_LOCAL_IO_CAPS
#define BTM_LOCAL_IO_CAPS           BTM_IO_CAP_OUT
#endif

#ifndef BTM_LOCAL_IO_CAPS_BLE
#define BTM_LOCAL_IO_CAPS_BLE       BTM_IO_CAP_OUT
#endif

#ifndef MTK_LINUX_GAP_PTS_TEST
#define MTK_LINUX_GAP_PTS_TEST      FALSE
#endif

#ifndef MTK_LINUX_RESET
#define MTK_LINUX_RESET             TRUE
#endif
#endif /* (MTK_LINUX_GAP == TRUE) */

/* **************************************************************************
 * **
 * ** BLE
 * **
 * *************************************************************************/
/* For MTK solution, vendor specific extensions is supported by default */
#ifndef BLE_VND_INCLUDED
#define BLE_VND_INCLUDED            TRUE
#endif

#ifndef BLE_PRIVACY_SPT
#define BLE_PRIVACY_SPT             TRUE
#endif

/* For MTK BT chip, set below CE length */
#ifndef MTK_BLE_CONN_MIN_CE_LEN
#define MTK_BLE_CONN_MIN_CE_LEN     0x0060
#endif

#ifndef MTK_BLE_CONN_MAX_CE_LEN
#define MTK_BLE_CONN_MAX_CE_LEN     0x0140
#endif

#ifndef BTM_BLE_SCAN_SLOW_INT_1
#define BTM_BLE_SCAN_SLOW_INT_1     96
#endif

#ifndef BTM_BLE_CONN_INT_MIN_DEF
#define BTM_BLE_CONN_INT_MIN_DEF    8
#endif

#ifndef BTM_BLE_CONN_INT_MAX_DEF
#define BTM_BLE_CONN_INT_MAX_DEF    8
#endif

#ifndef BTM_BLE_CONN_INT_MIN_LIMIT
#define BTM_BLE_CONN_INT_MIN_LIMIT  8
#endif

#ifndef BTM_BLE_CONN_TIMEOUT_DEF
#define BTM_BLE_CONN_TIMEOUT_DEF    300
#endif

#ifndef BTA_HOST_INTERLEAVE_SEARCH
#define BTA_HOST_INTERLEAVE_SEARCH  FALSE
#endif

#ifndef MTK_BTM_LE_SCAN_PARM_IGNORE
#define MTK_BTM_LE_SCAN_PARM_IGNORE FALSE
#endif

/* **************************************************************************
 * **
 * ** Advanced Audio Distribution Profile (A2DP)
 * **
 * *************************************************************************/
/* A2DP advanced codec support */

/* support A2DP SRC AAC codec */
/* Please enable this feature by changing option value defined here */
#ifndef MTK_A2DP_SRC_AAC_CODEC
#define MTK_A2DP_SRC_AAC_CODEC      FALSE
#endif

/* support A2DP SRC APTX codec */
/* Please enable this feature by adding feature option in ProjectConfig.mk */
#ifndef MTK_A2DP_SRC_APTX_CODEC
#define MTK_A2DP_SRC_APTX_CODEC     FALSE
#endif

/* Content protection: SCMS_T switch */
#ifndef BTA_AV_CO_CP_SCMS_T
#define BTA_AV_CO_CP_SCMS_T         TRUE
#endif

/* support a2dp audio dump debug feature */
#ifndef MTK_A2DP_PCM_DUMP
#define MTK_A2DP_PCM_DUMP           TRUE
#endif

#if !defined(MTK_LINUX_A2DP_PLUS) && defined(MTK_LINUX)
#define MTK_LINUX_A2DP_PLUS         TRUE
#else
#define MTK_LINUX_A2DP_PLUS         FALSE
#endif

/* only for MTK LINUX A2DP PLUS*/
#if (MTK_LINUX_A2DP_PLUS == TRUE)

#ifndef MTK_LINUX_A2DP_SINK_ENABLE
#define MTK_LINUX_A2DP_SINK_ENABLE  TRUE
#endif /* MTK_LINUX_A2DP_SINK_ENABLE */

#if (MTK_LINUX_A2DP_SINK_ENABLE == TRUE)
#define BTA_AV_SINK_INCLUDED        TRUE
#define MTK_LINUX_A2DP_AUDIO_TRACK  TRUE
#else
#define MTK_LINUX_A2DP_AUDIO_TRACK  FALSE
#endif

#ifndef MTK_LINUX_A2DP_DEFAULT_BITPOLL
#define MTK_LINUX_A2DP_DEFAULT_BITPOLL TRUE
#endif /* MTK_LINUX_A2DP_DEFAULT_BITPOLL */

#ifndef MTK_LINUX_A2DP_DEFAULT_SAMPLING_RATE
#define MTK_LINUX_A2DP_DEFAULT_SAMPLING_RATE TRUE
#endif /* MTK_LINUX_A2DP_DEFAULT_SAMPLING_RATE */

#if (MTK_LINUX_A2DP_DEFAULT_SAMPLING_RATE == TRUE)
#ifdef BTIF_A2DP_SRC_SAMPLING_RATE
#undef BTIF_A2DP_SRC_SAMPLING_RATE
#endif
#define BTIF_A2DP_SRC_SAMPLING_RATE 48000
#endif

#ifndef MTK_LINUX_AUDIO_LOG_DEFINE
#define MTK_LINUX_AUDIO_LOG_DEFINE TRUE
#endif /* MTK_LINUX_AUDIO_LOG_DEFINE */

#ifndef MTK_LINUX_AUDIO_LOG_CTRL
#define MTK_LINUX_AUDIO_LOG_CTRL TRUE
#endif /* MTK_LINUX_AUDIO_LOG_CTRL */

#ifndef BTA_AV_MAX_A2DP_MTU
#define BTA_AV_MAX_A2DP_MTU     668 //224 (DM5) * 3 - 4(L2CAP header)
#endif
#endif /* (MTK_LINUX_A2DP_PLUS == TRUE) */

/* For Low power bug fix */
#define BTIF_MEDIA_NUM_TICK             1
#if (MTK_LINUX_A2DP_PLUS == TRUE)
/* linux audio no need low power */
#define MTK_A2DP_BTIF_MEDIA_TIME_EXT    (1)
#else
#define MTK_A2DP_BTIF_MEDIA_TIME_EXT    (3)
#endif

/* Parameters relate to AAC codec */
#define MTK_A2DP_AAC_DEFAULT_BIT_RATE 200000
#define MTK_A2DP_BTIF_MEDIA_FR_PER_TICKS_AAC MTK_A2DP_BTIF_MEDIA_TIME_EXT
#define MTK_A2DP_AAC_ENC_INPUT_BUF_SIZE (4*1024)
#define MTK_A2DP_AAC_ENC_OUTPUT_BUF_SIZE (4*1024)
#define MTK_A2DP_AAC_READ_BUF_SIZE (BTIF_MEDIA_TIME_TICK * 44100 / 1000 * 2 * 2)
#define MTK_A2DP_AAC_LIMIT_MTU_SIZE (800)

#define BTIF_MEDIA_TIME_TICK            (20 * BTIF_MEDIA_NUM_TICK * MTK_A2DP_BTIF_MEDIA_TIME_EXT)
#define MAX_PCM_FRAME_NUM_PER_TICK      (14 * MTK_A2DP_BTIF_MEDIA_TIME_EXT)
#define MAX_PCM_ITER_NUM_PER_TICK       (3 * MTK_A2DP_BTIF_MEDIA_TIME_EXT)
#define MAX_OUTPUT_A2DP_FRAME_QUEUE_SZ  (MAX_PCM_FRAME_NUM_PER_TICK * 2)

/* **************************************************************************
 * **
 * ** Audio/Video Remote Control Profile (AVRCP)
 * **
 * *************************************************************************/
#ifndef MTK_LINUX_AVRCP_PLUS
#if defined(MTK_LINUX)
#define MTK_LINUX_AVRCP_PLUS TRUE
#else
#define MTK_LINUX_AVRCP_PLUS FALSE
#endif
#endif /* MTK_LINUX_AVRCP_PLUS */

#ifdef MTK_BT_AVRCP_TG_16
#define MTK_AVRCP_TG_15         TRUE
#define MTK_AVRCP_TG_16         TRUE
#define MTK_BT_PSM_COVER_ART    0x1077
#endif

#ifdef MTK_BT_AVRCP_TG_15
/**
 * support AVRCP 1.5 version target role
 * Following features are included
 * 1. Media player selection is mandandory (if support catogary 1)
 * 2. Get Folder Item - media player list (if support catogary 1)
 * 3. SDP record for AVRCP 1.5
 */
#ifndef MTK_AVRCP_TG_15
#define MTK_AVRCP_TG_15     TRUE
#endif

#define MTK_AVRCP_VERSION_SDP_POSITION  7
#define MTK_AVRCP_13_VERSION    0x03
#define MTK_AVRCP_14_VERSION    0x04
#endif /* MTK_BT_AVRCP_TG_15 */

/* switch for send AVRCP passthrough command to HAL or uinput */
#ifndef MTK_AVRCP_SEND_TO_HAL
#if defined(MTK_LINUX)
#define MTK_AVRCP_SEND_TO_HAL TRUE
#else
#define MTK_AVRCP_SEND_TO_HAL FALSE
#endif
#endif /* MTK_AVRCP_SEND_TO_HAL */


#if defined(MTK_LINUX)
#ifndef MTK_LINUX_AVRCP_GET_PLAY_STATUS
#define MTK_LINUX_AVRCP_GET_PLAY_STATUS FALSE
#endif /* MTK_LINUX_AVRCP_GET_PLAY_STATUS */
#endif

/* **************************************************************************
 * **
 * ** Audio/Video Control Transport Protocol (AVCTP)
 * **
 * *************************************************************************/
#if (MTK_AVRCP_TG_15 == TRUE)
/* enable AOSP avct browse included */
#define AVCT_BROWSE_INCLUDED        TRUE
#endif /* MTK_AVRCP_TG_15 */

/* **************************************************************************
 * **
 * ** HANDS FREE PROFILE (HFP)
 * **
 * *************************************************************************/
/* HF indicator feature for 17 */
#ifdef MTK_BT_HFP_AG_17
#define MTK_HFP_HF_IND              TRUE
#endif

/* Dual HandsFree feature */
#ifdef MTK_BT_HFP_DUAL_HF
#define MTK_HFP_DUAL_HF             TRUE
#endif

/* Wide Band Speech */
#ifndef MTK_HFP_WBS
#define MTK_HFP_WBS                 TRUE
#endif

/* In-band Ringtone */
#ifndef MTK_HFP_INBAND_RING
#define MTK_HFP_INBAND_RING         FALSE
#endif

/* Use eSCO S4 */
#ifndef MTK_HFP_eSCO_S4
#define MTK_HFP_eSCO_S4             FALSE
#endif

#if (MTK_HFP_WBS == TRUE)
#ifndef BTM_WBS_INCLUDED
#define BTM_WBS_INCLUDED            TRUE
#endif

#ifndef BTIF_HF_WBS_PREFERRED
#define BTIF_HF_WBS_PREFERRED       TRUE
#endif
#endif /* MTK_HFP_WBS == TRUE */

/* **************************************************************************
 * **
 * ** dial-up networking (DUN)
 * **
 * *************************************************************************/
#ifdef MTK_BT_DUN_GW_12
#define MTK_DUN_GW_12               TRUE
#endif

/* **************************************************************************
 * **
 * ** New MTK Vendor Opcodes
 * **
 * *************************************************************************/
/* Enable MTK-owned vendor opcode. */
/* Must be TRUE */
#define MTK_VENDOR_OPCODE           TRUE

/* **************************************************************************
 * **
 * ** MTK Power Saving Mode Ctrl by HCI Interface
 * **
 * *************************************************************************/
/* Support PSM Mode in HCI Interface. */
#define MTK_HCI_POWERSAVING_MODE    TRUE

/* **************************************************************************
 * **
 * ** BT Snoop Log
 * **
 * *************************************************************************/
/* Create a thread for accessing file i/o of btsnoop log. */
#ifndef MTK_BTSNOOPLOG_THREAD
#if defined(MTK_LINUX)
#define MTK_BTSNOOPLOG_THREAD       TRUE
#else
#define MTK_BTSNOOPLOG_THREAD       FALSE
#endif
#endif

/* Default enable customize snnop log mode*/
#ifndef MTK_BTSNOOPLOG_MODE_SUPPORT
#if defined(MTK_LINUX)
#define MTK_BTSNOOPLOG_MODE_SUPPORT TRUE
#else
#define MTK_BTSNOOPLOG_MODE_SUPPORT FALSE
#endif
#endif

/* **************************************************************************
 * **
 * ** ATT/GATT Protocol/Profile Settings
 * **
 * *************************************************************************/
#ifndef BTA_DM_GATT_CLOSE_DELAY_TOUT
#define BTA_DM_GATT_CLOSE_DELAY_TOUT    1500
#endif

/* Force handle ble data length extension packets which packet size is bigger than GATT_MTU(23B). */
/* This is a temp solution for BLE RC voice search. */
/* The final official solution should be : GATT_MTU set by APP layer. */
#ifndef MTK_FORCE_HANDLE_BLE_DATA_LENGTH_EXTENSION_PACKET
#define MTK_FORCE_HANDLE_BLE_DATA_LENGTH_EXTENSION_PACKET FALSE
#endif

/* **************************************************************************
 * **
 * ** HID/HOGP Profile Settings
 * **
 * *************************************************************************/
#ifndef MTK_HID_IOT_MOUSE_IME
#define MTK_HID_IOT_MOUSE_IME           TRUE
#endif

#if defined(MTK_COMMON) && (MTK_COMMON == TRUE)
#define HANDLE_KEY_MISSING TRUE
#endif

#ifdef MTK_BT_KERNEL_3_18
#define MTK_HID_DRIVER_KERNEL_3_18      TRUE
#endif

#ifdef MTK_BT_KERNEL_4_4
#define MTK_HID_DRIVER_KERNEL_4_4       TRUE
#endif

#ifndef MTK_LINUX_HID
#if defined(MTK_LINUX)
#define MTK_LINUX_HID   TRUE
#else
#define MTK_LINUX_HID   FALSE
#endif
#endif /* MTK_LINUX_HID */

#if MTK_LINUX_HID == TRUE
#ifndef MTK_LINUX_HID_SET_UHID_NAME
#define MTK_LINUX_HID_SET_UHID_NAME  FALSE
#endif

#endif /* MTK_LINUX_HID == TRUE */

#ifndef MTK_LINUX_GATT
#if defined(MTK_LINUX)
#define MTK_LINUX_GATT   TRUE
#else
#define MTK_LINUX_GATT   FALSE
#endif
#endif

#if MTK_LINUX_GATT == TRUE
#ifndef MTK_LINUX_GATTC_LE_NAME
#define MTK_LINUX_GATTC_LE_NAME  TRUE
#endif

#ifndef MTK_LINUX_GATTC_RPA
#define MTK_LINUX_GATTC_RPA      TRUE
#endif

#ifndef MTK_LINUX_GATTC_DISC_MODE
#define MTK_LINUX_GATTC_DISC_MODE   TRUE
#endif


#ifndef MTK_LINUX_GATTC_PTS_TEST
#define MTK_LINUX_GATTC_PTS_TEST  FALSE
#endif
#endif

#if MTK_LINUX_GATTC_LE_NAME == TRUE
#ifndef MTK_LINUX_GATT_MAX_LE_NAME_LEN
#define MTK_LINUX_GATT_MAX_LE_NAME_LEN     31
#endif
#endif

#ifndef PIN_KEY_MISSING_HANDLE_UNPAIR
#define PIN_KEY_MISSING_HANDLE_UNPAIR TRUE
#endif


#endif /* MTK_BLUEDROID_PATCH */

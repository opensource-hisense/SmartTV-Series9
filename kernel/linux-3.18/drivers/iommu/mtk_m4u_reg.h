/*
 * Copyright (c) 2014-2015 MediaTek Inc.
 * Author: Yong Wu <yong.wu at mediatek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#ifndef MTK_M4U_REG_H
#define MTK_M4U_REG_H 1

#define MMU_NRS 2
#define MMU_RS_INFO_NRS 16
#define M4U_MAX_CLIENTS 2

#define REG_MMU_PT_BASE_ADDR			0x000
    #define PAGE_TBL_BASE_ADDR_MSK (~0x3ffUL)
    #define PAGE_TBL_BASE_ADDR_PA32_33_MSK (0x3UL)

#define REG_MMU_INVLDT		 0x20
    #define F_MMU_INV_ALL	BIT(1)
    #define F_MMU_INV_RANGE	BIT(0)
 
#define REG_MMU_INVLDT_SA		 0x24
#define REG_MMU_INVLDT_EA        0x28

#define REG_MMU_INVLDT_SEL 0x38
    #define F_MMU_INV_EN_L1	 BIT(0)
    #define F_MMU_INV_EN_L2	 BIT(1)
    #define F_MMU_INV_VA32	 BIT(2)
 
#define REG_MMU_DUMMY   0x44
    #define F_MMU_CLEAR_MODE_OLD BIT(5)//use new invalidate range(compare tag and index) or old(only compare tag)
    
#define REG_MMU_MISC_CTRL 0x48
#define REG_MMU_DCM_DIS 0x50
#define REG_MMU_WR_LEN_CTRL 0x54

#define REG_MMU_READ_ENTRY 0x100
    #define MMU_PTLB_BLOCK_OFFSET_BIT BIT(2)
    #define MMU_PTLB_READ_ENTRY_IDX BIT(5)
    #define MMU_VICT_TLB_SEL_BIT BIT(13)
    #define MMU0_MTLB_ENTRY_IDX_BIT BIT(14)
    #define MMU_PTLB_READ_SEL BIT(26)
    #define MMU0_MTLB_READ_SEL BIT(27)
    #define MMU_TLB_ENTRY_READ_ENABLE BIT(31)

#define REG_MMU_DESC_RDATA 0x104 
#define REG_MMU_L2_TAG_RDATA 0x108
    #define L2TLB_TAG_VIRTUAL_ADDR_MSK (0x3ff0)
    #define L2TLB_TAG_VIRTUAL_ADDR_SHIFT (18)
    #define L2TLB_TAG_LAYER (1UL<<3)
    #define L2TLB_TAG_VIRTUAL_ADDR32_BIT (1UL<<2)
    #define L2TLB_TAG_SECURE_BIT (1UL<<1)
    #define VTLB_TAG_SET_NUM_BIT_MSK (0x7fUL<<20)
    #define VTLB_TAG_SET_NUM_SHIFT_BIT (20)
    
#define REG_MMU_VICT_TAG_VALID 0x10C

#define REG_MMU_CTRL_REG  0x110
    #define F_MMU_PFH_DIS BIT(0)
    #define F_MMU_PERF_MON_EN BIT(1)
    #define F_MMU_PERF_MON_CLR BIT(2)
    
#define REG_MMU_VLD_PA_RNG 0x118 //valid physical range
#define REG_MMU_INT_CONTROL0 0x120
    #define F_L2_MULIT_HIT_FAULT			BIT(0)
    #define F_L2_TABLE_WALK_FAULT_INT_FAULT		BIT(1)
    #define F_PREETCH_FIFO_OVERFLOW_INT_FAULT	BIT(2)
    #define F_MISS_FIFO_OVERFLOW_INT_FAULT	BIT(3)
    #define F_L2_INVDT_DONE_INT_FAULT		BIT(4)  
    #define F_INT_CLR_BIT				BIT(12)      

#define REG_MMU_INT_CONTROL1 0x124
    #define F_INT_TRANSLATION_FAULT     	BIT(0)
    #define F_INT_MAIN_MULTI_HIT_FAULT		BIT(1)
    #define F_INT_INVALID_PA_FAULT			BIT(2)
    #define F_INT_ENTRY_REPLACEMENT_FAULT		BIT(3)
    #define F_INT_TLB_MISS_FAULT			BIT(4)
    #define F_INT_MISS_TRANSATION_FIFO_FAULT	BIT(5)
    #define F_INT_PRETETCH_TRANSATION_FIFO_FAULT	BIT(6)
    #define F_MM2_INT_MAU0_ASSERT_EN BIT(7)
    #define F_MM2_INT_MAU1_ASSERT_EN BIT(8)
    #define F_MM2_INT_MAU2_ASSERT_EN BIT(9)
    #define F_MM2_INT_MAU3_ASSERT_EN BIT(10)
    #define F_INT_MAU0_ASSERT_EN BIT(14)
    #define F_INT_MAU1_ASSERT_EN BIT(15)
    #define F_INT_MAU2_ASSERT_EN BIT(16)
    #define F_INT_MAU3_ASSERT_EN BIT(17)

#define REG_MMU_CPE_DONE			0x12C
    #define F_RNG_INVLDT_DONE BIT(0)

#define REG_MMU_L2_FAULT_STA         0x130

#define REG_MMU_MAIN_FAULT_STA       0x134

#define REG_MMU_L2_TABLE_FAULT_STA      0x138
    #define F_MMU_L2_TBWALK_FAULT_VA_MSK   ~(0xfffUL)
    #define F_MMU_L2_TBWALK_FAULT_TBL_ID_MSK   (0x3fUL<<2)
    #define F_MMU_L2_TBWALK_FAULT_VA_32_MSK   (1<<1)
    #define F_MMU_L2_TBWALK_FAULT_LAYER_MSK   (1<<0)

#define REG_MMU0_FAULT_STA      0x13C
    #define F_MMU_MTLB_FAULT_VA_MSK ~(0xfffUL)
    #define F_MMU_MTLB_FAULT_VA_32_MSK (1UL<<8)
    #define F_MMU_MTLB_FAULT_INVLD_PA_33_32_MSK (3U<<6)
    #define F_MMU_MTLB_FAULT_INVLD_PA_33_32_SFT (6)
    #define F_MMU_MTLB_FAULT_SECURE (1UL<<2)
    #define F_MMU_MTLB_FAULT_IS_WRITE (1UL<<1)
    #define F_MMU_MTLB_FAULT_LAYER_MSK   (1<<0)
#define REG_MMU0_INVLD_PA    0x140
#define REG_MMU0_INT_ID      0x150
    #define F_MMU_TF_ERR_ID_MSK 0x3ff
    #define F_MMU_GID_MSK 0x03
    #define F_MMU_PORT_ID_SHIFT 0x2
    #define F_MMU_PORT_ID_MSK (0x1f<<F_MMU_PORT_ID_SHIFT)
    #define F_MMU_LARB_ID_SHIFT 0x7
    #define F_MMU_LARB_ID_MSK (0x7<<F_MMU_LARB_ID_SHIFT)        
   
#define REG_MMU1_FAULT_STA      0x144
#define REG_MMU1_INVLD_PA    0x148
#define REG_MMU1_INT_ID      0x154
//perf
#define REG_MMU_PF_LAYER1_MSCNT 0x1A0
#define REG_MMU_PF_LAYER2_MSCNT 0x1A4
#define REG_MMU_PF_LAYER1_CNT 0x1A8
#define REG_MMU_PF_LAYER2_CNT 0x1AC
#define REG_MMU0_ACC_CNT 0x1C0
#define REG_MMU0_MAIN_LAYER1_MSCNT 0x1C4
#define REG_MMU0_MAIN_LAYER2_MSCNT 0x1C8
#define REG_MMU0_RS_PERF_CNT       0x1CC 
#define REG_MMU0_LOOKUP_CNT       0x1D0
#define MMU_PERF_REG_OFFSET 0x20
struct m4u_perf_info
{
    u32 l2_layer1_miss;
    u32 l2_layer2_miss;
    u32 l2_layer1_pfh;//prefetch count
    u32 l2_layer2_pfh;
    u32 lookup_cnt;//total translation request count
    u32 m_layer1_miss;
    u32 m_layer2_miss;
    u32 rs_state;//stay in rs state count
    u32 total_cnt;//total transaction count
};

#define REG_MMU_L2_TAG_VLD0_WAY0 0x200//way0 set0~31 l2 tag valid bit

#define MAU_SET_OFFSET 0x24
#define MMU_SET_MAU_OFFSET 0x100
#define MTLB_ENT_NUM 64
#define L2TLB_WAYS 4
#define L2TLB_SETS 128
#define L2TLB_DESC_NUM 8
#define VICT_TLB_ENT_NUM 4
#define VICT_TLB_DESC_NUM 8
#define MMU_SET_MTLB_TAG_OFFSET 0x300

#define REG_MMU0_RS0_VA 0x380
#define REG_MMU0_RS0_PA 0x384
#define REG_MMU0_2ND_BASE 0x388
#define REG_MMU0_RS0_STA 0x38C
#define MMU_RS_INFO_OFFSET 0x300

#define REG_MMU0_MAIN_TAG0 0x500
#define MTLB_TAG_VIRTUAL_ADDR_MSK (~0xfffU)
#define MTLB_TAG_LOCK_BIT (1UL<<11)
#define MTLB_TAG_VALID_BIT (1UL<<10)
#define MTLB_TAG_LAYER (1UL<<9)
#define MTLB_TAG_16X_SIZE (1UL<<8)
#define MTLB_TAG_SEC_BIT (1UL<<7)
#define MTLB_TAG_INV_DESC_BIT (1UL<<6)
#define MTLB_TAG_VIRTUAL_ADDR32_BIT (1UL<<0)

#define REG_MMU0_MAU0_ASRT_ID 0x918
#define REG_MMU0_MAU0_AA     0x91C
#define REG_MMU0_MAU0_AA_EXT     0x920
#define REG_MMU0_MAU_CLR     0x990 

#define REG_MMU0_MAU_ASRT_STA 0x9A0
    #define F_MAU0_ASSERT BIT(0)
    #define F_MAU1_ASSERT BIT(1)
    #define F_MAU2_ASSERT BIT(2)
    #define F_MAU3_ASSERT BIT(3)

#define REG_MMU0_TRF_MON_EN 0xE00
    #define F_MMU_TRF_MON_EN BIT(0)

#define REG_MMU0_TRF_MON_CLR 0xE04
    #define F_MMU_TRF_MON_CLR BIT(0)
    
#define REG_MMU0_TRF_MON_ID 0xE08
    #define F_MMU_TRF_MON_ID_MSK 0xffffUL
    #define F_MMU_TRF_MON_ALL_ID BIT(16)
    #define F_MMU_TRF_MON_ID_SEL BIT(17)
    #define F_MMU_TRF_MON_TBL BIT(18)
#define REG_MMU0_TRF_MON_CTL 0xE0C
    #define F_MMU_TRF_MON_RW BIT(3)
    #define F_MMU_TRF_MON_DP_MODE BIT(2)
    #define F_MMU_TRF_MON_REQ_SEL_MSK 0x03UL
#define REG_MMU0_TRF_MON_ACC_CNT 0xE10
#define REG_MMU0_TRF_MON_REQ_CNT 0xE14
#define REG_MMU0_TRF_MON_BEAT_CNT 0xE18
#define REG_MMU0_TRF_MON_BYTE_CNT 0xE1C
#define REG_MMU0_TRF_MON_DATA_CNT 0xE20
#define REG_MMU0_TRF_MON_CP_CNT 0xE24
#define REG_MMU0_TRF_MON_DP_CNT 0xE28
#define REG_MMU0_TRF_MON_CP_MAX 0xE2C
#define REG_MMU0_TRF_MON_OSTD_CNT 0xE30
#define REG_MMU0_TRF_MON_OSTD_MAX 0xE34
#define MMU_TRF_MON_REG_OFFSET 0x40 

   
#endif

/*
 * Copyright (c) 2014-2017 MediaTek Inc.
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

#define MAX_CHAN_NUM  5
#define I2C_TIMEOUT (HZ<<1) // 2000ms

#define Fld(wid,shft) (((u32)wid<<8)|shft)
#define Fld_wid(fld) (u8)(fld>>8)
#define Fld_shft(fld) (u8)(fld)
#define Fld_mask(fld) Fld_wid(fld)==32 ? 0xffffffff : \
				   ((((u32)1<<Fld_wid(fld))-1)<<Fld_shft(fld))

static inline void mtk_set_bit(void __iomem *reg, u32 mask)
{
	writel(readl(reg) | mask, reg);
}

static inline void mtk_clr_bit(void __iomem *reg, u32 mask)
{
	writel(readl(reg) & ~mask, reg);
}

static inline u32 mtk_read_func(u32 *reg, u32 mask, u8 shft)
{
	return ((readl(reg) & mask) >> shft);
}

static inline void mtk_write_func(u32 *reg, u32 mask, u8 shft, u32 val)
{
	writel(((readl(reg) & ~mask ) | ((val << shft)& mask)), reg);
}

#define mtk_read_fld(reg,fld) \
	mtk_read_func(reg,Fld_mask(fld),Fld_shft(fld))

#define mtk_write_fld(reg,fld,val) \
	mtk_write_func(reg,Fld_mask(fld),Fld_shft(fld),val)

#define PFld(fld,val) (((sizeof(upk))>1) ? Fld_mask(fld) \
	: (((u32)val & ((1<< Fld_wid(fld)) -1)) << Fld_shft(fld)))

#define mtk_write_pfld(reg,list) \
{ \
	u32 upk; \
	u32 mask = (u32) list; \
	{ u8 upk; \
		mtk_write_func(reg, mask, 0, list); \
	} \
}


#define I2C_CTL0_SCL_STRECH            Fld(1,0)
#define I2C_CTL0_EN                    Fld(1,1)
#define I2C_CTL0_SDA_STATE             Fld(1,2)
#define I2C_CTL0_SCL_STATE             Fld(1,3)
#define I2C_CTL0_CS_STATUS             Fld(1,4)
#define I2C_CTL0_DEG_EN                Fld(1,5)
#define I2C_CTL0_WAIT_LEVEL            Fld(1,6)
#define I2C_CTL0_DEG_CNT               Fld(8,8)
#define I2C_CTL0_CLK_DIV               Fld(12,16)
#define I2C_CTL0_VSYNC_MODE            Fld(2,28)
#define I2C_CTL0_ODRAIN                Fld(1,31)
#define I2C_CTL1_TRI                   Fld(1,0)
#define I2C_CTL1_MODE                  Fld(3,4)
#define I2C_CTL1_PGLEN                 Fld(6,8)
#define I2C_CTL1_ACK0                  Fld(16,16)
#define I2C_CTL2_SCL_MODE              Fld(1,0)
#define I2C_CTL2_SCL_RISE_VALUE        Fld(12,16)
#define I2C_CTL3_ACT_MODE              Fld(7,0)
#define I2C_CTL3_SDEV_MODE             Fld(1,0)
#define I2C_CTL3_STR_RDNACK_STOP_MODE  Fld(1,1)
#define I2C_CTL3_STR_RDACK_MODE        Fld(1,2)
#define I2C_CTL3_RDNACK_STOP_MODE      Fld(1,3)
#define I2C_CTL3_STR_WD_STOP_MODE      Fld(1,4)
#define I2C_CTL3_STR_WD_MODE           Fld(1,5)
#define I2C_CTL3_WD_STOP_MODE          Fld(1,6)
#define I2C_CTL3_ACK1                  Fld(20,11)
#define I2C_CTL3_SACK                  Fld(1,31)
#define I2C_SDEV_ADDR                  Fld(8,0)
#define I2C_SM0_INT                    Fld(1,0)

#define MODE_START      (0x01)
#define MODE_WRITE      (0x02)
#define MODE_STOP       (0x03)
#define MODE_READ_NACK  (0x04)
#define MODE_READ       (0x05)

#define I2C_OK                (0)
#define I2C_ERROR_NODEV       (-1)       // I2C device doesn't ack to SIF master
#define I2C_ERROR_SUBADDR     (-2)       // There is error when I2C master transmits subaddress data
#define I2C_ERROR_DATA        (-3)       // There is error when I2C master transmits/receives data
#define I2C_ERROR_BUS_BUSY    (-4)       // I2C bus is busy, can't access
#define I2C_ERROR_PARAM       (-5)       // Function parameter is invalid
#define I2C_ERROR_TIMEOUT     (-6)       // I2C device timeout

struct mt53xx_i2c_regs {
	u32 ctl0;
	u32 ctl1;
	u32 ctl2;
	u32 ctl3;
	u32 sdev;
	u32 data0;
	u32 data1;
	u32 data2;
	u32 data3;
	u32 data4;
	u32 data5;
	u32 data6;
	u32 data7;
	u32 data8;
	u32 intren;
	u32 intren_fld;
	u32 intrst;
	u32 intrst_fld;
	u32 intrcl;
	u32 intrcl_fld;
	u32 hwrst;
	u32 hwrst_fld;
};
const struct mt53xx_i2c_regs mt53xx_i2c_regs[MAX_CHAN_NUM] =
{
	{       // i2c0
		.ctl0 = 0x240,
		.ctl1 = 0x244,
		.ctl2 = 0x248,
		.ctl3 = 0x2FC,
		.sdev = 0x24C,
		.data0 = 0x250,
		.data1 = 0x254,
		.data2 = 0x2E0,
		.data3 = 0x2E4,
		.data4 = 0x2E8,
		.data5 = 0x2EC,
		.data6 = 0x2F0,
		.data7 = 0x2F4,
		.data8 = 0x2F8,
		.intren = 0x25C,
		.intren_fld = Fld(1,0),
		.intrst = 0x260,
		.intrst_fld = Fld(1,0),
		.intrcl = 0x264,
		.intrcl_fld = Fld(1,0),
		.hwrst = 0x000,
		.hwrst_fld = Fld(1,28),
	},
	{        // i2c1
		.ctl0 = 0x270,
		.ctl1 = 0x274,
		.ctl2 = 0x278,
		.ctl3 = 0x2A8,
		.sdev = 0x27C,
		.data0 = 0x280,
		.data1 = 0x284,
		.data2 = 0x288,
		.data3 = 0x28C,
		.data4 = 0x290,
		.data5 = 0x294,
		.data6 = 0x298,
		.data7 = 0x29C,
		.data8 = 0x2A4,
		.intren = 0x25C,
		.intren_fld = Fld(1,1),
		.intrst = 0x260,
		.intrst_fld = Fld(1,1),
		.intrcl = 0x264,
		.intrcl_fld = Fld(1,1),
		.hwrst = 0x000,
		.hwrst_fld = Fld(1,27),
	},
	{       // i2c2
		.ctl0 = 0x200,
		.ctl1 = 0x204,
		.ctl2 = 0x208,
		.ctl3 = 0x238,
		.sdev = 0x20C,
		.data0 = 0x210,
		.data1 = 0x214,
		.data2 = 0x218,
		.data3 = 0x21C,
		.data4 = 0x220,
		.data5 = 0x224,
		.data6 = 0x228,
		.data7 = 0x22C,
		.data8 = 0x234,
		.intren = 0x25C,
		.intren_fld = Fld(1,2),
		.intrst = 0x260,
		.intrst_fld = Fld(1,2),
		.intrcl = 0x264,
		.intrcl_fld = Fld(1,2),
		.hwrst = 0x000,
		.hwrst_fld = Fld(1,26),
	},
	{       // i2c3
		.ctl0 = 0x1C0,
		.ctl1 = 0x1C4,
		.ctl2 = 0xC00,
		.ctl3 = 0xC04,
		.sdev = 0xC24,
		.data0 = 0x1C8,
		.data1 = 0x1CC,
		.data2 = 0xC08,
		.data3 = 0xC0C,
		.data4 = 0xC10,
		.data5 = 0xC14,
		.data6 = 0xC18,
		.data7 = 0xC1C,
		.data8 = 0xC20,
		.intren = 0x44,
		.intren_fld = Fld(1,8),
		.intrst = 0x40,
		.intrst_fld = Fld(1,8),
		.intrcl = 0x48,
		.intrcl_fld = Fld(1,8),
		.hwrst = 0xC24, // no i2c hardware reset in pdwnc, use sdev last bit.
		.hwrst_fld = Fld(1,31),
	},
	{       // i2c4
		.ctl0 = 0x160,
		.ctl1 = 0x164,
		.ctl2 = 0xC28,
		.ctl3 = 0xC2C,
		.sdev = 0xC4C,
		.data0 = 0x168,
		.data1 = 0x16C,
		.data2 = 0xC30,
		.data3 = 0xC34,
		.data4 = 0xC38,
		.data5 = 0xC3C,
		.data6 = 0xC40,
		.data7 = 0xC44,
		.data8 = 0xC48,
		.intren = 0x44,
		.intren_fld = Fld(1,20),
		.intrst = 0x40,
		.intrst_fld = Fld(1,20),
		.intrcl = 0x48,
		.intrcl_fld = Fld(1,20),
		.hwrst = 0xC24, // no i2c hardware reset in pdwnc, use sdev last bit.
		.hwrst_fld = Fld(1,31),
	},
};

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
#define pr_fmt(fmt)    "[mtk_i2c]: " fmt
#include <linux/device.h>
#include <linux/i2c.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/printk.h>
#include <linux/delay.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include "i2c-mt53xx.h"

struct mtk_i2c_data
{
	void *base;//base address;
	struct device *dev;
	const struct mt53xx_i2c_regs *regs;
	struct i2c_adapter *adap;
	struct completion done;
	int irq;
};

static int mtk_i2c_hw_init(struct mtk_i2c_data *data);

static irqreturn_t mtk_i2c_isr(int irq, void *dev_id)
{
	struct mtk_i2c_data *data = dev_id;
	const struct mt53xx_i2c_regs *regs;

	regs = data->regs;
	if(mtk_read_fld(data->base+regs->intrst,regs->intrst_fld) == 1)
	{
		mtk_write_fld(data->base+regs->intrcl,regs->intrcl_fld,1);
		complete(&data->done);
		return IRQ_HANDLED;
	}
	return IRQ_NONE;
}

#ifdef CONFIG_PM
static int mtk_i2c_suspend(struct device *dev)
{
	return 0;
}

static int mtk_i2c_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct mtk_i2c_data *data = platform_get_drvdata(pdev);
	mtk_i2c_hw_init(data);
	return 0;
}
#else
#define mtk_i2c_suspend NULL
#define mtk_i2c_resume NULL
#endif

static u32 mtk_i2c_func(struct i2c_adapter *adap)
{
	return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL;
}

static void mtk_i2c_adap_init(struct i2c_adapter *adap)
{
	struct mtk_i2c_data *data = i2c_get_adapdata(adap);
	const struct mt53xx_i2c_regs *regs = data->regs;

	mtk_write_fld(data->base+regs->intrcl,regs->intrcl_fld,1);
	mtk_write_fld(data->base+regs->intren,regs->intren_fld,1);

	mtk_write_fld(data->base+regs->ctl0,I2C_CTL0_ODRAIN,0);
	mtk_write_fld(data->base+regs->ctl0,I2C_CTL0_SCL_STRECH,0);
	mtk_write_fld(data->base+regs->ctl0,I2C_CTL0_DEG_EN,0x1);
	mtk_write_fld(data->base+regs->ctl0,I2C_CTL0_DEG_CNT,0x3);
	mtk_write_fld(data->base+regs->ctl0,I2C_CTL0_EN, 0x1);

	//pr_info("init: adapter%d ctl0 (%p) val 0x%x\n",
	//	adap->nr,data->base+regs->ctl0,readl(data->base+regs->ctl0));
}

static int mtk_i2c_reset(struct i2c_adapter *adap)
{
	struct mtk_i2c_data *data = i2c_get_adapdata(adap);
	const struct mt53xx_i2c_regs *regs = data->regs;

	mtk_write_fld(data->base+regs->intren,regs->intren_fld,0);
	mtk_write_fld(data->base+regs->intrcl,regs->intrcl_fld,1);
	mtk_write_fld(data->base+regs->hwrst,regs->hwrst_fld,1);
	mtk_write_fld(data->base+regs->hwrst,regs->hwrst_fld,0);
	mtk_write_fld(data->base+regs->ctl0, I2C_CTL0_EN, 0x1);
	udelay(100);
	mtk_i2c_adap_init(adap);

	return 0;
}

static int mtk_i2c_wait_hw(struct i2c_adapter *adap)
{
	ulong time_left;
	struct mtk_i2c_data *data = i2c_get_adapdata(adap);
	const struct mt53xx_i2c_regs *regs = data->regs;
	reinit_completion(&data->done);
	time_left = wait_for_completion_timeout(&data->done,I2C_TIMEOUT);
	if(!time_left)
	{
		pr_info("adap%d timeout\n",adap->nr);
		pr_info("adap%d ctl0(%p)=0x%x\n",adap->nr,data->base+regs->ctl0,readl(data->base+regs->ctl0));
		pr_info("adap%d ctl1(%p)=0x%x\n",adap->nr,data->base+regs->ctl1,readl(data->base+regs->ctl1));
		pr_info("adap%d ctl2(%p)=0x%x\n",adap->nr,data->base+regs->ctl2,readl(data->base+regs->ctl2));
		pr_info("adap%d ctl3(%p)=0x%x\n",adap->nr,data->base+regs->ctl3,readl(data->base+regs->ctl3));
		pr_info("adap%d sdev(%p)=0x%x\n",adap->nr,data->base+regs->sdev,readl(data->base+regs->sdev));
		pr_info("adap%d intren(%p)=0x%x\n",adap->nr,data->base+regs->intren,readl(data->base+regs->intren));
		pr_info("adap%d intrst(%p)=0x%x\n",adap->nr,data->base+regs->intrst,readl(data->base+regs->intrst));
		pr_info("adap%d intrcl(%p)=0x%x\n",adap->nr,data->base+regs->intrcl,readl(data->base+regs->intrcl));
		mtk_i2c_reset(adap);
		return I2C_ERROR_TIMEOUT;
	}
	return I2C_OK;
}
// check i2c bus states
static int mtk_i2c_bus_state(struct i2c_adapter *adap)
{
	u8 scl=0,sda=0;
	struct mtk_i2c_data *data = i2c_get_adapdata(adap);
	const struct mt53xx_i2c_regs *regs = data->regs;
	scl = (u8)(mtk_read_fld(data->base+regs->ctl0, I2C_CTL0_SCL_STATE));
	sda = (u8)(mtk_read_fld(data->base+regs->ctl0, I2C_CTL0_SDA_STATE));
	return (scl << 1) | sda ;
}
// put start bit
static int mtk_i2c_put_start(struct i2c_adapter *adap)
{
	u32 ret=0;
	struct mtk_i2c_data *data = i2c_get_adapdata(adap);
	const struct mt53xx_i2c_regs *regs = data->regs;
	mtk_write_fld(data->base+regs->ctl3,I2C_CTL3_ACT_MODE,0x00);
	mtk_write_fld(data->base+regs->ctl1, I2C_CTL1_MODE, MODE_START);
	mtk_write_fld(data->base+regs->ctl1, I2C_CTL1_TRI, 0x1);
	ret = mtk_i2c_wait_hw(adap);
	return ret;
}

// put stop bit
static int mtk_i2c_put_stop(struct i2c_adapter *adap)
{
	u32 ret=0;
	struct mtk_i2c_data *data = i2c_get_adapdata(adap);
	const struct mt53xx_i2c_regs *regs = data->regs;
	mtk_write_fld(data->base+regs->ctl3, I2C_CTL3_ACT_MODE, 0x0);
	mtk_write_fld(data->base+regs->ctl1, I2C_CTL1_MODE, MODE_STOP);
	mtk_write_fld(data->base+regs->ctl1, I2C_CTL1_TRI, 0x1);
	ret = mtk_i2c_wait_hw(adap);
	return ret;
}

// check ack
static int mtk_i2c_check_ack(u32 ackh, u32 ackl)
{
	int i=0;
	u32 cnt=0,ack = (ackh << 16) | ackl;
	if(ackh >= 0xffff)
	{
		if(ackl != 0xffff)
		{
			cnt = 0;
		}
		else
		{
			for(i=4;i>=0;i--)
			{
				if(ackh == ((1<<(i+16)) - 1))
				{
					cnt = i + 32;
				}
			}
		}
	}
	else
	{
		for(i=31;i>0;i--)
		{
			if(ack == ((1<<i) -1))
			{
				cnt = i;
			}
		}
	}
	return cnt;
}
// put bytes
// mode BIT(0), 1:send start, 0: no start.
// mode BIT(1), 1:send stop,  0: no stop.
static int mtk_i2c_put_bytes(struct i2c_adapter *adap, u8 *buf, u32 len, u8 mode)
{
	u8 *pbuf=buf;
	int i=0,j=0;
	u8 sack;
	u32 ackh=0,ackl=0;
	int ack_cnt=0,cnt=0,ret=0;
	int total_loop=0,split_bytes=0;
	u32 d0,d1,d2,d3,d4,d5,d6,d7,d8;
	struct mtk_i2c_data *data = i2c_get_adapdata(adap);
	const struct mt53xx_i2c_regs *regs = data->regs;

	total_loop = (len + 35) / 36;
	for(i=0;i<total_loop;i++)
	{
		if(i == (total_loop - 1))
		{
			if((mode & BIT(1)) == 0)
			{
				mtk_write_fld(data->base+regs->ctl3,I2C_CTL3_ACT_MODE,0x00);
				mtk_write_fld(data->base+regs->ctl1, I2C_CTL1_MODE, MODE_READ);
			}
			if((mode & BIT(1)) == 2)
			{
				mtk_write_fld(data->base+regs->ctl3,I2C_CTL3_ACT_MODE,0x40);
				mtk_write_fld(data->base+regs->ctl1, I2C_CTL1_MODE, 0);
			}
			split_bytes = len % 36;
		}
		else
		{
			if(i == 0)
			{
				if((mode & BIT(0)) == 0)
				{
					mtk_write_fld(data->base+regs->ctl3,I2C_CTL3_ACT_MODE,0x00);
					mtk_write_fld(data->base+regs->ctl1, I2C_CTL1_MODE, MODE_WRITE);
				}
				if((mode & BIT(0)) == 1)
				{
					mtk_write_fld(data->base+regs->ctl3,I2C_CTL3_ACT_MODE,0x21);
					mtk_write_fld(data->base+regs->ctl1, I2C_CTL1_MODE, 0);
				}
			}
			else
			{
				mtk_write_fld(data->base+regs->ctl3,I2C_CTL3_ACT_MODE,0x00);
				mtk_write_fld(data->base+regs->ctl1, I2C_CTL1_MODE, MODE_WRITE);
			}
			split_bytes = 36;
		}
		if(split_bytes == 0)
		{
			split_bytes = 36;
		}
		if(total_loop == 1)
		{
			if(mode == 0)
			{
				mtk_write_fld(data->base+regs->ctl3,I2C_CTL3_ACT_MODE,0x00);
				mtk_write_fld(data->base+regs->ctl1, I2C_CTL1_MODE, MODE_WRITE);
			}
			if(mode == 1)
			{
				mtk_write_fld(data->base+regs->ctl3,I2C_CTL3_ACT_MODE,0x21);
				mtk_write_fld(data->base+regs->ctl1, I2C_CTL1_MODE, 0);
			}
			if(mode == 2)
			{
				mtk_write_fld(data->base+regs->ctl3,I2C_CTL3_ACT_MODE,0x40);
				mtk_write_fld(data->base+regs->ctl1, I2C_CTL1_MODE, 0);
			}
			if(mode == 3)
			{
				mtk_write_fld(data->base+regs->ctl3,I2C_CTL3_ACT_MODE,0x11);
				mtk_write_fld(data->base+regs->ctl1, I2C_CTL1_MODE, 0);
			}
		}

		d0 = 0;
		d1 = 0;
		d2 = 0;
		d3 = 0;
		d4 = 0;
		d5 = 0;
		d6 = 0;
		d7 = 0;
		d8 = 0;

		for(j=0; j< ((split_bytes > 4)? 4: split_bytes); j++)
		{
			d0 |= ((pbuf[j] << (8*j)) & (0xff << (8*j)));
		}

		if(split_bytes > 4)
		{
			for(j=0; j< ((split_bytes > 8)? 4: split_bytes-4); j++)
			{
				d1 |= ((pbuf[4+j] << (8*j)) & (0xff << (8*j)));
			}
		}

		if(split_bytes > 8)
		{
			for(j=0; j< ((split_bytes > 12)? 4: split_bytes-8); j++)
			{
				d2 |= ((pbuf[8+j] << (8*j)) & (0xff << (8*j)));
			}
		}

		if(split_bytes > 12)
		{
			for(j=0; j< ((split_bytes > 16)? 4: split_bytes-12); j++)
			{
				d3 |= ((pbuf[12+j] << (8*j)) & (0xff << (8*j)));
			}
		}

		if(split_bytes > 16)
		{
			for(j=0; j< ((split_bytes > 20)? 4: split_bytes-16); j++)
			{
				d4 |= ((pbuf[16+j] << (8*j)) & (0xff << (8*j)));
			}
		}

		if(split_bytes > 20)
		{
			for(j=0; j< ((split_bytes > 24)? 4: split_bytes-20); j++)
			{
				d5 |= ((pbuf[20+j] << (8*j)) & (0xff << (8*j)));
			}
		}

		if(split_bytes > 24)
		{
			for(j=0; j< ((split_bytes > 28)? 4: split_bytes-24); j++)
			{
				d6 |= ((pbuf[24+j] << (8*j)) & (0xff << (8*j)));
			}
		}

		if(split_bytes > 28)
		{
			for(j=0; j< ((split_bytes > 32)? 4: split_bytes-28); j++)
			{
				d7 |= ((pbuf[28+j] << (8*j)) & (0xff << (8*j)));
			}
		}

		if(split_bytes > 32)
		{
			for(j=0; j< (split_bytes-32); j++)
			{
				d8 |= ((pbuf[32+j] << (8*j)) & (0xff << (8*j)));
			}
		}
		pbuf += split_bytes;

		writel(d0,data->base+regs->data0);
		writel(d1,data->base+regs->data1);
		writel(d2,data->base+regs->data2);
		writel(d3,data->base+regs->data3);
		writel(d4,data->base+regs->data4);
		writel(d5,data->base+regs->data5);
		writel(d6,data->base+regs->data6);
		writel(d7,data->base+regs->data7);
		writel(d8,data->base+regs->data8);

		mtk_write_fld(data->base+regs->ctl1, I2C_CTL1_PGLEN, split_bytes -1);
		mtk_write_fld(data->base+regs->ctl1, I2C_CTL1_TRI, 0x1);
		ret = mtk_i2c_wait_hw(adap);
		if(ret != I2C_OK)
		{
			return ret;
		}

		if(i==0 && (mode & BIT(0)) ==1)
		{
			sack = mtk_read_fld(data->base+regs->ctl3,I2C_CTL3_SACK);
			if(sack == 0)
			{
				return I2C_ERROR_NODEV;
			}
		}
		ackl = mtk_read_fld(data->base+regs->ctl1,I2C_CTL1_ACK0);
		ackh = mtk_read_fld(data->base+regs->ctl3,I2C_CTL3_ACK1);
		ack_cnt = mtk_i2c_check_ack(ackh,ackl);
		if(ack_cnt!=split_bytes)
		{
			pr_info("abnormal device ack sequence 0x%5x%4x.\n",ackh,ackl);
			ret = mtk_i2c_put_stop(adap);
			if(ret != I2C_OK)
			{
				pr_info("write:put stop bit error\n");
			}
			return I2C_ERROR_DATA;
		}
		cnt += ack_cnt;
	}
	if(cnt != len)
	{
		return I2C_ERROR_DATA;
	}
	return cnt;
}
// get bytes
// mode BIT(0), 1:send start, 0: no start.
// mode BIT(1), 1:send stop,  0: no stop.
static int mtk_i2c_get_bytes(struct i2c_adapter *adap, u8 *buf, u32 len, u8 mode)
{
	u8 *pbuf=buf;
	u32 total_loop=0,split_bytes=0;
	int cnt=0,ret=0,i=0,j=0;
	u8 sack;
	u32 d0,d1,d2,d3,d4,d5,d6,d7,d8;
	struct mtk_i2c_data *data = i2c_get_adapdata(adap);
	const struct mt53xx_i2c_regs *regs = data->regs;

	total_loop = (len + 35) / 36;
	for(i=0;i<total_loop;i++)
	{
		if(i == (total_loop -1))
		{
			if((mode & BIT(1)) == 0)
			{
				mtk_write_fld(data->base+regs->ctl3,I2C_CTL3_ACT_MODE,0x00);
				mtk_write_fld(data->base+regs->ctl1, I2C_CTL1_MODE, MODE_READ);
			}
			if((mode & BIT(1)) == 2)
			{
				mtk_write_fld(data->base+regs->ctl3,I2C_CTL3_ACT_MODE,0x08);
				mtk_write_fld(data->base+regs->ctl1, I2C_CTL1_MODE, 0);
			}
			split_bytes = len % 36;
		}
		else
		{
			if(i == 0)
			{
				if((mode & BIT(0)) == 0)
				{
					mtk_write_fld(data->base+regs->ctl3,I2C_CTL3_ACT_MODE,0x00);
					mtk_write_fld(data->base+regs->ctl1, I2C_CTL1_MODE, MODE_READ);
				}
				if((mode & BIT(0)) == 1)
				{
					mtk_write_fld(data->base+regs->ctl3,I2C_CTL3_ACT_MODE,0x05);
					mtk_write_fld(data->base+regs->ctl1, I2C_CTL1_MODE, 0);
				}
			}
			else
			{
				mtk_write_fld(data->base+regs->ctl3,I2C_CTL3_ACT_MODE,0x00);
				mtk_write_fld(data->base+regs->ctl1, I2C_CTL1_MODE, MODE_READ);
			}
			split_bytes = 36;
		}
		if(split_bytes == 0)
		{
			split_bytes = 36;
		}

		if(total_loop == 1)
		{
			if(mode == 0)
			{
				mtk_write_fld(data->base+regs->ctl3,I2C_CTL3_ACT_MODE,0x00);
				mtk_write_fld(data->base+regs->ctl1, I2C_CTL1_MODE, MODE_READ);
			}
			if(mode == 1)
			{
				mtk_write_fld(data->base+regs->ctl3,I2C_CTL3_ACT_MODE,0x05);
				mtk_write_fld(data->base+regs->ctl1, I2C_CTL1_MODE, 0);
			}
			if(mode == 2)
			{
				mtk_write_fld(data->base+regs->ctl3,I2C_CTL3_ACT_MODE,0x08);
				mtk_write_fld(data->base+regs->ctl1, I2C_CTL1_MODE, 0);
			}
			if(mode == 3)
			{
				mtk_write_fld(data->base+regs->ctl3,I2C_CTL3_ACT_MODE,0x03);
				mtk_write_fld(data->base+regs->ctl1, I2C_CTL1_MODE, 0);
			}
		}

		mtk_write_fld(data->base+regs->ctl1, I2C_CTL1_PGLEN, split_bytes-1);
		mtk_write_fld(data->base+regs->ctl1, I2C_CTL1_TRI, 0x1);
		ret = mtk_i2c_wait_hw(adap);
		if(ret != I2C_OK)
		{
			return ret;
		}
          
		if(i==0 && (mode & BIT(0)) ==1)
		{
			sack = mtk_read_fld(data->base+regs->ctl3,I2C_CTL3_SACK);
			if(sack == 0)
			{
				return I2C_ERROR_NODEV;
			}
		}

		d0 = readl(data->base+regs->data0);
		d1 = readl(data->base+regs->data1);
		d2 = readl(data->base+regs->data2);
		d3 = readl(data->base+regs->data3);
		d4 = readl(data->base+regs->data4);
		d5 = readl(data->base+regs->data5);
		d6 = readl(data->base+regs->data6);
		d7 = readl(data->base+regs->data7);
		d8 = readl(data->base+regs->data8);

		for(j=0;j<((split_bytes > 4) ? 4 : split_bytes); j++)
		{
			*pbuf = (u8) (d0 & 0xff);
			d0 >>=8;
			pbuf++;
			cnt++;
		}

		if(split_bytes > 4)
		{
			for(j=0;j<((split_bytes > 8) ? 4 : split_bytes-4); j++)
			{
				*pbuf = (u8) (d1 & 0xff);
				d1 >>=8;
				pbuf++;
				cnt++;
			}
		}

		if(split_bytes > 8)
		{
			for(j=0;j<((split_bytes > 12) ? 4 : split_bytes-8); j++)
			{
				*pbuf = (u8) (d2 & 0xff);
				d2 >>=8;
				pbuf++;
				cnt++;
			}
		}

		if(split_bytes > 12)
		{
			for(j=0;j<((split_bytes > 16) ? 4 : split_bytes-12); j++)
			{
				*pbuf = (u8) (d3 & 0xff);
				d3 >>=8;
				pbuf++;
				cnt++;
			}
		}

		if(split_bytes > 16)
		{
			for(j=0;j<((split_bytes > 20) ? 4 : split_bytes-16); j++)
			{
				*pbuf = (u8) (d4 & 0xff);
				d4 >>=8;
				pbuf++;
				cnt++;
			}
		}

		if(split_bytes > 20)
		{
			for(j=0;j<((split_bytes > 24) ? 4 : split_bytes-20); j++)
			{
				*pbuf = (u8) (d5 & 0xff);
				d5 >>=8;
				pbuf++;
				cnt++;
			}
		}

		if(split_bytes > 24)
		{
			for(j=0;j<((split_bytes > 28) ? 4 : split_bytes - 24); j++)
			{
				*pbuf = (u8) (d6 & 0xff);
				d6 >>=8;
				pbuf++;
				cnt++;
			}
		}

		if(split_bytes > 28)
		{
			for(j=0;j<((split_bytes > 32) ? 4 : split_bytes - 28); j++)
			{
				*pbuf = (u8) (d7 & 0xff);
				d7 >>=8;
				pbuf++;
				cnt++;
			}
		}

		if(split_bytes > 32)
		{
			for(j=0;j<(split_bytes - 32); j++)
			{
				*pbuf = (u8) (d8 & 0xff);
				d8 >>=8;
				pbuf++;
				cnt++;
			}
		}
	}
	if(cnt != len)
	{
		return I2C_ERROR_DATA;
	}
	return cnt;
}

// do action
static int mtk_i2c_do_action(struct i2c_adapter *adap, u8 addr, u32 sub_addr,
		u8 sub_addr_num, u8 isread, u8 *buf, u32 len)
{
	struct mtk_i2c_data *data = i2c_get_adapdata(adap);
	const struct mt53xx_i2c_regs *regs = data->regs;
	int ret=0,cnt=0;
	u8 subaddr[3]={0};
	// check bus is busy
	if(mtk_i2c_bus_state(adap)!=0x3)
	{
		ret = mtk_i2c_put_stop(adap);
		if(ret != I2C_OK)
		{
			pr_info("checkbus:put stop bit error\n");
		}
		if(mtk_i2c_bus_state(adap)!=0x3)
		{
			//pr_info("checkbus:bus is too busy to action\n");
			return I2C_ERROR_BUS_BUSY;
		}
	}

	if(sub_addr_num) // sub address
	{
		if(sub_addr_num == 3)
		{
			subaddr[0] = (u8)((sub_addr & 0xff0000) >> 16);
			subaddr[1] = (u8)((sub_addr & 0x00ff00) >> 8);
			subaddr[2] = (u8)((sub_addr & 0x0000ff));
		}
		else if(sub_addr_num == 2)
		{
			subaddr[0] = (u8)((sub_addr & 0x00ff00) >> 8);
			subaddr[1] = (u8)((sub_addr & 0x0000ff));
		}
		else if(sub_addr_num == 1)
		{
			subaddr[0] = (u8)((sub_addr & 0x0000ff));
		}
		else
		{
			return I2C_ERROR_SUBADDR;
		}

		mtk_write_fld(data->base+regs->ctl3,I2C_CTL3_ACT_MODE,0x21);
		mtk_write_fld(data->base+regs->sdev,I2C_SDEV_ADDR,addr);
		cnt = mtk_i2c_put_bytes(adap,(u8*)(&subaddr),sub_addr_num,0x1);
		if(cnt != (u32)sub_addr_num)
		{
			ret = mtk_i2c_put_stop(adap);
			if(ret != I2C_OK)
			{
				pr_info("subaddr:put stop bit error\n");
			}
			if(cnt == I2C_ERROR_NODEV)
			{
				return I2C_ERROR_NODEV;
			}
			else
			{
				return I2C_ERROR_SUBADDR;
			}
		}

		if(isread)
		{
			mtk_write_fld(data->base+regs->sdev,I2C_SDEV_ADDR,addr|1);
			cnt = mtk_i2c_get_bytes(adap,buf,len,0x3);
		}
		else
		{
			mtk_write_fld(data->base+regs->sdev,I2C_SDEV_ADDR,addr);
			cnt = mtk_i2c_put_bytes(adap,buf,len,0x2);
		}
	}

	else
	{
		if(isread)
		{
			mtk_write_fld(data->base+regs->sdev,I2C_SDEV_ADDR,addr|1);
			cnt = mtk_i2c_get_bytes(adap,buf,len,0x3);
		}
		else
		{
			mtk_write_fld(data->base+regs->sdev,I2C_SDEV_ADDR,addr);
			cnt = mtk_i2c_put_bytes(adap,buf,len,0x3);
		}
	}
	mtk_write_fld(data->base+regs->ctl3,I2C_CTL3_ACT_MODE,0x00);

	if(mtk_i2c_bus_state(adap)!=0x3)
	{
		ret = mtk_i2c_put_stop(adap);
		if(ret != I2C_OK)
		{
			pr_info("%s:put stop bit error\n",__FUNCTION__);
		}
		if(mtk_i2c_bus_state(adap)!=0x3)
		{
			pr_info("%s:bus is still busy\n",__FUNCTION__);
			mtk_i2c_reset(adap);
		}
	}
	return cnt;
}

static int mtk_i2c_xfer(struct i2c_adapter *adap, struct i2c_msg msgs[],
		int num)
{
	struct mtk_i2c_data *data = i2c_get_adapdata(adap);
	const struct mt53xx_i2c_regs *regs = data->regs;
	void *base=data->base;
	u16 clkdiv;
	u32 riseval;
	u8  sub_addr_num;
	u32 sub_addr;
	u32 dev_addr;
	u8 *buf1,*buf2;
	u32 msg_idx=0;
	u16 len;
	u8 isread;
	u32 ret=0;

	if(num<2)
	{
		pr_err("message format is wrong\n");
		return -EINVAL;
	}
	//pr_info("channel %d xfer\n",adap->nr);

	buf1=msgs[0].buf;
	if(!buf1)
		return -EINVAL;
	if(msgs[0].len!=(sizeof(clkdiv)+sizeof(sub_addr_num)+sizeof(sub_addr)))
		return -EINVAL;

	memcpy(&clkdiv,buf1,sizeof(clkdiv));
	buf1+=sizeof(clkdiv);
	memcpy(&sub_addr_num,buf1,sizeof(sub_addr_num));
	buf1+=sizeof(sub_addr_num);
	memcpy(&sub_addr,buf1,sizeof(sub_addr));
	msg_idx++;
	num-=1;

	//pr_info("clkdiv %x addr_num %x,addr %x\n",clkdiv,sub_addr_num,sub_addr);

	// ensure interrupt is enabled
	mtk_write_fld(data->base+regs->intrcl,regs->intrcl_fld,1);
	mtk_write_fld(data->base+regs->intren,regs->intren_fld,1);

	// set clkdiv
	if(clkdiv < 0x20)
	{
		//pr_info("xfer: clkdiv < 0x20 , limit it to 0x20\n");
		clkdiv = 0x20;
		riseval = 0x20 * 3 / 5;
		mtk_write_fld(base+regs->ctl2,I2C_CTL2_SCL_RISE_VALUE,riseval);
		mtk_write_fld(base+regs->ctl2,I2C_CTL2_SCL_MODE,0x1);
	}
	else if(clkdiv >= 0x20 && clkdiv < 0x80)
	{
		riseval = clkdiv * 3 / 5;
		//pr_info("xfer: clkdiv < 0x80 , use fast mode\n");
		mtk_write_fld(base+regs->ctl2,I2C_CTL2_SCL_RISE_VALUE,riseval);
		mtk_write_fld(base+regs->ctl2,I2C_CTL2_SCL_MODE,0x1);
	}
	else
	{
		//pr_info("xfer: clkdiv >= 0x80, use standard mode\n");
		mtk_write_fld(base+regs->ctl2,I2C_CTL2_SCL_MODE,0);
	}
	mtk_write_fld(base+regs->ctl0,I2C_CTL0_CLK_DIV,clkdiv);

	while(num)
	{
		dev_addr=msgs[msg_idx].addr;
		buf2=msgs[msg_idx].buf;
		len=msgs[msg_idx].len;
		isread=(msgs[msg_idx].flags&I2C_M_RD)?1:0;
		//pr_info("action:%s, count=%d\n",isread?"read":"write",len);
		ret=mtk_i2c_do_action(adap,dev_addr,sub_addr,
				sub_addr_num,isread,buf2,len);
		msg_idx++;
		num--;
	}
	return ret;
}
static const struct i2c_algorithm mtk_i2c_algo = {
	.master_xfer	= mtk_i2c_xfer,
	.functionality	= mtk_i2c_func,
};

static int mtk_i2c_hw_init(struct mtk_i2c_data *data)
{
	const struct mt53xx_i2c_regs *regs = data->regs;

	mtk_write_fld(data->base+regs->intrcl,regs->intrcl_fld,1);
	mtk_write_fld(data->base+regs->intren,regs->intren_fld,0);

	mtk_write_fld(data->base+regs->ctl0,I2C_CTL0_ODRAIN,0);
	mtk_write_fld(data->base+regs->ctl0,I2C_CTL0_SCL_STRECH,0);
	mtk_write_fld(data->base+regs->ctl0,I2C_CTL0_DEG_EN,0x1);
	mtk_write_fld(data->base+regs->ctl0,I2C_CTL0_DEG_CNT,0x3);
	mtk_write_fld(data->base+regs->ctl0,I2C_CTL0_EN, 0x1);

	mtk_write_fld(data->base+regs->intrcl,regs->intrcl_fld,1);
	mtk_write_fld(data->base+regs->intren,regs->intren_fld,1);

	init_completion(&data->done);

	return 0;
}
static int mtk_i2c_probe(struct platform_device *pdev)
{
	struct mtk_i2c_data *data;
	u32 chan_nr;
	int ret;
	struct i2c_adapter *adap;
	struct device * dev=&pdev->dev;
	int irq;
	unsigned long irq_flags;

	ret = of_property_read_u32_array(pdev->dev.of_node, "chan",
			&chan_nr,1);
	if(ret < 0)
	{
		dev_warn(&pdev->dev,"Could not read chan_nr property\n");
		chan_nr=0;
	}
	else
	{
		pr_info("chan_nr=%d\n",chan_nr);
	}
	data = devm_kzalloc(dev, sizeof(*data), GFP_KERNEL);
	if(!data)
		return -ENOMEM;

	data->dev=dev;
	adap=devm_kzalloc(dev, sizeof(struct i2c_adapter), GFP_KERNEL);
	if(!adap)
	{
		devm_kfree(dev,data->adap);
		devm_kfree(dev,data);
		return -ENOMEM;
	}
	init_completion(&data->done);
	adap->owner = THIS_MODULE;
	adap->class = I2C_CLASS_DEPRECATED;
	snprintf(adap->name,sizeof(adap->name),"mtk_i2c%d",chan_nr);
	adap->algo = &mtk_i2c_algo;
	adap->dev.parent = &pdev->dev;
	adap->dev.of_node = pdev->dev.of_node;
	adap->nr=chan_nr;
	pr_info("prepare to add i2c adapter%d %p\n",adap->nr,adap);
	data->regs = &(mt53xx_i2c_regs[chan_nr]);
	//pr_info("i2c adapter%d ctl0 reg offset 0x%x\n",adap->nr,data->regs->ctl0);

	ret = i2c_add_numbered_adapter(adap);
	if(ret<0)
	{
		dev_warn(&pdev->dev,"Could not add i2c adapter\n");
		devm_kfree(dev,data->adap);

		devm_kfree(dev,data);
		return ret;
	}
	i2c_set_adapdata(adap, data);

	data->base=of_iomap(dev->of_node,0);

	if(IS_ERR(data->base))
	{
		pr_err("data->base is error(%ld)\n",PTR_ERR(data->base));
		ret=PTR_ERR(data->base);
		devm_kfree(dev,data->adap);
		devm_kfree(dev,data);
		return ret;
	}

	pr_info("base address=%p\n",data->base);
	platform_set_drvdata(pdev, data);

	ret=mtk_i2c_hw_init(data);
	if(ret<0)
	{
		devm_kfree(dev,data->adap);
		devm_kfree(dev,data);
		return ret;
	}
	irq=platform_get_irq(pdev,0);

	if(irq > 0)
	{
		irq_flags = IRQF_SHARED;
#ifdef CONFIG_ARM_GIC
		irq_flags |= IRQF_TRIGGER_HIGH;
#endif
		data->irq = irq;
		if(devm_request_irq(&pdev->dev,data->irq, mtk_i2c_isr,
					irq_flags,
					adap->name, data) < 0)
		{
			data->irq = 0;
			dev_warn(&pdev->dev, "interrupt not available.\n");
		}
	}
	return 0;
}
static int mtk_i2c_remove(struct platform_device *pdev)
{
	struct mtk_i2c_data *data;
	struct device * dev=&pdev->dev;

	data=platform_get_drvdata(pdev);
	pr_info("remove i2c adapter%d\n",data->adap->nr);
	i2c_del_adapter(data->adap);
	devm_kfree(dev,data);
	return 0;
}
const struct dev_pm_ops mtk_i2c_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(mtk_i2c_suspend, mtk_i2c_resume)
};
static const struct of_device_id mtk_i2c_of_ids[] = {
	{ .compatible = "Mediatek,I2C", },
	{}
};
static struct platform_driver mtk_i2c_driver = {
	.probe	= mtk_i2c_probe,
	.remove	= mtk_i2c_remove,
	.driver	= {
		.name = "mtk-i2c",
		.of_match_table = mtk_i2c_of_ids,
		.pm = &mtk_i2c_pm_ops,
	}
};

static int __init mtk_i2c_init(void)
{
	return platform_driver_register(&mtk_i2c_driver);
}
subsys_initcall(mtk_i2c_init);

static void __exit mtk_i2c_exit(void)
{
	platform_driver_unregister(&mtk_i2c_driver);
}

module_exit(mtk_i2c_exit);

MODULE_AUTHOR("Mediatek Inc");
MODULE_DESCRIPTION("Mediatek DTV I2C Driver");
MODULE_LICENSE("GPL v2");

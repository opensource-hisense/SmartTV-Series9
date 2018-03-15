/*
 * Copyright (c) 2016 MediaTek Inc.
 * Author: Chen Zhong <chen.zhong@mediatek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/of_regulator.h>
#include <linux/slab.h>
#include <linux/i2c.h>

#define MT53xx_ID_VCPU 0
#define MT53xx_ID_VGPU 1
#define MT53xx_MAX_REGULATOR 2

/*
 * LOG
 */
#define mt53xxreg_error(fmt, args...)     pr_err("[MT53xx-regulator]: " fmt, ##args)
#if 0
#define mt53xxreg_warning(fmt, args...)   pr_warn("[MT53xx-regulator]: " fmt, ##args)
#define mt53xxreg_notice(fmt, args...)    pr_info("[MT53xx-regulator]: " fmt, ##args)
#define mt53xxreg_info(fmt, args...)      pr_info("[MT53xx-regulator]: " fmt, ##args)
#define mt53xxreg_debug(fmt, args...)     pr_debug("[MT53xx-regulator]: " fmt, ##args) 
#else
#define mt53xxreg_warning(fmt, args...)   
#define mt53xxreg_notice(fmt, args...)    
#define mt53xxreg_info(fmt, args...)      
#define mt53xxreg_debug(fmt, args...)     
#endif

#if 0
#define I2C_VCPU_DEVADDR 0xC6 //RT5758
#define I2C_VCPU_DATADDR 0x02 //RT5758
#define I2C_VCPU_MODADDR 0x03 //RT5758
#define I2C_VCPU_MODMASK 0X04 //RT5758
#define I2C_VCPU_MODPWM_EN 1 //RT5758
#define I2C_VCPU_VMIN 600 //unit: mV
#define I2C_VCPU_VMAX 1500 //unit: mV
#define I2C_VCPU_STEP 1000 //unit: 10uV

#define I2C_VGPU_DEVADDR 0xC8 //RT5090 Buck3
#define I2C_VGPU_DATADDR 0x05 //RT5090 Buck3
#define I2C_VGPU_MODADDR 0X06 //RT5090 Buck3
#define I2C_VGPU_MODMASK 0X10 //RT5090 Buck3
#define I2C_VGPU_MODPWM_EN 1 //RT5090 Buck3
#define I2C_VGPU_VMIN 700 //unit: mV
#define I2C_VGPU_VMAX 1500 //unit: mV
#define I2C_VGPU_STEP 1000 //unit: 10uV
#define I2C_CHANNEL_ID 4 //pdwnc
#else
struct mt_regulator_info {
	char devaddr;
	char dataddr;
	char modaddr;
	char modmask;
	char modepwm_en;
	unsigned int vmin;
	unsigned int vmax;
	unsigned int vstep;
	unsigned int channel_id;
};

struct mt_regulator_info mtreg_info[MT53xx_MAX_REGULATOR] = {
	[MT53xx_ID_VCPU] = {
		.devaddr = 0xC6,
		.dataddr = 0x02,
		.modaddr = 0x03,
		.modmask = 0X04,
		.modepwm_en = 1,
		.vmin = 600,
		.vmax = 1500,
		.vstep = 1000,
	},
	[MT53xx_ID_VGPU] = {
		.devaddr = 0xC8,
		.dataddr = 0x05,
		.modaddr = 0X06,
		.modmask = 0X10,
		.modepwm_en = 1,
		.vmin = 700,
		.vmax = 1500,
		.vstep = 1000,
	},
};
EXPORT_SYMBOL(mtreg_info);

#define I2C_VCPU_DEVADDR mtreg_info[MT53xx_ID_VCPU].devaddr
#define I2C_VCPU_DATADDR mtreg_info[MT53xx_ID_VCPU].dataddr
#define I2C_VCPU_MODADDR mtreg_info[MT53xx_ID_VCPU].modaddr
#define I2C_VCPU_MODMASK mtreg_info[MT53xx_ID_VCPU].modmask
#define I2C_VCPU_MODPWM_EN mtreg_info[MT53xx_ID_VCPU].modepwm_en
#define I2C_VCPU_VMIN mtreg_info[MT53xx_ID_VCPU].vmin
#define I2C_VCPU_VMAX mtreg_info[MT53xx_ID_VCPU].vmax
#define I2C_VCPU_STEP mtreg_info[MT53xx_ID_VCPU].vstep
#define I2C_CHANNEL_ID mtreg_info[MT53xx_ID_VCPU].channel_id

#define I2C_VGPU_DEVADDR mtreg_info[MT53xx_ID_VGPU].devaddr
#define I2C_VGPU_DATADDR mtreg_info[MT53xx_ID_VGPU].dataddr
#define I2C_VGPU_MODADDR mtreg_info[MT53xx_ID_VGPU].modaddr
#define I2C_VGPU_MODMASK mtreg_info[MT53xx_ID_VGPU].modmask
#define I2C_VGPU_MODPWM_EN mtreg_info[MT53xx_ID_VGPU].modepwm_en
#define I2C_VGPU_VMIN mtreg_info[MT53xx_ID_VGPU].vmin
#define I2C_VGPU_VMAX mtreg_info[MT53xx_ID_VGPU].vmax
#define I2C_VGPU_STEP mtreg_info[MT53xx_ID_VGPU].vstep
#endif
#define SIF_CLK_DIV 0x100

#ifdef CC_AOS_PMIC
#define STEP_OFFSET 5
#else
#define STEP_OFFSET 0
#endif
/*
 * MT53xx regulators' information
 *
 * @desc: standard fields of regulator description.
 * @qi: Mask for query enable signal status of regulators
 * @vselon_reg: Register sections for hardware control mode of bucks
 * @vselctrl_reg: Register for controlling the buck control mode.
 * @vselctrl_mask: Mask for query buck's voltage control mode.
 */
struct mt53xx_regulator_info {
	struct regulator_desc desc;
	u32 I2C_addr;
};

#define MT53xx_BUCK(match, vreg, min, max, step, volt_ranges, _I2C_addr)				\
[MT53xx_ID_##vreg] = {							\
	.desc = {							\
		.name = #vreg,						\
		.of_match = of_match_ptr(match),			\
		.ops = &mt53xx_volt_range_ops,				\
		.type = REGULATOR_VOLTAGE,				\
		.id = MT53xx_ID_##vreg,					\
		.owner = THIS_MODULE,					\
		.n_voltages = (max - min)/step + 1,			\
		.linear_ranges = volt_ranges,				\
		.n_linear_ranges = ARRAY_SIZE(volt_ranges),		\
	},								\
	.I2C_addr = _I2C_addr,					\
}


#if 0
static struct regulator_linear_range buck_volt_range1[] = {
	REGULATOR_LINEAR_RANGE(I2C_VCPU_VMIN*1000, 0, ((I2C_VCPU_VMAX-I2C_VCPU_VMIN)*100/I2C_VCPU_STEP),
		I2C_VCPU_STEP*10),
};
static struct regulator_linear_range buck_volt_range2[] = {
	REGULATOR_LINEAR_RANGE(I2C_VGPU_VMIN*1000, 0, ((I2C_VGPU_VMAX-I2C_VGPU_VMIN)*100/I2C_VGPU_STEP),
		I2C_VGPU_STEP*10),
};
#else
static struct regulator_linear_range buck_volt_range1[] = {
	REGULATOR_LINEAR_RANGE(600000, 0, (90000/1000),	10000),
};
static struct regulator_linear_range buck_volt_range2[] = {
	REGULATOR_LINEAR_RANGE(700000, 0, (80000/1000),	10000),
};
#endif

#define WAIT_I2C_READY (1)
#ifdef CC_AOS_PMIC
static int _I2C_Xfer_Write_Cmd_before_read(u8 device_addr, u8 addr, u8 data)
{
#if WAIT_I2C_READY
    u8 u1Dev;
    u16 u2ClkDiv;
    u32 u4Addr;
    u32 u4Count;
    u32 u4IsRead;
//    int i;
    int i4RetVal;
//    u8* pu1Buf;
//    u8* pu2Buf;
    u8 u1AddrNum;
    u8 u1ChannelId;
    u8 buf1[sizeof(u2ClkDiv)+sizeof(u1AddrNum)+sizeof(u4Addr)];
    struct i2c_msg xfer_msg[2];
    struct i2c_adapter *i2c_adapt;

    u1ChannelId=I2C_CHANNEL_ID;
    u2ClkDiv = SIF_CLK_DIV;
    u1Dev = device_addr;
    u1AddrNum = 0;
    u4Addr = addr;
    u4Count = 1;
    u4IsRead = 0;

    memcpy(buf1,&u2ClkDiv,sizeof(u2ClkDiv));//2B clock div
    buf1[sizeof(u2ClkDiv)]=u1AddrNum;//addr num
    memcpy(buf1+sizeof(u2ClkDiv)+sizeof(u1AddrNum),&u4Addr,sizeof(u4Addr));//4B addr

//    pu2Buf=pu1Buf = (UINT8*)x_mem_alloc(u4Count);

//    if (pu1Buf == NULL)
//    {
//        return -1;
//    }
	
    xfer_msg[0].addr=u1Dev;
    xfer_msg[0].len=sizeof(buf1);
    xfer_msg[0].buf=buf1;
    xfer_msg[0].flags=u4IsRead;
    xfer_msg[1].addr=u1Dev;
    xfer_msg[1].len=u4Count;
    xfer_msg[1].buf=&data;
    xfer_msg[1].flags=u4IsRead;

    i2c_adapt=i2c_get_adapter(u1ChannelId);

    if(!i2c_adapt)
    {
        mt53xxreg_error("can not get adapter for channel %d\n ",u1ChannelId);
    }
    else
    {
        i4RetVal = i2c_transfer(i2c_adapt,xfer_msg,sizeof(xfer_msg)/sizeof(xfer_msg[0]));
        if(i4RetVal<0)
        {
            mt53xxreg_error("i2c transfer fails\n");
        }
        i2c_put_adapter(i2c_adapt);
        if(i4RetVal > 0)
        {
            mt53xxreg_info("write successfully! byte count = %d\n",i4RetVal);
        }
        else
        {
            mt53xxreg_info("write failed! return value = %d\n",i4RetVal);
        }
    }
//    x_mem_free(pu2Buf);
    return 0;
#else
	return 0;
#endif
}
#endif

static int _I2C_Xfer_Read_Cmd(u8 device_addr, u8 addr)
{
#if WAIT_I2C_READY
    u8 u1Dev;
    u16 u2ClkDiv;
    u32 u4Addr;
    u32 u4Count;
    u32 u4IsRead;
    int i;
    int i4RetVal;
//    u8* pu1Buf;
//    u8* pu2Buf;
    u8 u1AddrNum;
    u8 u1ChannelId;
    u8 buf1[sizeof(u2ClkDiv)+sizeof(u1AddrNum)+sizeof(u4Addr)];
    struct i2c_msg xfer_msg[2];
    struct i2c_adapter *i2c_adapt;

#ifdef CC_AOS_PMIC
	unsigned char data[2];
    u1AddrNum = 0;
    u4Addr = 0;
    u4Count = 2;
    u4IsRead = 1;
    i4RetVal = _I2C_Xfer_Write_Cmd_before_read(device_addr, 0, 0);
#else
	unsigned char data;
    u1AddrNum = 1;
    u4Addr = addr;
    u4Count = 1;
    u4IsRead = 1;
#endif

    u1ChannelId=I2C_CHANNEL_ID;
    u2ClkDiv = SIF_CLK_DIV;
    u1Dev = device_addr;

    memcpy(buf1,&u2ClkDiv,sizeof(u2ClkDiv));//2B clock div
    buf1[sizeof(u2ClkDiv)]=u1AddrNum;//addr num
    memcpy(buf1+sizeof(u2ClkDiv)+sizeof(u1AddrNum),&u4Addr,sizeof(u4Addr));//4B addr
//    pu2Buf=pu1Buf = (u8*)kmalloc(u4Count, GFP_KERNEL);

//    if (pu1Buf == NULL)
//    {
//        return -1;
//    }
//    memset((void*)pu1Buf, 0, u4Count);

    xfer_msg[0].addr=device_addr;
    xfer_msg[0].len=sizeof(buf1);
    xfer_msg[0].buf=buf1;
    xfer_msg[0].flags=u4IsRead;
    xfer_msg[1].addr=u1Dev;
    xfer_msg[1].len=u4Count;
    xfer_msg[1].buf=&data;
    xfer_msg[1].flags=u4IsRead;

    i2c_adapt=i2c_get_adapter(u1ChannelId);

    if(!i2c_adapt)
    {
        mt53xxreg_error("can not get adapter for channel %d\n ",u1ChannelId);
		i4RetVal=-1;
    }
    else
    {
        i4RetVal = i2c_transfer(i2c_adapt,xfer_msg,sizeof(xfer_msg)/sizeof(xfer_msg[0]));
        if(i4RetVal<0)
            mt53xxreg_error("i2c transfer fails\n");
		else
#ifdef CC_AOS_PMIC
            if(addr == 0)
			 i4RetVal = data[0] & 0x7F;
            else
             i4RetVal = data[1];
#else
        i4RetVal = data;
#endif
        i2c_put_adapter(i2c_adapt);
    }
//    kfree(pu2Buf);
    return i4RetVal;
#else
	return 0;
#endif
}

static int _I2C_Xfer_Write_Cmd(u8 device_addr, u8 addr, u8 data)
{
#if WAIT_I2C_READY
    u8 u1Dev;
    u16 u2ClkDiv;
    u32 u4Addr;
    u32 u4Count;
    u32 u4IsRead;
//    int i;
    int i4RetVal;
//    u8* pu1Buf;
//    u8* pu2Buf;
    u8 u1AddrNum;
    u8 u1ChannelId;
    u8 buf1[sizeof(u2ClkDiv)+sizeof(u1AddrNum)+sizeof(u4Addr)];
    struct i2c_msg xfer_msg[2];
    struct i2c_adapter *i2c_adapt;

    u1ChannelId=I2C_CHANNEL_ID;
    u2ClkDiv = SIF_CLK_DIV;
    u1Dev = device_addr;
    u1AddrNum = 1;
    u4Addr = addr;
    u4Count = 1;
    u4IsRead = 0;
#ifdef CC_AOS_PMIC
    int parity = 0;
    u8 tmp;

    if(addr == 0)
    {
        tmp = data;
        while(tmp != 0) {
            parity ^= tmp;
            tmp >>= 1;
        }
        parity&=0x1;
        if(parity == 0)
            data |= 0x80;
    }
#endif

    memcpy(buf1,&u2ClkDiv,sizeof(u2ClkDiv));//2B clock div
    buf1[sizeof(u2ClkDiv)]=u1AddrNum;//addr num
    memcpy(buf1+sizeof(u2ClkDiv)+sizeof(u1AddrNum),&u4Addr,sizeof(u4Addr));//4B addr

//    pu2Buf=pu1Buf = (UINT8*)x_mem_alloc(u4Count);

//    if (pu1Buf == NULL)
//    {
//        return -1;
//    }

    xfer_msg[0].addr=u1Dev;
    xfer_msg[0].len=sizeof(buf1);
    xfer_msg[0].buf=buf1;
    xfer_msg[0].flags=u4IsRead;
    xfer_msg[1].addr=u1Dev;
    xfer_msg[1].len=u4Count;
    xfer_msg[1].buf=&data;
    xfer_msg[1].flags=u4IsRead;

    i2c_adapt=i2c_get_adapter(u1ChannelId);

    if(!i2c_adapt)
    {
        mt53xxreg_error("can not get adapter for channel %d\n ",u1ChannelId);
    }
    else
    {
        i4RetVal = i2c_transfer(i2c_adapt,xfer_msg,sizeof(xfer_msg)/sizeof(xfer_msg[0]));
        if(i4RetVal<0)
        {
            mt53xxreg_error("i2c transfer fails\n");
        }
        i2c_put_adapter(i2c_adapt);
        if(i4RetVal > 0)
        {
            mt53xxreg_info("write successfully! byte count = %d\n",i4RetVal);
        }
        else
        {
            mt53xxreg_info("write failed! return value = %d\n",i4RetVal);
        }
    }
//    x_mem_free(pu2Buf);
    return 0;
#else
	return 0;
#endif
}

static int mt53xx_regulator_set_voltage(struct regulator_dev *rdev,
				       unsigned voltage)
{
    int i4RetVal;
	u8 step;
	u32 VCPU_table[] = {I2C_VCPU_VMIN*1000, I2C_VCPU_VMAX*1000, I2C_VCPU_STEP*10+STEP_OFFSET,};
	u32 VGPU_table[] = {I2C_VGPU_VMIN*1000, I2C_VGPU_VMAX*1000, I2C_VGPU_STEP*10+STEP_OFFSET,};
	struct mt53xx_regulator_info *info = rdev_get_drvdata(rdev);
	if(info->I2C_addr == I2C_VCPU_DEVADDR)
	{
		step = (voltage-VCPU_table[0]+VCPU_table[2]-1)/VCPU_table[2];
		i4RetVal = _I2C_Xfer_Write_Cmd((u8) info->I2C_addr, I2C_VCPU_DATADDR, step);
	}
	else
	{
		step = (voltage-VGPU_table[0]+VGPU_table[2]-1)/VGPU_table[2];
		i4RetVal = _I2C_Xfer_Write_Cmd((u8) info->I2C_addr, I2C_VGPU_DATADDR, step);
	}
	mt53xxreg_info("%s addr=0x%x voltage step=0x%x\n", __func__,info->I2C_addr,step);
	return i4RetVal;
}
static int mt53xx_regulator_get_voltage(struct regulator_dev *rdev)
{
	struct mt53xx_regulator_info *info = rdev_get_drvdata(rdev);
	int voltage;
	u32 VCPU_table[] = {I2C_VCPU_VMIN*1000, I2C_VCPU_VMAX*1000, I2C_VCPU_STEP*10+STEP_OFFSET,};
	u32 VGPU_table[] = {I2C_VGPU_VMIN*1000, I2C_VGPU_VMAX*1000, I2C_VGPU_STEP*10+STEP_OFFSET,};
	if(info->I2C_addr == I2C_VCPU_DEVADDR)
	{
		voltage = _I2C_Xfer_Read_Cmd((u8) info->I2C_addr, I2C_VCPU_DATADDR);
		if(voltage > 0)
			voltage = voltage*VCPU_table[2]+VCPU_table[0];
	}
	else
	{
		voltage = _I2C_Xfer_Read_Cmd((u8) info->I2C_addr, I2C_VGPU_DATADDR);
		if(voltage > 0)
			voltage = voltage*VGPU_table[2]+VGPU_table[0];
	}
	mt53xxreg_info("%s addr=0x%x voltage=%d uV\n", __func__,info->I2C_addr,voltage);
	return voltage;
}

static int mt53xx_regulator_set_mode(struct regulator_dev *rdev,
				       unsigned int mode)
{

    int i4RetVal;
	u8 mode_data;
	struct mt53xx_regulator_info *info = rdev_get_drvdata(rdev);

	if(info->I2C_addr == I2C_VCPU_DEVADDR)
	{
		mode_data = _I2C_Xfer_Read_Cmd((u8) info->I2C_addr, I2C_VCPU_MODADDR);
		mode_data &= ~I2C_VCPU_MODMASK;
		switch (mode) {
			case REGULATOR_MODE_FAST:
				mode_data = (I2C_VCPU_MODPWM_EN == 1) ? 
					(mode_data | I2C_VCPU_MODMASK) : mode_data;
				break;
			case REGULATOR_MODE_NORMAL: 
				mode_data = (I2C_VCPU_MODPWM_EN == 1) ? 
					mode_data : (mode_data | I2C_VCPU_MODMASK);
				break;
			default:
				return 0;
			}
		i4RetVal = _I2C_Xfer_Write_Cmd((u8) info->I2C_addr, I2C_VCPU_MODADDR, mode_data);
	}
	else
	{
		mode_data = _I2C_Xfer_Read_Cmd((u8) info->I2C_addr, I2C_VGPU_MODADDR);
		mode_data &= ~I2C_VGPU_MODMASK;
		switch (mode) {
			case REGULATOR_MODE_FAST:
				mode_data = (I2C_VGPU_MODPWM_EN == 1) ? 
					(mode_data | I2C_VGPU_MODMASK) : mode_data;
				break;
			case REGULATOR_MODE_NORMAL: 
				mode_data = (I2C_VGPU_MODPWM_EN == 1) ? 
					mode_data : (mode_data | I2C_VGPU_MODMASK);
				break;
			default:
				return 0;
			}
		i4RetVal = _I2C_Xfer_Write_Cmd((u8) info->I2C_addr, I2C_VGPU_MODADDR, mode_data);
	}

	mt53xxreg_info("%s addr=0x%x voltage mode=0x%x\n", __func__,info->I2C_addr,mode_data);
	return 0;
}

 static unsigned int mt53xx_regulator_get_mode(struct regulator_dev *rdev)
{
	 return 0;
}

static struct regulator_ops mt53xx_volt_range_ops = {
//	.list_voltage = regulator_list_voltage_linear_range,
//	.map_voltage = regulator_map_voltage_linear_range,
	.set_voltage = mt53xx_regulator_set_voltage,
	.get_voltage = mt53xx_regulator_get_voltage,
	.set_mode = mt53xx_regulator_set_mode,
	.get_mode = mt53xx_regulator_get_mode,
};


/* The array is indexed by id(MT53xx_ID_XXX) */
 struct mt53xx_regulator_info mt53xx_regulators[] = {
	MT53xx_BUCK("buck_vcpu", VCPU, 600000, 1500000, 10000,
		buck_volt_range1, 0),
	MT53xx_BUCK("buck_vgpu", VGPU, 700000, 1500000, 10000,
		buck_volt_range2, 0),
};

static int mt53xx_regulator_probe(struct platform_device *pdev)
{
	struct regulator_config config = {};
	struct regulator_dev *rdev;
	struct regulation_constraints *c;
	int i;
	u32 reg_value;

 
	mt53xx_regulators[0].I2C_addr = I2C_VCPU_DEVADDR;
	mt53xx_regulators[1].I2C_addr = I2C_VGPU_DEVADDR;
 mt53xxreg_info("%s\n", __func__);
 for (i = 0; i < MT53xx_MAX_REGULATOR; i++) {
 		config.dev = &pdev->dev;
		config.driver_data = &mt53xx_regulators[i];
		rdev = devm_regulator_register(&pdev->dev,
				&mt53xx_regulators[i].desc, &config);
		if (IS_ERR(rdev)) {
			dev_err(&pdev->dev, "failed to register %s\n",
				mt53xx_regulators[i].desc.name);
			return PTR_ERR(rdev);
		}
		/* Constrain board-specific capabilities according to what
		 * this driver and the chip itself can actually do.
		 */
		c = rdev->constraints;
		c->valid_modes_mask |= REGULATOR_MODE_NORMAL|
			REGULATOR_MODE_STANDBY | REGULATOR_MODE_FAST;
		c->valid_ops_mask |= REGULATOR_CHANGE_MODE;
 	}

 	return 0;
}

static const struct platform_device_id mt53xx_platform_ids[] = {
	{"mt53xx-regulator", 0},
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(platform, mt53xx_platform_ids);

static const struct of_device_id mt53xx_of_match[] = {
	{ .compatible = "mediatek,mt53xx-regulator", },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, mt53xx_of_match);

static struct platform_driver mt53xx_regulator_driver = {
	.driver = {
		.name = "mt53xx-regulator",
		.of_match_table = of_match_ptr(mt53xx_of_match),
	},
	.probe = mt53xx_regulator_probe,
	.id_table = mt53xx_platform_ids,
};

module_platform_driver(mt53xx_regulator_driver);

MODULE_AUTHOR("Chen Zhong <chen.zhong@mediatek.com>");
MODULE_DESCRIPTION("Regulator Driver for MediaTek MT6392 PMIC");
MODULE_LICENSE("GPL v2");

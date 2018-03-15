#include "star.h"

#if defined(CONFIG_ARCH_MT5863)
CL45RegAddr const Dev1E_TX_I2MPB_A_1 = {0x1E, 0x0012};
  #define DA_TX_I2MPB_SHIFT_GBE_A    10
  #define DA_TX_I2MPB_SHIFT_TBT_A    0

  #define DA_TX_I2MPB_MASK_GBE_A   0xFC00
  #define DA_TX_I2MPB_MASK_TBT_A   0x003F

CL45RegAddr const Dev1E_TX_I2MPB_A_2 = {0x1E, 0x0016};
  #define DA_TX_I2MPB_SHIFT_HBT_A    10
  #define DA_TX_I2MPB_SHIFT_TST_A    0

  #define DA_TX_I2MPB_MASK_HBT_A   0xFC00
  #define DA_TX_I2MPB_MASK_TST_A   0x003F

CL45RegAddr const Dev1E_TX_I2MPB_B_1 = {0x1E, 0x0017};
  #define DA_TX_I2MPB_SHIFT_GBE_B    8
  #define DA_TX_I2MPB_SHIFT_TBT_B    0

  #define DA_TX_I2MPB_MASK_GBE_B   0x3F00
  #define DA_TX_I2MPB_MASK_TBT_B   0x003F

CL45RegAddr const Dev1E_TX_I2MPB_B_2 = {0x1E, 0x0018};
  #define DA_TX_I2MPB_SHIFT_HBT_B    8
  #define DA_TX_I2MPB_SHIFT_TST_B    0

  #define DA_TX_I2MPB_MASK_HBT_B   0x3F00
  #define DA_TX_I2MPB_MASK_TST_B   0x003F

CL45RegAddr const Dev1E_TX_OFFSET_0 = {0x1E, 0x0172};
  #define CR_TX_AMP_OFFSET_SHIFT_A     8
  #define CR_TX_AMP_OFFSET_SHIFT_B     0

  #define CR_TX_AMP_OFFSET_MASK_A    0x3F00
  #define CR_TX_AMP_OFFSET_MASK_B    0x003F

CL45RegAddr const Dev1E_R50OHM_RSEL_CTRL_0 = {0x1E, 0x0174};
  #define RG_R50ohm_RSEL_TX_A_EN       (1<<15)
  #define RG_R50ohm_RSEL_TX_B_EN       (1<<7)
  #define CR_R50ohm_RSEL_TX_SHIFT_A    8
  #define CR_R50ohm_RSEL_TX_SHIFT_B    0

  #define CR_R50ohm_RSEL_TX_MASK_A   0x7F00
  #define CR_R50ohm_RSEL_TX_MASK_B   0x007F

CL45RegAddr const Dev1E_ANACAL_REG_5 = {0x1E, 0x00E0};
  #define RG_REXT_TRIM_SHIFT           8
  #define RG_ZCAL_CTRL_SHIFT           0
  #define RG_ZCAL_CTRL_0dB             0x00

  #define REXT_ZCAL_VALID_BITS       6
  #define RG_REXT_CTRL_MASK          0x3F00
  #define RG_ZCAL_CTRL_MASK          0x003F

CL45RegAddr const Dev1F_BG_RASEL = {0x1F, 0x0115};

CL45RegAddr const Dev1F_LED_BASIC_CONTROL = {0x1F, 0x0021};

CL45RegAddr const Dev1F_TXVLD_DA_REG8 = {0x1F, 0x0272};

CL45RegAddr const Dev1F_10M_DRIVER_REG = {0x1F, 0x027C};

CL45RegAddr const Dev1E_1_TX_MLT3_SHAPER_0_1 = {0x1E, 0x0001};

CL45RegAddr const Dev1E_2_TX_MLT3_SHAPER_0_1 = {0x1E, 0x0002};

CL45RegAddr const Dev1E_2_TX_MLT3_SHAPER_1_0 = {0x1E, 0x0005};

CL45RegAddr const Dev1E_2_TX_MLT3_SHAPER_0_m1 = {0x1E, 0x0008};

CL45RegAddr const Dev1E_2_TX_MLT3_SHAPER_m1_0 = {0x1E, 0x000B};

CL45RegAddr const Dev1E_PD_TX_LD_CTRL = {0x1E, 0x003E};
  #define FORCE_EN_TXVLD_ABCD          0xF000

CL45RegAddr const Dev1E_ANALOG_REG_040 = {0x1E, 0x0040};
#endif

int StarPhyMode(StarDev *dev)
{
#ifdef INTERNAL_PHY
    return INT_PHY_MODE;
#else
    return EXT_MII_MODE;
#endif
}

#ifdef USE_EXT_CLK_SRC
static int StarIntPhyClkExtSrc(StarDev *dev, int enable)
{
    volatile u16 data;
    StarPrivate *starPrv = dev->starPrv;
    
    if (enable)
    {
        StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1f, 0x2a30);
        
        data = StarMdcMdioRead(dev, starPrv->phy_addr, 0x0f);
        data |= (1<<8);
        StarMdcMdioWrite(dev, starPrv->phy_addr, 0x0f, data);
        
        data = StarMdcMdioRead(dev, starPrv->phy_addr, 0x01);
        data |= (1<<3);
        StarMdcMdioWrite(dev, starPrv->phy_addr, 0x01, data);

        StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1f, 0x0);
    }

    return 0;
}
#endif /* USE_EXT_CLK_SRC */

#ifdef INTERNAL_PHY
void vStarSetSlewRate(StarDev *dev , u32 uRate)
{
	StarPrivate *starPrv = dev->starPrv;

	if(uRate>0x06) {
		STAR_MSG(STAR_ERR, "setting fail ! value must be 0 ~ 6 \n");
		return;
	}

	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x2a30);
	switch(uRate)
	{
		case 0:
			StarMdcMdioWrite(dev, starPrv->phy_addr, 0x15, 0x0F01);
			break;
		case 1:
			StarMdcMdioWrite(dev, starPrv->phy_addr, 0x15, 0x0F03);
			break;
		case 2:
			StarMdcMdioWrite(dev, starPrv->phy_addr, 0x15, 0x0F07);
			break;
		case 3:
			StarMdcMdioWrite(dev, starPrv->phy_addr, 0x15, 0x0F0f);
			break;
		case 4:
			StarMdcMdioWrite(dev, starPrv->phy_addr, 0x15, 0x0F1f);
			break;
		case 5:
			StarMdcMdioWrite(dev, starPrv->phy_addr, 0x15, 0x0F3f);
			break;
		case 6:
			StarMdcMdioWrite(dev, starPrv->phy_addr, 0x15, 0x0F7f);
			break;
		default:
			StarMdcMdioWrite(dev, starPrv->phy_addr, 0x15, 0x0F0f);
			break;
	}  
}

void vStarGetSlewRate(StarDev *dev , u32 * uRate)
{
	StarPrivate *starPrv = dev->starPrv;

	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x2a30);
	switch(StarMdcMdioRead(dev, starPrv->phy_addr, 0x15)&0x7F)
	{
	case 0x01:
		*uRate = 0;
		break;
	case 0x03:
		*uRate = 1;
		break;
	case 0x07:
		*uRate = 2;
		break;
	case 0x0f:
		*uRate = 3;
		break;
	case 0x1f:
		*uRate = 4;
		break;
	case 0x3f:
		*uRate = 5;
		break;
	case 0x7f:
		*uRate = 6;
		break;
	default:
		break;
	}  
}

void vStarSetOutputBias(StarDev *dev , u32 uOBiasLevel)
{
	u32 val;
	StarPrivate *starPrv = dev->starPrv;

	if(uOBiasLevel > 0x03) {
		STAR_MSG(STAR_ERR, "setting fail ! value must be 0 ~ 3 \n");
		return;
	}

	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x2a30);

	uOBiasLevel &= 0x03;	

	val = StarMdcMdioRead(dev, starPrv->phy_addr, 0x1d);  ////REG_TXG_OBIAS_100  //default value = 1
	val = ( val & (~(0x03<<4)))|((uOBiasLevel & 0x03)<< 4);//bit4~ bit5
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1d, val);

	val = StarMdcMdioRead(dev, starPrv->phy_addr, 0x19);  ////REG_TXG_OBIAS_STB_100  //default value = 2
	val = ( val & (~(0x03<<4)))|((uOBiasLevel & 0x03)<< 4);//bit4~ bit5
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x19, val);

	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x0);//Normal Page
}

void vStarGetOutputBias(StarDev *dev , u32 * uOBiasLevel)
{
	u32 val;
	StarPrivate *starPrv = dev->starPrv;

	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x2a30);
	val = StarMdcMdioRead(dev, starPrv->phy_addr, 0x1d);  ////REG_TXG_OBIAS_100  //default value = 1
	val = ( val>>4)  & 0x03 ;
	*uOBiasLevel = val ;
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x0);//Normal Page
}

void vStarSetInputBias(StarDev *dev , u32 uInBiasLevel)
{
	u32 val;
	StarPrivate *starPrv = dev->starPrv;

	if(uInBiasLevel > 0x03) {
		STAR_MSG(STAR_ERR, "setting fail ! value must be 0 ~ 3 \n");
		return;
	}

	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x2a30);
		
	uInBiasLevel = 0x03 - (uInBiasLevel& 0x03);	

	val = StarMdcMdioRead(dev, starPrv->phy_addr, 0x1D);
	val = ( val& 0xfffc)|((uInBiasLevel & 0x03)<< 0);//bit 0~ bit1	//REG_TXG_BIASCTRL	default = 0
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1d, val);

	val = StarMdcMdioRead(dev, starPrv->phy_addr, 0x19);
	val = ( val& 0xfffc)|((uInBiasLevel & 0x03)<< 0);//bit 0~ bit1 //REG_TXG_BIASCTRL  default = 3
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x19, val);

	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x0);//Normal Page
}

void vStarGetInputBias(StarDev *dev , u32 * uInBiasLevel)
{
	u32 val;
	StarPrivate *starPrv = dev->starPrv;

	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x2a30);	
	val = StarMdcMdioRead(dev, starPrv->phy_addr, 0x1D);
	val =  val & 0x03;//bit 0~ bit1	//REG_TXG_BIASCTRL	default = 0
	*uInBiasLevel = 0x03 - val;
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x0);//Normal Page
}

void vSetDACAmp(StarDev *dev , u32 uLevel)
{
	u32 val;
	StarPrivate *starPrv = dev->starPrv;

	if(uLevel > 0x0F) {
	  STAR_MSG(STAR_ERR, "setting fail ! value must be 0~15 \n");
	  return;
	}

	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x2a30);
	val = StarMdcMdioRead(dev, starPrv->phy_addr, 0x1E);
	val = ( val & (~(0x0F<<12)))|((uLevel & 0x0F)<< 12);
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1E, val);  
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x0);//Normal Page
}
void vGetDACAmp(StarDev *dev , u32 * uLevel)
{
	u32 val;
	StarPrivate *starPrv = dev->starPrv;
	
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x2a30);
	val = StarMdcMdioRead(dev, starPrv->phy_addr, 0x1E);
	*uLevel = ( val>>12)& 0x0f ;
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x0);//Normal Page
}
void vSet100Amp(StarDev *dev , u32 uLevel) /////100M DAC 
{
	u32 val;
	StarPrivate *starPrv = dev->starPrv;

	if(uLevel > 0x0F) {
	  STAR_MSG(STAR_ERR, "setting fail ! value must be 0~15 \n");
	  return;
	}

	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x2a30);
	val = StarMdcMdioRead(dev, starPrv->phy_addr, 0x1A);
	val = ( val & (~0x0F))|(uLevel & 0x0F);
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1A, val);  
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x0);//Normal Page
}
void vGet100Amp(StarDev *dev , u32 * uLevel)
{
	u32 val;
	StarPrivate *starPrv = dev->starPrv;

	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x2a30);
	val = StarMdcMdioRead(dev, starPrv->phy_addr, 0x1A);
	*uLevel =  val & 0x0f ;
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x0);//Normal Page
}

void vSet100SlewRate(StarDev *dev , u32 uLevel) //100M SlewRate
{
	u32 val;
	StarPrivate *starPrv = dev->starPrv;

	if(uLevel > 0x7F) {
	  STAR_MSG(STAR_ERR, "setting fail ! value must be 0x00~0x7f \n");
	  return;
	}

	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x2a30);
	val = StarMdcMdioRead(dev, starPrv->phy_addr, 0x15);
	val = ( val & (~0x7F))|(uLevel & 0x7F);
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x15, val);  
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x0);//Normal Page
}
void vGet100SlewRate(StarDev *dev , u32 * uLevel)
{
	u32 val;
	StarPrivate *starPrv = dev->starPrv;

	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x2a30);
	val = StarMdcMdioRead(dev, starPrv->phy_addr, 0x15);
	*uLevel =  val & 0x7f ;
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x0);//Normal Page
}

void vStarSet10MAmp(StarDev *dev , u32 uLevel)
{
  u32 val;
  StarPrivate *starPrv = dev->starPrv;
  
  if(uLevel > 3) {
    STAR_MSG(STAR_ERR, "setting fail ! value must be 0~3 \n");
	return;
  }

  StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x2a30); 	
  val = StarMdcMdioRead(dev, starPrv->phy_addr, 0x18);
  val = ( val& (~(0x03<<0)))|((uLevel & 0x03)<< 0);//bit 0, bit1 default =2
  StarMdcMdioWrite(dev, starPrv->phy_addr, 0x18, val);
  StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x0);//Normal Page
}

void vStarGet10MAmp(StarDev *dev , u32* uLevel)
{
  u32 val;
  StarPrivate *starPrv = dev->starPrv;
  
  StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x2a30); 	
  val = StarMdcMdioRead(dev, starPrv->phy_addr, 0x18);
  val = ( val);//bit 0, bit1 default =2
  *uLevel=val & 0x03 ;
  StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x0);//Normal Page
}

void vStar100mTx(StarDev *dev)
{
	StarPrivate *starPrv = dev->starPrv;

	STAR_MSG(STAR_WARN, "100M TX\n");

	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1f, 0x2a30);
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x05, 0x1010);
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x0d, 0x1080);
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x04, 0x02a0);
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1f, 0x0000);
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x00, 0x2100);
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1f, 0x0001);
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1d, 0x0040);
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1f, 0x0000);
}

void vStar10mLinkPulse(StarDev *dev)
{
	StarPrivate *starPrv = dev->starPrv;

	STAR_MSG(STAR_WARN, "10M Link Pulse\n");

	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1f, 0x2a30);
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x05, 0x1010);
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x0d, 0x1080);
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x04, 0x02a0);
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1f, 0x0000);
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x00, 0x0100);
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1f, 0x0001);
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1d, 0x0040);
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1f, 0x0000);
}

void vStar100mReturnLoss(StarDev *dev)
{
	StarPrivate *starPrv = dev->starPrv;

	STAR_MSG(STAR_WARN, "100M Return Loss\n");

	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1f, 0x2a30);
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x05, 0x1010);
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x0d, 0x1000);
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x04, 0x02a0);
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1f, 0x0000);
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x00, 0x2100);
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1f, 0x0001);
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1d, 0x0040);
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1f, 0x0000);
}

void vStar10mPseudoRandom(StarDev *dev)
{
	StarPrivate *starPrv = dev->starPrv;

	STAR_MSG(STAR_WARN, "10M Pseudo Random\n");

	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1f, 0x2a30);
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x05, 0x1010);
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x0d, 0x1080);
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x04, 0x02a0);
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1f, 0x0000);
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x00, 0x0100);
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1f, 0x0001);
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1d, 0x0040);
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1e, 0x5678);
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1d, 0xf840);
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1f, 0x0000);
}

void vStar10mHarmonic(StarDev *dev)
{
	StarPrivate *starPrv = dev->starPrv;

	STAR_MSG(STAR_WARN, "10M Harmonic\n");

	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1f, 0x2a30);
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x05, 0x1010);
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x0d, 0x1080);
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x04, 0x02a0);
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1f, 0x0000);
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x00, 0x0100);
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1f, 0x0001);
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1d, 0x0040);
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1e, 0xffff);
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1d, 0xf840);
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1f, 0x0000);
}

void vStarSet50PercentBW(StarDev *dev , u32 uEnable)
{
  u32 val;
  StarPrivate *starPrv = dev->starPrv;
  
  if(uEnable > 1) {
    STAR_MSG(STAR_ERR, "setting fail ! value must be 0~1 \n");
	return;
  }

  StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x2a30);
  val = StarMdcMdioRead(dev, starPrv->phy_addr, 0x1D);
  val = ( val & (~(0x01<<14)))|((uEnable & 0x01)<< 14);//bit 14 default 0
  StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1D, val);
  StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x0);//Normal Page
}

void vStarGet50PercentBW(StarDev *dev , u32* uEnable)
{
  u32 val;
  StarPrivate *starPrv = dev->starPrv;

  StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x2a30);
  val = StarMdcMdioRead(dev, starPrv->phy_addr, 0x1D);
  val = ( val >> 14)&0x01;//bit 14 default 0
  *uEnable= val ;
  StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x0);//Normal Page
}

void vStarSetFeedBackCap(StarDev *dev , u32 uLevel)
{
  u32 val;
  StarPrivate *starPrv = dev->starPrv;

  if(uLevel > 3) {
    STAR_MSG(STAR_ERR, "setting fail ! value must be 0~3 \n");
	return;
  }

  StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x2a30);//test page
  val = StarMdcMdioRead(dev, starPrv->phy_addr, 0x1D);
  val = ( val & (~(0x03<<8)))|((uLevel & 0x03)<< 8);//bit 8 bit 9  default 0
  StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1D, val);
  StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x0);//Normal Page
}

void vStarGetFeedBackCap(StarDev *dev , u32* uLevel)
{
  u32 val;
  StarPrivate *starPrv = dev->starPrv;
  
  StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x2a30);//test page
  val = StarMdcMdioRead(dev, starPrv->phy_addr, 0x1D);
  val = ( val >> 8);//bit 8 bit 9  default 0
  *uLevel= val & 0x03 ; 
  StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x0);//Normal Page
}

#if defined(CONFIG_ARCH_MT5891) || defined(CONFIG_ARCH_MT5886) || defined(CONFIG_ARCH_MT5893)
void StarGainSet(StarDev *dev, int value)
{
	u32 TempLow,TempHi;
	StarPrivate *starPrv = dev->starPrv;

	STAR_MSG(STAR_ERR,"StarGainSet to %d\n", value);

	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1f, 0x2a30);
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1f, 0x2a30);
	star_mb();
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x09,
			((1<<15)|(1<<14)
			| StarMdcMdioRead(dev, starPrv->phy_addr, 0x09)));
	star_mb();
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1f, 0x52b5);
	star_mb();
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x10, 0xafa4);
	star_mb();
	TempLow = StarMdcMdioRead(dev, starPrv->phy_addr, 0x11);
	TempHi = StarMdcMdioRead(dev, starPrv->phy_addr, 0x12);
	TempHi = (TempHi & (~(0x7f<<1)))|(value<<1);
	star_mb();
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x11, TempLow);
	star_mb();
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x12, TempHi);
	star_mb();
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x10, 0x8fa4);
	star_mb();
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1f, 0x00);
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x00);
}
#endif

#if defined(CONFIG_ARCH_MT5886)
static int star_phy_efuse(StarDev *dev)
{
	u32 reg_val, eth0_9, ETH0_5, bim6d8, bim6dc, tmp;
	StarPrivate *starPrv = dev->starPrv;

	bim6d8 = StarGetReg(dev->bim_base +0x6d8);
	bim6dc = StarGetReg(dev->bim_base +0x6dc);

	if((((bim6d8 >> 20) & 0x3ff) == 0) && ((((bim6dc >> 24)) & 0x3f) == 0)) {
		STAR_MSG(STAR_DBG, "not efuse done IC\n");
		return 1;
	}

	/* eth[9:0] = eth0_9 */
	eth0_9 = (bim6d8 >> 20) &0x3ff;

	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x2a30);

	/* RG_TX_DMY[15:12]=eth[7:4] */
	reg_val = StarMdcMdioRead(dev, starPrv->phy_addr, 0x1a);
	reg_val &= ~(0xf << 12);
	reg_val |= ((eth0_9 >> 4) & 0xf) << 12;
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1a, reg_val);

	ETH0_5 = (bim6dc >> 24)&0x3f;
	/* {Ethernet[1:0],eth[9:8]} = tmp */
	tmp = 0;
	tmp = ((eth0_9 >> 8) & 0x3) + ((ETH0_5 & 0x3) << 2) + 0x8;

	/* RG_DACG_AMP[3:0]={Ethernet[1:0],eth[9:8]} */
	reg_val = StarMdcMdioRead(dev, starPrv->phy_addr, 0x1a);
	reg_val &= ~0xf;
	if (tmp <= 0xf)
		reg_val |= tmp;
	else
		reg_val += 0xf;
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1a, reg_val);

	/* RG_DACT_AMPCTRL */
	reg_val = StarMdcMdioRead(dev, starPrv->phy_addr, 0x1e);
	reg_val &= ~(0xf << 12);
	reg_val |= (((ETH0_5 >> 2) & 0xf) + 3) << 12;
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1e, reg_val);

	return 0;
}
#endif

#if defined(CONFIG_ARCH_MT5893)
static int star_phy_efuse(StarDev *dev)
{
	u16 eth15_0, mdi_cal, reg_val;
	u32 bim6dc;
	u8 map_table[16] = {0x1e, 0x1d, 0x1c, 0x1b, 0x19, 0x18, 0x17, 0x16,
			    0x14, 0x12, 0x10, 0xe, 0xc, 0x9, 0x7, 0x5};
	StarPrivate *starPrv = dev->starPrv;

	bim6dc = StarGetReg(dev->bim_base + 0x6dc);

	if((bim6dc & 0xffff) == 0) {
		STAR_MSG(STAR_DBG, "not efuse done IC\n");
		return 1;
	}

	/* eth15_0 = bim6dc[15:0] */
	eth15_0 = bim6dc & 0xffff;

	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x2a30);

	reg_val = StarMdcMdioRead(dev, starPrv->phy_addr, 0x14);
	reg_val |= 0x1040;
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x14, reg_val);

	/* MDI_CAL_VAL[11:8] = eth15_0[15:12] */
	reg_val = StarMdcMdioRead(dev, starPrv->phy_addr, 0x14);
	mdi_cal = (eth15_0 >> 12) & 0xf;
	reg_val &= ~(0xf << 8);
	reg_val |= mdi_cal << 8;
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x14, reg_val);

	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x3500);

	/* TST_RESERVE_INT[4:0] = map_table[mdi_cal] */
	reg_val = StarMdcMdioRead(dev, starPrv->phy_addr, 0x3);
	reg_val &= ~0xff;
	reg_val |= map_table[mdi_cal];
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x3, reg_val);

	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x2a30);

	/* RG_TX_DMY[15:12] = eth15_0[11:8] */
	reg_val = StarMdcMdioRead(dev, starPrv->phy_addr, 0x1a);
	reg_val &= ~(0xf << 12);
	reg_val |= ((eth15_0 >> 8) & 0xf) << 12;
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1a, reg_val);

	/* RG_DACG_AMP_100[3:0] = eth15_0[7:4] */
	reg_val = StarMdcMdioRead(dev, starPrv->phy_addr, 0x1a);
	reg_val &= ~0xf;
	if (((eth15_0 >> 4) & 0xf) < 0xb)
		reg_val |= ((eth15_0 >> 4) & 0xf) + 0x4;
	else
		reg_val |= 0xf;
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1a, reg_val);

	/* RG_DACT_AMPCTRL[15:12] = eth15_0[3:0] */
	reg_val = StarMdcMdioRead(dev, starPrv->phy_addr, 0x1e);
	reg_val &= ~(0xf << 12);
	if ((eth15_0 & 0xf) < 0xb)
		reg_val |= ((eth15_0 & 0xf) + 0x4) << 12;
	else
		reg_val |= 0xf << 12;
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1e, reg_val);

	return 0;
}
#endif

#if defined(CONFIG_ARCH_MT5863)
static int star_phy_efuse(StarDev *dev)
{
	u32 bim6b8, bim6bc;
	u16 reg_val, reg_val_tmp;
	StarPrivate *starPrv = dev->starPrv;

	bim6b8 = StarGetReg(dev->bim_base + 0x6b8);
	bim6bc = StarGetReg(dev->bim_base + 0x6bc);

	if((bim6b8 == 0) && ((bim6bc & 0xfffff) == 0)) {
		STAR_MSG(STAR_DBG, "not efuse done IC\n");
		return 1;
	}

	/* R50OHM_RSEL_TX_A[6:0] = bim6b8[24:18], R50OHM_RSEL_TX_B[6:0] = bim6b8[31:25] */
	reg_val = 0x8080 | (((bim6b8 >> 18) & 0x7f) << 8) | ((bim6b8 >> 25) & 0x7f);
	StarMdcMdioWrite45(dev, starPrv->phy_addr, Dev1E_R50OHM_RSEL_CTRL_0.regAddr, Dev1E_R50OHM_RSEL_CTRL_0.devAddr, reg_val);

	/* RG_REXT_TRIM[5:0] = bim6bc[16:11] */
	reg_val = StarMdcMdioRead45(dev, starPrv->phy_addr, Dev1E_ANACAL_REG_5.regAddr, Dev1E_ANACAL_REG_5.devAddr);
	reg_val &= ~0x3f00;
	reg_val |= ((bim6bc >> 11) & 0x3f) << 8;
	StarMdcMdioWrite45(dev, starPrv->phy_addr, Dev1E_ANACAL_REG_5.regAddr, Dev1E_ANACAL_REG_5.devAddr, reg_val);

	/* RG_BG_RASEL[2:0] = bim6bc[19:17] */
	reg_val = (bim6bc >> 17) & 0x7;
	StarMdcMdioWrite45(dev, starPrv->phy_addr, Dev1F_BG_RASEL.regAddr, Dev1F_BG_RASEL.devAddr, reg_val);

	/* cr_tx_amp_offset_a[5:0] =  bim6b8[17:12], cr_tx_amp_offset_b[5:0] =  bim6bc[5:0] */
	reg_val = (((bim6b8 >> 12) & 0x3f) << 8) | (bim6bc & 0x3f);
	StarMdcMdioWrite45(dev, starPrv->phy_addr, Dev1E_TX_OFFSET_0.regAddr, Dev1E_TX_OFFSET_0.devAddr, reg_val);

	/* da_tx_i2mpb_a_gbe[5:0]  da_tx_i2mpb_a_tbt[5:0] da_tx_i2mpb_a_hbt[5:0] da_tx_i2mpb_a_tst[5:0] = bim6b8[5:0] */
	reg_val_tmp = bim6b8 & 0x3f;
	reg_val = (reg_val_tmp << 10) | reg_val_tmp;
	StarMdcMdioWrite45(dev, starPrv->phy_addr, Dev1E_TX_I2MPB_A_1.regAddr, Dev1E_TX_I2MPB_A_1.devAddr, reg_val);
	reg_val = ((reg_val_tmp + 6) << 10) | reg_val_tmp;
	StarMdcMdioWrite45(dev, starPrv->phy_addr, Dev1E_TX_I2MPB_A_2.regAddr, Dev1E_TX_I2MPB_A_2.devAddr, reg_val);

	/* da_tx_i2mpb_b_gbe[5:0]  da_tx_i2mpb_b_tbt[5:0] da_tx_i2mpb_b_hbt[5:0] da_tx_i2mpb_b_tst[5:0] = bim6b8[11:6] */
	reg_val_tmp = (bim6b8 >> 6) & 0x3f;
	reg_val = (reg_val_tmp << 8) | reg_val_tmp;
	StarMdcMdioWrite45(dev, starPrv->phy_addr, Dev1E_TX_I2MPB_B_1.regAddr, Dev1E_TX_I2MPB_B_1.devAddr, reg_val);
	reg_val = ((reg_val_tmp + 6) << 8) | reg_val_tmp;
	StarMdcMdioWrite45(dev, starPrv->phy_addr, Dev1E_TX_I2MPB_B_2.regAddr, Dev1E_TX_I2MPB_B_2.devAddr, reg_val);

	return 0;
}

static void StarInternalPhyInit(StarDev *dev)
{
	u16 data;
	StarPrivate *starPrv = dev->starPrv;

	StarMdcMdioWrite45(dev, starPrv->phy_addr, Dev1F_TXVLD_DA_REG8.regAddr, Dev1F_TXVLD_DA_REG8.devAddr, 0x20ff);
	StarMdcMdioWrite45(dev, starPrv->phy_addr, Dev1E_1_TX_MLT3_SHAPER_0_1.regAddr, Dev1E_1_TX_MLT3_SHAPER_0_1.devAddr, 0x01ce);
	StarMdcMdioWrite45(dev, starPrv->phy_addr, Dev1E_2_TX_MLT3_SHAPER_0_1.regAddr, Dev1E_2_TX_MLT3_SHAPER_0_1.devAddr, 0x01c4);
	StarMdcMdioWrite45(dev, starPrv->phy_addr, Dev1E_2_TX_MLT3_SHAPER_1_0.regAddr, Dev1E_2_TX_MLT3_SHAPER_1_0.devAddr, 0x0204);
	StarMdcMdioWrite45(dev, starPrv->phy_addr, Dev1E_2_TX_MLT3_SHAPER_0_m1.regAddr, Dev1E_2_TX_MLT3_SHAPER_0_m1.devAddr, 0x03c5);
	StarMdcMdioWrite45(dev, starPrv->phy_addr, Dev1E_2_TX_MLT3_SHAPER_m1_0.regAddr, Dev1E_2_TX_MLT3_SHAPER_m1_0.devAddr, 0x0004);
	StarMdcMdioWrite45(dev, starPrv->phy_addr, Dev1E_PD_TX_LD_CTRL.regAddr, Dev1E_PD_TX_LD_CTRL.devAddr, FORCE_EN_TXVLD_ABCD);
	StarMdcMdioWrite45(dev, starPrv->phy_addr, Dev1E_ANALOG_REG_040.regAddr, Dev1E_ANALOG_REG_040.devAddr, 0x0);

	/* set PHY register by efuse or use default value */
	if (star_phy_efuse(dev)) {
		/* DC calibration default value */
		/* R50OHM_RSEL_TX_A[6:0] = 7'b1000000 , R50OHM_RSEL_TX_B[6:0] = 7'b1000000 */
		StarMdcMdioWrite45(dev, starPrv->phy_addr, Dev1E_R50OHM_RSEL_CTRL_0.regAddr, Dev1E_R50OHM_RSEL_CTRL_0.devAddr, 0xc0c0);

		/* RG_REXT_TRIM[5:0] = 6'b100000 */
		data = StarMdcMdioRead45(dev, starPrv->phy_addr, Dev1E_ANACAL_REG_5.regAddr, Dev1E_ANACAL_REG_5.devAddr);
		data &= ~(RG_REXT_CTRL_MASK);
		data |= (0x2000);
		StarMdcMdioWrite45(dev, starPrv->phy_addr, Dev1E_ANACAL_REG_5.regAddr, Dev1E_ANACAL_REG_5.devAddr, data);

		/* RG_BG_RASEL[2:0] = 3'b100 */
		StarMdcMdioWrite45(dev, starPrv->phy_addr, Dev1F_BG_RASEL.regAddr, Dev1F_BG_RASEL.devAddr, 0x4);
	}

	/* set PHY LED gpio */
	StarSetBitMask((dev->pdwncBase + 0x684), 0xff, 24, 0x11);

	/* set LED Mode 01: LED0:on for 10/100/1000M linkup; LED1:blink for 10/100/1000 tx/rx*/
	data = StarMdcMdioRead45(dev, starPrv->phy_addr, Dev1F_LED_BASIC_CONTROL.regAddr, Dev1F_LED_BASIC_CONTROL.devAddr);
	data &= ~(0x8003);
	data |= (0x1);
	StarMdcMdioWrite45(dev, starPrv->phy_addr, Dev1F_LED_BASIC_CONTROL.regAddr, Dev1F_LED_BASIC_CONTROL.devAddr, data);

	STAR_MSG(STAR_DBG, "PHY Init done.\n");
}
#else
static void StarInternalPhyInit(StarDev *dev)
{
    volatile u32 data;
    StarPrivate *starPrv = dev->starPrv;

#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891) || defined(CONFIG_ARCH_MT5886) || defined(CONFIG_ARCH_MT5893)
	mdelay(50); // Delay for the Phy working in a normal status.
#endif

#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891) || defined(CONFIG_ARCH_MT5886) || defined(CONFIG_ARCH_MT5893)
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1f, 0x3500); // Extended test page

	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x08, 0xfc55); 

	data = StarMdcMdioRead(dev, starPrv->phy_addr, 0x02);
	data = data | 1;
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x02, data); 

	udelay(1);
    
	data = StarMdcMdioRead(dev, starPrv->phy_addr, 0x02);
	data = data & ~1;
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x02, data); 
    
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x07, 0x0c10);
 
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1f, 0x2a30);
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x19, 0x778f);
#if defined(CONFIG_ARCH_MT5891) || defined(CONFIG_ARCH_MT5886) || defined(CONFIG_ARCH_MT5893)
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1d, 0x1052);
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x15, 0x0f2f);
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1, 0x8000);
#else
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1d, 0x1051);
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x15, 0x0f5f);
#endif
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1f, 0x00);
#endif

#if defined(CONFIG_ARCH_MT5880)|| defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5882) || defined(CONFIG_ARCH_MT5865) || defined(CONFIG_ARCH_MT5883)|| defined(CONFIG_ARCH_MT5891) || defined(CONFIG_ARCH_MT5886) || defined(CONFIG_ARCH_MT5893)
        // configure LED 
#if defined(CONFIG_ARCH_MT5882) || defined(CONFIG_ARCH_MT5865) || defined(CONFIG_ARCH_MT5883)
	mdelay(100); //maybe can remove
        StarSetReg(dev->base+0x6c,StarGetReg(dev->base+0x6c)|(1<<2)); // set FRC_SMI_ENB to write phy successfully
#endif
    StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1f, 0x0002); //misc page
    StarMdcMdioWrite(dev, starPrv->phy_addr, 0x00, 0x80f0);  // EXT_CTL([15]) = 1
    StarMdcMdioWrite(dev, starPrv->phy_addr, 0x03, 0xc000);  // LED0 on clean
    StarMdcMdioWrite(dev, starPrv->phy_addr, 0x04, 0x003f);  // LED0 blink for 10/100/1000 tx/rx
    StarMdcMdioWrite(dev, starPrv->phy_addr, 0x05, 0xc007);  // LED1 on for 10/100/1000M linkup
    StarMdcMdioWrite(dev, starPrv->phy_addr, 0x06, 0x0000);  // LED1 blink clean
    StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x0000);  // main page
#endif

#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5882) || defined(CONFIG_ARCH_MT5865) || defined(CONFIG_ARCH_MT5883)|| defined(CONFIG_ARCH_MT5891) || defined(CONFIG_ARCH_MT5886) || defined(CONFIG_ARCH_MT5893)
        StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x2a30);  // test page
        
        data = StarMdcMdioRead(dev, starPrv->phy_addr, 0x1A);
        data = (data & 0xfff0)|(0x03 & 0x0f);//100M amp
        StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1A, data);  // 100M slew rate
        
        data = StarMdcMdioRead(dev, starPrv->phy_addr, 0x15);
        data = (data & 0xff00) |((0x7f & 0x7f)<< 0);//100M slew rate
        StarMdcMdioWrite(dev, starPrv->phy_addr, 0x15, data);  // 100M slew rate
	   
        data = StarMdcMdioRead(dev, starPrv->phy_addr, 0x18);
        data = (data & 0xfffc) |((0 & 0x03)<< 0);//10M amp
        StarMdcMdioWrite(dev, starPrv->phy_addr, 0x18, data);  // 100M slew rate
        
        data = StarMdcMdioRead(dev, starPrv->phy_addr, 0x1E);
        data = (data & 0x0fff) |((0x3)<< 12);//10M amp
        StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1E, data);  // 100M slew rate
        
        StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x0000);  // main page
        
#elif defined(CONFIG_ARCH_MT5880)|| defined (CONFIG_ARCH_MT5881)
        StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x2a30);  // test page
        msleep(50);
        data = StarMdcMdioRead(dev, starPrv->phy_addr, 0x15);
        data = (data & 0xff00) | 0x2f;
		StarMdcMdioWrite(dev, starPrv->phy_addr, 0x15, data);  // 100M slew rate
		msleep(50);
        data = StarMdcMdioRead(dev, starPrv->phy_addr, 0x18);
        data = (data & 0xfffC) | 0x0;
		StarMdcMdioWrite(dev, starPrv->phy_addr, 0x18, data);  // 10M amplitude
		msleep(50);
		StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x0000);  // main page
        StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x0000);  // main page
#endif

    //End Internal PHY Setting
    StarClearBit(STAR_PHY_CTRL1(dev->base), STAR_PHY_CTRL1_APDIS);//Enable Auto Polling after  set PHY register

#if defined(CONFIG_ARCH_MT5890) || defined (CONFIG_ARCH_MT5891) || defined(CONFIG_ARCH_MT5886) || defined(CONFIG_ARCH_MT5893)
	vSetDACAmp(dev, 0xa);

	#if defined (CONFIG_ARCH_MT5891) || defined(CONFIG_ARCH_MT5886) || defined(CONFIG_ARCH_MT5893)
	vSet100Amp(dev, 0xa);
	#else
	vSet100Amp(dev, 0x8);
	#endif
#else
	vSetDACAmp(dev, INTERNAL_PHY_DACAMP);
	vSet100Amp(dev, INTERNAL_PHY_100AMP);
#endif

	vStarSet10MAmp(dev, INTERNAL_PHY_10M_AMP_LEVEL);

#if defined(CONFIG_ARCH_MT5880) || defined(CONFIG_ARCH_MT5883)
	vSet100Amp(dev, 4);  
#endif

#if defined(CONFIG_ARCH_MT5883)
	vSet100SlewRate(dev, 0x3f);
#endif

#if defined(CONFIG_ARCH_MT5891)|| defined(CONFIG_ARCH_MT5886) || defined(CONFIG_ARCH_MT5893)
	StarGainSet(dev, 0x1f);
#endif

#if defined(CONFIG_ARCH_MT5886) || defined(CONFIG_ARCH_MT5893)
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x3500);
#if defined(CONFIG_ARCH_MT5893)
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x03, 0x0010);
#else
	data = StarMdcMdioRead(dev, starPrv->phy_addr, 0x03);
	data |= (1<<4) | (1<<5);
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x03, data);
#endif
	msleep(5);
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x2a30);
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1d, 0x1052);
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x15, 0x0F0F);

	data = StarMdcMdioRead(dev, starPrv->phy_addr, 0x14);
	data |= 1<<12;
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x14, data);
	msleep(5);

	data = StarMdcMdioRead(dev, starPrv->phy_addr, 0x14);
	data &= ~(1<<12);
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x14, data);
	msleep(5);

	data = StarMdcMdioRead(dev, starPrv->phy_addr, 0x14);
	data |= 1<<12;
	if (((data >> 8) &0xF) >= 0xE)
		data |= 0xF << 8;
	else
		data += (0x2 << 8);
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x14, data);

	if (star_phy_efuse(dev)) {
		StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x2a30);
		StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1a, 0x000b);
		StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1e, 0x3000);
	}

	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1f, 0x3500);
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x08, 0xfc50);
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x0);
#endif

#if defined(CONFIG_ARCH_MT5882) || defined(CONFIG_ARCH_MT5865)
#if defined(CONFIG_ARCH_MT5865)
#else
	if (IS_IC_MT5885())
#endif
	{
		/* For 100M return loss test */
		StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x2a30);
		StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1d, 0x1051);
		StarMdcMdioWrite(dev, starPrv->phy_addr, 0x15, 0x0f1f);
		StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x0);

		/* write PMA Dbg09 register to 0x1a689 for 200m cable packet loss */
		StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x52b5);
		StarMdcMdioWrite(dev, starPrv->phy_addr, 0x11, 0xa689);
		StarMdcMdioWrite(dev, starPrv->phy_addr, 0x12, 0x1);
		StarMdcMdioWrite(dev, starPrv->phy_addr, 0x10, 0x8f92);
		StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x0);
	}
#endif
}
#endif //#else defined(CONFIG_ARCH_MT5863)

void StarInternalPhyReset(StarDev *dev)
{
    u16 val = 0;
    StarPrivate *starPrv = dev->starPrv;

    val = StarMdcMdioRead(dev, starPrv->phy_addr, 0);
    val |= 0x8000;
    StarMdcMdioWrite(dev, starPrv->phy_addr, 0, val);
}
#endif //#ifdef INTERNAL_PHY


#define PHY_REG_IDENTFIR2	(3) 	 /* Reg3: PHY Identifier 2 */

#define PHYID2_INTEPHY		(0x9481) /* Internal PHY */
#define PHYID2_INTVITESSE	(0x0430) /* Internal PHY */

static struct eth_phy_ops internal_phy_ops = {
	.init = StarInternalPhyInit,
	.reset = StarInternalPhyReset,
	.calibration = NULL,
};

int StarDetectPhyId(StarDev *dev)
{
	int phyId;
	u16 phyIdentifier2;

	for (phyId = 0; phyId < PHY_MAX_ADDR; phyId ++) {
		phyIdentifier2 = StarMdcMdioRead(dev, phyId, PHY_REG_IDENTFIR2);

		if (phyIdentifier2 == PHYID2_INTVITESSE) {
			STAR_MSG(STAR_DBG, "Internal Vitesse PHY\n");
			dev->phy_ops = &internal_phy_ops;
			dev->phy_ops->addr = phyId;
			break;
		}
		else if (phyIdentifier2 == PHYID2_INTEPHY) {
			STAR_MSG(STAR_DBG, "Internal EPHY \n");
			dev->phy_ops = &internal_phy_ops;
			dev->phy_ops->addr = phyId;
			break;
		}
	}

	/* If can't find phy id, try reading from PHY ID 2,
	 and check the return value. If success, should return
	 a value other than 0xffff.
	*/
    if (phyId >= PHY_MAX_ADDR) {
        for (phyId = 0; phyId < PHY_MAX_ADDR; phyId ++) {
            phyIdentifier2 = StarMdcMdioRead(dev, phyId, PHY_REG_IDENTFIR2);
            if (phyIdentifier2 != 0xffff)
                break;
        }
    }

	return phyId;
}



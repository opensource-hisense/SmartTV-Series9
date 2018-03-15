#ifndef _STAR_PHY_H_
#define _STAR_PHY_H_

#include <asm/io.h>
#include <asm/types.h>

#ifdef CONFIG_ARM64
#include "linux/soc/mediatek/platform.h"
#else
#include <mach/platform.h>
#endif

#define PHY_MAX_ADDR	32

#define INTERNAL_PHY_SLEW_RATE_LEVEL 3
#define INTERNAL_PHY_OUTPUT_BIAS_LEVEL 1 //0:+0%, 1,2:+100%, 3:+200%
#define INTERNAL_PHY_INPUT_BIAS_LEVEL 1 //0:66%, 1:200%, 2:+50%, 3:100%
#define INTERNAL_PHY_10M_AMP_LEVEL 0 //0:110%, 1:108.3% 2:105%, 3:103%
#define INTERNAL_PHY_FEED_BACK_CAP 3 //0:min compensation cap, 1:,2: Modeate, 3:Max
#define INTERNAL_PHY_DACAMP 4        //min:0x0 , max 0xf
#define INTERNAL_PHY_100AMP 5        //min:0x0 , max 0xf

#define INTERNAL_PHY_OUT_EXTRA_BAIS_CONTROL 1  //0:+0% 1:+100%
#define INTERNAL_PHY_EXTEND_10M_AMP_LEVEL 0 //0~0xf Increase
#define INTERNAL_PHY_50_PERCENT_BW_ENABLE 0 //0: BW 100% 1:50%

typedef struct
{
	u8 devAddr;
	u16 regAddr;
} CL45RegAddr;

int StarPhyMode(StarDev *dev);
int StarDetectPhyId(StarDev *dev);

#ifdef INTERNAL_PHY
void vStarSetSlewRate(StarDev *dev , u32 uRate);
void vStarGetSlewRate(StarDev *dev , u32 * uRate);
void vStarSetOutputBias(StarDev *dev , u32 uOBiasLevel);
void vStarGetOutputBias(StarDev *dev , u32 * uOBiasLevel);
void vStarSetInputBias(StarDev *dev , u32 uInBiasLevel);
void vStarGetInputBias(StarDev *dev , u32 * uInBiasLevel);
void vSetDACAmp(StarDev *dev , u32 uLevel);
void vGetDACAmp(StarDev *dev , u32 * uLevel);
void vSet100SlewRate(StarDev *dev , u32 uLevel);
void vGet100SlewRate(StarDev *dev , u32 * uLevel);
void vSet100Amp(StarDev *dev , u32 uLevel);
void vStarSet10MAmp(StarDev *dev , u32 uLevel);
void vStarGet10MAmp(StarDev *dev , u32* uLevel);
void vStarSet50PercentBW(StarDev *dev , u32 uEnable);
void vStarGet50PercentBW(StarDev *dev , u32* uEnable);
void vStarSetFeedBackCap(StarDev *dev , u32 uLevel);
void vStarGetFeedBackCap(StarDev *dev , u32* uLevel);

void vStar100mTx(StarDev *dev);
void vStar10mLinkPulse(StarDev *dev);
void vStar100mReturnLoss(StarDev *dev);
void vStar10mPseudoRandom(StarDev *dev);
void vStar10mHarmonic(StarDev *dev);
#endif

#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891)
int i4MacPhyZCALInit(StarDev *dev);
int i4MacPhyZCALRemapTable(StarDev *dev, u8 value);
#endif

#endif

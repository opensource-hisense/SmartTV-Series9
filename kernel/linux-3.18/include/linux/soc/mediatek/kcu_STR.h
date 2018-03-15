#ifndef KCU_STR_H_FILE
#define KCU_STR_H_FILE 1
//cat make.log |grep "undefined reference" | awk -F '`' '{print $2}' | sort -u | awk -F "'" 'BEGIN{print "#ifndef KCU_STR_H_FILE";print "#define KCU_STR_H_FILE 1"}{print "void "$1"(void);"}END{print "#endif"}' >kcu_STR.H
#ifdef __KERNEL__
typedef uint8_t  UINT8;
typedef uint32_t  UINT32;
typedef int32_t  INT32;
#endif
#define OUT_ONLY 
void ADC_pm_resume(void);
void AUD_pm_str_resume(void);
void AUD_pm_str_suspend(void);
void B2R_pm_str_resume(void);
void B2R_pm_str_suspend(void);
void bApiVideoMainSubSrc(UINT8 bMainSrc, UINT8 bSubSrc);
void CEC_pm_resume(void);
void CEC_pm_suspend(void);
void DMARB_mid_resume(void);
void DMARB_mid_suspend(void);
void DMX_pm_str_resume(void);
void DMX_pm_str_suspend(void);
INT32 EEPDTV_GetCfg(OUT_ONLY DTVCFG_T* prDtvCfg);
void GCPU_pm_str_resume(void);
void GCPU_pm_str_suspend(void);
void GDMA_pm_str_resume(void);
void GDMA_pm_str_suspend(void);
void GPIO_pm_str_resume(void);
void GPIO_pm_str_suspend(void);
void HDMI2_pm_resume(void);
void HDMI2_pm_suspend(void);
void Image_pm_str_resume(void);
void Image_pm_str_suspend(void);
void Imgrz_pm_str_resume(void);
void Imgrz_pm_str_suspend(void);
void IRRX_pm_resume(void);
void IRTX_pm_resume(void);
void IRTX_pm_suspend(void);
void MHL_pm_resume(void);
void MHL_pm_suspend(void);
UINT32 MID_QueryIntrusionStatus(void);
UINT32 MID_WarmBootRegionProtect(UINT32 onoff);
void PCMCIA_pm_str_resume(void);
void PCMCIA_pm_str_suspend(void);
void PDWNC_EnterSuspend(void);
UINT32 PDWNC_ReadWakeupReason(void);
void PDWNC_Reboot(void);
void PDWNC_Resume(void);
void PDWNC_SetSuspendMode(UINT32 SuspendMode);
void PMX_pm_resume(void);
void PMX_pm_suspend(void);
void PWM_pm_str_resume(void);
void PWM_pm_str_suspend(void);
void SIF_pm_str_resume(void);
void SIF_pm_str_suspend(void);
INT32 SMC_Deactivate_NoLock(UINT8 u1SlotNo);
void Tuner_pm_str_resume(void);
void Tuner_pm_str_suspend(void);
void TVD_pm_resume(void);
void TVD_pm_suspend(void);
void TVE_pm_resume(void);
void TVE_pm_suspend(void);
void VBI_pm_resume(void);
void VBI_pm_suspend(void);
void VDEC_pm_str_resume(void);
void VDEC_pm_str_suspend(void);
void vDrvDisplayInit_pm_resume(void);
void vDrvLVDSResume(void);
void vDrvLVDSSuspend(void);
void vDrvNptvResume(void);
void vDrvThermal_pm_resume(void);
void vDrvThermal_pm_suspend(void);
void vDrvVideoResume(void);
void vDrvVideoSuspend(void);
void vDrvVOPLLResume(void);
void vDrvVOPLLSuspend(void);
void VENC_pm_str_resume(void);
void VENC_pm_str_suspend(void);
void WDT_pm_resume(void);
void WDT_pm_suspend(void);
typedef struct
{
	UINT8 bMainSrc;
	UINT8 bSubSrc;
}BAPIVIDEOMAINSUBSRC_INFO;

typedef struct
{
	DTVCFG_T prDtvCfg;
	INT32 func_ret;
}RET_EEPDTV_GETCFG_INFO;

#endif

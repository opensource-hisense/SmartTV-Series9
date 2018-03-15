#ifndef KCU_GRAPHIC_H
#include "drvcust_if.h"
#include "pmx_if.h"
#include "pmx_drvif.h"
#include "fbm_drvif.h"
#include "drv_display.h"
#include "imgrz_if.h"

#define IN_ONLY
#define OUT_ONLY

#define KCU_GRAPHIC_H 1

#if 0
typedef unsigned int UINT32;
typedef unsigned short UINT16;
typedef unsigned char UINT8;
#ifndef INT32
typedef unsigned char BOOL;
#endif
#ifndef INT32
typedef signed int INT32;
#endif
#ifndef UCHAR
typedef unsigned long UPTR;
#endif

#ifndef UCHAR
typedef unsigned char UCHAR;
#endif
#ifndef UINT64
typedef unsigned long long  UINT64; 
#endif
#endif


UINT32 PANEL_GetPanelWidth(void);
UINT32 PANEL_GetPanelHeight(void);
void PANEL_SetPanelWidth(UINT32 u4Value);
void PANEL_SetPanelHeight(UINT32 u4Value);
UINT32 GRAPHMODE_GetPanelWidth(void);
UINT32 GRAPHMODE_GetPanelHeight(void);
UINT32 PANEL_GetControlWord(void);
UINT32 PANEL_GetControlWord2(void);
UINT8 fgIsOSDFromGFX(void);
UINT32 fgIsMJCToOSTG(void);
void vDrvSwitchOSDRoute(void);
UINT16 wDrvGetHsyncBp(void);
UINT8 u1GetFlipMirrorConfig(void);
VOID vDrvGetStartPosition(OSD_START_POS_T *pPosition);
void IMGRZ_Cfg_MMU_IO_Ex(UINT32 u4Which, BOOL MMU_Read, BOOL MMU_Write);
void IMGRZ_Flush(void);
void IMGRZ_Flush_Ex(UINT32 u4Which);
void IMGRZ_Lock(void);
void IMGRZ_Lock_Ex(UINT32 u4Which);
void IMGRZ_ReInit(void);
void IMGRZ_ReInit_Ex(UINT32 u4Which);
void IMGRZ_Scale(IN_ONLY void *pvSclParam);
void IMGRZ_Scale_Ex(UINT32 u4Which, IN_ONLY void *pvSclParam);
void IMGRZ_SetScaleOpt(E_RZ_INOUT_TYPE_T eInOutType);
void IMGRZ_SetScaleOpt_Ex(UINT32 u4Which, E_RZ_INOUT_TYPE_T eInOutType);
void IMGRZ_Unlock(void);
void IMGRZ_Unlock_Ex(UINT32 u4Which);
void IMGRZ_Wait(void);
void IMGRZ_Wait_Ex(UINT32 u4Which);
void IMG_LockHwResource(UCHAR ucImgId);
void IMG_UnLockHwResource(UCHAR ucImgId);
UINT32 BSP_GetFbmMemAddr(void);
INT32 DRVCUST_OptQueryUptr(QUERY_UPTR_T eQryType, OUT_ONLY UPTR *pu4Data);
FBM_POOL_T* FBM_GetPoolInfo(UCHAR ucPoolType);
void FBM_Init(void);
UINT32 GetHidKbCountry(void);
void SetHidKbCountry(UINT32 u4Country);
UINT32 VDP_SetEnable(UCHAR ucVdpId, UCHAR ucEnable);

/***********************************************************
*******  mt53_fb_gpu.c using
************************************************************/
FBM_PIC_HDR_T* FBM_GetFrameBufferPicHdr(UCHAR ucFbgId, UCHAR ucFbId);
FBM_SEQ_HDR_T* FBM_GetFrameBufferSeqHdr(UCHAR ucFbgId);
BOOL FBM_GetFbByDispTag(UINT64 u4Tag, OUT_ONLY UCHAR *pucFbgId, OUT_ONLY UCHAR *pucFbId);
void FBM_GetFrameBufferAddr(UCHAR ucFbgId, UCHAR ucFbId, OUT_ONLY UPTR *pu4AddrY, OUT_ONLY UPTR *pu4AddrC);
void FBM_GetFrameBufferAddrExt(UCHAR ucFbgId, UCHAR ucFbId, OUT_ONLY UPTR *pu4AddrYExt, OUT_ONLY UPTR *pu4AddrCExt);
UINT32 PANEL_GetCurrentPanelHeightCM(void);
UINT32 PANEL_GetCurrentPanelWidthCM(void);

/***********************************************************
*******  mt53_fb.c using
************************************************************/
UINT32 PMX_SetPlaneOrderArray(IN_ONLY const UINT32 *pu4PlaneOrder);
INT32 drv_extmjc_init(VOID);
void PMX_Init(void);

typedef struct
{
    UINT32 u4Which;
     BOOL MMU_Read;
     BOOL MMU_Write;
}IMGRZ_CFG_MMU_IO_EX_INFO;

typedef struct
{
    UINT32 u4Which;
    UINT32 pvSclParamArr[256];
}IMGRZ_SCALE_EX_INFO;

typedef struct
{
    UINT32 u4Which;
    E_RZ_INOUT_TYPE_T eInOutType;
}IMGRZ_SETSCALEOPT_EX_INFO;

typedef struct
{
    UPTR pu4Data;
    INT32 func_ret;
}RET_DRVCUST_OPTQUERYUPTR_INFO;
typedef struct
{
	UINT32 pu4Data;
	INT32 func_ret;
}RET_DRVCUST_OPTQUERY_INFO;

/***********************************************************
*******  mt53_fb_dfb.c using
************************************************************/
typedef struct
{
    UCHAR ucVdpId;
    UCHAR ucEnable;
}VDP_SETENABLE_INFO;

typedef struct
{
    BOOL func_ret;
    UCHAR pucFbgId;
    UCHAR pucFbId;
} RET_FBM_GETFBBYDISPTAG_INFO;

typedef struct
{
    UCHAR ucFbgId;
    UCHAR ucFbId;
}FBM_GETFRAMEBUFFERADDR_INFO;

typedef struct
{
    BOOL func_ret;
    UPTR pu4AddrY;
    UPTR pu4AddrC;
}RET_FBM_GETFRAMEBUFFERADDR_INFO;

#endif

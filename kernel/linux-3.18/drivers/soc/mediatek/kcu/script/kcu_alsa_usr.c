#include "kcu.h"
unsigned long kcu_AUD_InitALSAPlayback_MixSnd(void * pvArg)
{
	unsigned long ret=0;
	UINT8 * parg=(UINT8 *)pvArg;
	AUD_InitALSAPlayback_MixSnd(*parg);
	return ret;
}
unsigned long kcu_AUD_DeInitALSAPlayback_MixSnd(void * pvArg)
{
	unsigned long ret=0;
	UINT8 * parg=(UINT8 *)pvArg;
	AUD_DeInitALSAPlayback_MixSnd(*parg);
	return ret;
}
unsigned long kcu_AUD_GetMixSndFIFOStart(void * pvArg)
{
	unsigned long ret=0;
	UINT8 * parg=(UINT8 *)pvArg;
	ret=AUD_GetMixSndFIFOStart(*parg);
	return ret;
}
int mtk_alsa_aud_kcu_register()
{
	int ret=0;
	ret=kcu_register_nfyid(NFY_AUD_INITALSAPLAYBACK_MIXSND,1,kcu_AUD_InitALSAPlayback_MixSnd,NULL);
	if(ret)return ret;
	ret=kcu_register_nfyid(NFY_AUD_DEINITALSAPLAYBACK_MIXSND,1,kcu_AUD_DeInitALSAPlayback_MixSnd,NULL);
	if(ret)return ret;
	ret=kcu_register_nfyid(NFY_AUD_GETMIXSNDFIFOSTART,1,kcu_AUD_GetMixSndFIFOStart,NULL);
	if(ret)return ret;
	return ret;
}

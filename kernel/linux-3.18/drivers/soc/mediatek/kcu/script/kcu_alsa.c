#include <linux/soc/mediatek/kcu.h>
void AUD_InitALSAPlayback_MixSnd(UINT8 u1StreamId)
{
	int ret;
	UINT8 arg=u1StreamId;
	ret=kcu_notify_sync(NFY_AUD_INITALSAPLAYBACK_MIXSND,&arg,sizeof(arg),NULL);
	if(ret!=0)
		printk("call nfy_id %d fails,ret=%d",NFY_AUD_INITALSAPLAYBACK_MIXSND,ret);
}
void AUD_DeInitALSAPlayback_MixSnd(UINT8 u1StreamId)
{
	int ret;
	UINT8 arg=u1StreamId;
	ret=kcu_notify_sync(NFY_AUD_DEINITALSAPLAYBACK_MIXSND,&arg,sizeof(arg),NULL);
	if(ret!=0)
		printk("call nfy_id %d fails,ret=%d",NFY_AUD_DEINITALSAPLAYBACK_MIXSND,ret);
}
UPTR AUD_GetMixSndFIFOStart(UINT8 u1StreamId)
{
	int ret;
	UINT8 arg=u1StreamId;
	unsigned long func_ret=0;
	ret=kcu_notify_sync(NFY_AUD_GETMIXSNDFIFOSTART,&arg,sizeof(arg),&func_ret);
	if(ret!=0)
		printk("call nfy_id %d fails,ret=%d",NFY_AUD_GETMIXSNDFIFOSTART,ret);
	return func_ret;
}

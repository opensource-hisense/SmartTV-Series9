#include <linux/soc/mediatek/kcu.h>
#include <linux/soc/mediatek/mtk_alsa_aud.h>

void AUD_InitALSAPlayback_MixSnd(UINT8 u1StreamId)
{
	int ret;
	ret=kcu_notify_sync(NFY_AUD_INITALSAPLAYBACK_MIXSND,&u1StreamId,sizeof(u1StreamId),NULL,0);
	if(ret!=0)
	{
		printk("call nfy_id %d fails,ret=%d\n",NFY_AUD_INITALSAPLAYBACK_MIXSND,ret);
	}
}
EXPORT_SYMBOL(AUD_InitALSAPlayback_MixSnd);

void AUD_DeInitALSAPlayback_MixSnd(UINT8 u1StreamId)
{
	int ret;
	ret=kcu_notify_sync(NFY_AUD_DEINITALSAPLAYBACK_MIXSND,&u1StreamId,sizeof(u1StreamId),NULL,0);
	if(ret!=0)
	{
		printk("call nfy_id %d fails,ret=%d\n",NFY_AUD_DEINITALSAPLAYBACK_MIXSND,ret);
	}
}
EXPORT_SYMBOL(AUD_DeInitALSAPlayback_MixSnd);

void AUD_SetDSPDecoderType(AUD_FMT_T u4DspDecType)
{
	int ret;
	ret=kcu_notify_sync(NFY_AUD_SETDSPDECODERTYPE,&u4DspDecType,sizeof(u4DspDecType),NULL,0);
	if(ret!=0)
	{
		printk("call nfy_id %d fails,ret=%d\n",NFY_AUD_SETDSPDECODERTYPE,ret);
	}
}
EXPORT_SYMBOL(AUD_SetDSPDecoderType);

void AUD_SetDSPSampleRate(SAMPLE_FREQ_T u4DspSamplingRate)
{
	int ret;
	ret=kcu_notify_sync(NFY_AUD_SETDSPSAMPLERATE,&u4DspSamplingRate,sizeof(u4DspSamplingRate),NULL,0);
	if(ret!=0)
	{
		printk("call nfy_id %d fails,ret=%d\n",NFY_AUD_SETDSPSAMPLERATE,ret);
	}
}
EXPORT_SYMBOL(AUD_SetDSPSampleRate);

void AUD_SetDSPChannelNum(UINT8 u1DspChanNum)
{
	int ret;
	ret=kcu_notify_sync(NFY_AUD_SETDSPCHANNELNUM,&u1DspChanNum,sizeof(u1DspChanNum),NULL,0);
	if(ret!=0)
	{
		printk("call nfy_id %d fails,ret=%d\n",NFY_AUD_SETDSPCHANNELNUM,ret);
	}
}
EXPORT_SYMBOL(AUD_SetDSPChannelNum);

void AUD_SetDSPDecoderPause()
{
	int ret;
	ret=kcu_notify_sync(NFY_AUD_SETDSPDECODERPAUSE,NULL,0,NULL,0);
	if(ret!=0)
	{
		printk("call nfy_id %d fails,ret=%d\n",NFY_AUD_SETDSPDECODERPAUSE,ret);
	}
}
EXPORT_SYMBOL(AUD_SetDSPDecoderPause);

void AUD_SetDSPDecoderResume()
{
	int ret;
	ret=kcu_notify_sync(NFY_AUD_SETDSPDECODERRESUME,NULL,0,NULL,0);
	if(ret!=0)
	{
		printk("call nfy_id %d fails,ret=%d\n",NFY_AUD_SETDSPDECODERRESUME,ret);
	}
}
EXPORT_SYMBOL(AUD_SetDSPDecoderResume);

void AUD_FlushAFIFO()
{
	int ret;
	ret=kcu_notify_sync(NFY_AUD_FLUSHAFIFO,NULL,0,NULL,0);
	if(ret!=0)
	{
		printk("call nfy_id %d fails,ret=%d\n",NFY_AUD_FLUSHAFIFO,ret);
	}
}
EXPORT_SYMBOL(AUD_FlushAFIFO);

UINT32 AUD_GetMixSndFIFOStart(UINT8 u1StreamId)
{
	int ret;
	UINT32 func_ret;
	ret=kcu_notify_sync(NFY_AUD_GETMIXSNDFIFOSTART,&u1StreamId,sizeof(u1StreamId),&func_ret,sizeof(func_ret));
	if(ret!=0)
	{
		printk("call nfy_id %d fails,ret=%d\n",NFY_AUD_GETMIXSNDFIFOSTART,ret);
	}
	return func_ret;
}
EXPORT_SYMBOL(AUD_GetMixSndFIFOStart);

UINT32 AUD_GetMixSndFIFOEnd(UINT8 u1StreamId)
{
	int ret;
	UINT32 func_ret;
	ret=kcu_notify_sync(NFY_AUD_GETMIXSNDFIFOEND,&u1StreamId,sizeof(u1StreamId),&func_ret,sizeof(func_ret));
	if(ret!=0)
	{
		printk("call nfy_id %d fails,ret=%d\n",NFY_AUD_GETMIXSNDFIFOEND,ret);
	}
	return func_ret;
}
EXPORT_SYMBOL(AUD_GetMixSndFIFOEnd);

UINT32 AUD_GetMixSndReadPtr(UINT8 u1StreamId)
{
	int ret;
	UINT32 func_ret;
	ret=kcu_notify_sync(NFY_AUD_GETMIXSNDREADPTR,&u1StreamId,sizeof(u1StreamId),&func_ret,sizeof(func_ret));
	if(ret!=0)
	{
		printk("call nfy_id %d fails,ret=%d\n",NFY_AUD_GETMIXSNDREADPTR,ret);
	}
	return func_ret;
}
EXPORT_SYMBOL(AUD_GetMixSndReadPtr);

void AUD_SetMixSndWritePtr(UINT8 u1StreamId, UINT32 u4WritePtr)
{
	int ret;
	AUD_SETMIXSNDWRITEPTR_INFO input_arg=
	{
	u1StreamId,
	u4WritePtr
	};
	ret=kcu_notify_sync(NFY_AUD_SETMIXSNDWRITEPTR,&input_arg,sizeof(input_arg),NULL,0);
	if(ret!=0)
	{
		printk("call nfy_id %d fails,ret=%d\n",NFY_AUD_SETMIXSNDWRITEPTR,ret);
	}
}
EXPORT_SYMBOL(AUD_SetMixSndWritePtr);

void AUD_PlayMixSndRingFifo(UINT8 u1StreamId, UINT32 u4SampleRate, UINT8 u1StereoOnOff, UINT8 u1BitDepth, UINT32 u4BufferSize)
{
	int ret;
	AUD_PLAYMIXSNDRINGFIFO_INFO input_arg=
	{
	u1StreamId,
	u4SampleRate,
	u1StereoOnOff,
	u1BitDepth,
	u4BufferSize
	};
	ret=kcu_notify_sync(NFY_AUD_PLAYMIXSNDRINGFIFO,&input_arg,sizeof(input_arg),NULL,0);
	if(ret!=0)
	{
		printk("call nfy_id %d fails,ret=%d\n",NFY_AUD_PLAYMIXSNDRINGFIFO,ret);
	}
}
EXPORT_SYMBOL(AUD_PlayMixSndRingFifo);

void AUD_FlushDram(UINT32 u4Addr, UINT32 u4Len)
{
	int ret;
	AUD_FLUSHDRAM_INFO input_arg=
	{
	u4Addr,
	u4Len
	};
	ret=kcu_notify_sync(NFY_AUD_FLUSHDRAM,&input_arg,sizeof(input_arg),NULL,0);
	if(ret!=0)
	{
		printk("call nfy_id %d fails,ret=%d\n",NFY_AUD_FLUSHDRAM,ret);
	}
}
EXPORT_SYMBOL(AUD_FlushDram);

void AUD_InitALSARecordSpeaker()
{
	int ret;
	ret=kcu_notify_sync(NFY_AUD_INITALSARECORDSPEAKER,NULL,0,NULL,0);
	if(ret!=0)
	{
		printk("call nfy_id %d fails,ret=%d\n",NFY_AUD_INITALSARECORDSPEAKER,ret);
	}
}
EXPORT_SYMBOL(AUD_InitALSARecordSpeaker);

void AUD_DeInitALSARecordSpeaker()
{
	int ret;
	ret=kcu_notify_sync(NFY_AUD_DEINITALSARECORDSPEAKER,NULL,0,NULL,0);
	if(ret!=0)
	{
		printk("call nfy_id %d fails,ret=%d\n",NFY_AUD_DEINITALSARECORDSPEAKER,ret);
	}
}
EXPORT_SYMBOL(AUD_DeInitALSARecordSpeaker);

UINT32 AUD_GetUploadFIFOStart()
{
	int ret;
	UINT32 func_ret;
	ret=kcu_notify_sync(NFY_AUD_GETUPLOADFIFOSTART,NULL,0,&func_ret,sizeof(func_ret));
	if(ret!=0)
	{
		printk("call nfy_id %d fails,ret=%d\n",NFY_AUD_GETUPLOADFIFOSTART,ret);
	}
	return func_ret;
}
EXPORT_SYMBOL(AUD_GetUploadFIFOStart);

UINT32 AUD_GetUploadFIFOEnd()
{
	int ret;
	UINT32 func_ret;
	ret=kcu_notify_sync(NFY_AUD_GETUPLOADFIFOEND,NULL,0,&func_ret,sizeof(func_ret));
	if(ret!=0)
	{
		printk("call nfy_id %d fails,ret=%d\n",NFY_AUD_GETUPLOADFIFOEND,ret);
	}
	return func_ret;
}
EXPORT_SYMBOL(AUD_GetUploadFIFOEnd);

UINT32 AUD_GetUploadWritePnt()
{
	int ret;
	UINT32 func_ret;
	ret=kcu_notify_sync(NFY_AUD_GETUPLOADWRITEPNT,NULL,0,&func_ret,sizeof(func_ret));
	if(ret!=0)
	{
		printk("call nfy_id %d fails,ret=%d\n",NFY_AUD_GETUPLOADWRITEPNT,ret);
	}
	return func_ret;
}
EXPORT_SYMBOL(AUD_GetUploadWritePnt);

BOOL isHpPlugIn()
{
	int ret;
	BOOL func_ret;
	ret=kcu_notify_sync(NFY_ISHPPLUGIN,NULL,0,&func_ret,sizeof(func_ret));
	if(ret!=0)
	{
		printk("call nfy_id %d fails,ret=%d\n",NFY_ISHPPLUGIN,ret);
	}
	return func_ret;
}
EXPORT_SYMBOL(isHpPlugIn);


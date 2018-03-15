#ifndef MTK_ALSA_AUD_H
#define MTK_ALSA_AUD_H 1

typedef unsigned int UINT32;
typedef unsigned short UINT16;
typedef unsigned char UINT8;
typedef signed int INT32;
typedef UINT8 BOOL;
typedef unsigned long UPTR;

typedef enum
{
    AUD_FMT_UNKNOWN = 0,
    AUD_FMT_MPEG,
    AUD_FMT_AC3,
    AUD_FMT_PCM,
    AUD_FMT_MP3,
    AUD_FMT_AAC,
    AUD_FMT_DTS,
    AUD_FMT_WMA,
    AUD_FMT_RA,
    AUD_FMT_HDCD,
    AUD_FMT_MLP,     // 10
    AUD_FMT_MTS,
    AUD_FMT_EU_CANAL_PLUS,
    AUD_FMT_PAL,
    AUD_FMT_A2,
    AUD_FMT_FMFM,
    AUD_FMT_NICAM,
    AUD_FMT_TTXAAC,
    AUD_FMT_DETECTOR,
    AUD_FMT_MINER,
    AUD_FMT_LPCM,       // 20
    AUD_FMT_FMRDO,
    AUD_FMT_FMRDO_DET,
    AUD_FMT_SBCDEC,
    AUD_FMT_SBCENC,     // 24,
    AUD_FMT_MP3ENC,     // 25, MP3ENC_SUPPORT
    AUD_FMT_G711DEC,    // 26
    AUD_FMT_G711ENC,    // 27
    AUD_FMT_DRA,        // 28
    AUD_FMT_COOK,        // 29
    AUD_FMT_G729DEC,     // 30
    AUD_FMT_VORBISDEC,    //31, CC_VORBIS_SUPPORT
    AUD_FMT_WMAPRO,    //32  please sync number with middleware\res_mngr\drv\x_aud_dec.h
    AUD_FMT_HE_AAC,     //33 Terry Added for S1 UI notification
    AUD_FMT_HE_AAC_V2,  //34
    AUD_FMT_AMR,        //35 amr-nb run in DSP
    AUD_FMT_AWB,        //36 amr-wb run in DSP
    AUD_FMT_APE,        //37 //ian APE decoder
    AUD_FMT_FLAC = 39,  //39, paul_flac
    AUD_FMT_G726 = 40,  //40, G726 decoder

    AUD_FMT_TV_SYS = 63, //63, sync from x_aud_dec.h
    AUD_FMT_OMX_MSADPCM = 0x80,
    AUD_FMT_OMX_IMAADPCM = 0x81,
    AUD_FMT_OMX_ALAW = 0x82,
    AUD_FMT_OMX_ULAW = 0x83,
}   AUD_FMT_T;

typedef enum
{
    FS_16K = 0x00,
    FS_22K,
    FS_24K,
    FS_32K,
    FS_44K,
    FS_48K,
    FS_64K,
    FS_88K,
    FS_96K,
    FS_176K,
    FS_192K,
    FS_8K, // appended since 09/10/2007, don't change the order
    FS_11K, // appended since 09/10/2007, don't change the order
    FS_12K, // appended since 09/10/2007, don't change the order
    FS_52K, // appended since 24/02/2010, don't change the order
    FS_56K,
    FS_62K,  // appended since 24/02/2010, don't change the order
    FS_64K_SRC,
    FS_6K,
    FS_10K,
    FS_5K
}   SAMPLE_FREQ_T;


void AUD_InitALSAPlayback_MixSnd(UINT8 u1StreamId);
void AUD_DeInitALSAPlayback_MixSnd(UINT8 u1StreamId);
void AUD_SetDSPDecoderType(AUD_FMT_T u4DspDecType);
void AUD_SetDSPSampleRate(SAMPLE_FREQ_T u4DspSamplingRate);
void AUD_SetDSPChannelNum(UINT8 u1DspChanNum);
void AUD_SetDSPDecoderPause(void);
void AUD_SetDSPDecoderResume(void);
void AUD_FlushAFIFO(void);
UINT32 AUD_GetMixSndFIFOStart(UINT8 u1StreamId);
UINT32 AUD_GetMixSndFIFOEnd(UINT8 u1StreamId);
UINT32 AUD_GetMixSndReadPtr(UINT8 u1StreamId);
void AUD_SetMixSndWritePtr(UINT8 u1StreamId, UINT32 u4WritePtr);
void AUD_PlayMixSndRingFifo(UINT8 u1StreamId, UINT32 u4SampleRate, UINT8 u1StereoOnOff, UINT8 u1BitDepth, UINT32 u4BufferSize);
void AUD_FlushDram(UINT32 u4Addr, UINT32 u4Len);
void AUD_InitALSARecordSpeaker(void);
void AUD_DeInitALSARecordSpeaker(void);
UINT32 AUD_GetUploadFIFOStart(void);
UINT32 AUD_GetUploadFIFOEnd(void);
UINT32 AUD_GetUploadWritePnt(void);
BOOL isHpPlugIn(void);



typedef struct
{
	UINT8 u1StreamId;
	UINT32 u4WritePtr;
}AUD_SETMIXSNDWRITEPTR_INFO;

typedef struct
{
	UINT8 u1StreamId;
	UINT32 u4SampleRate;
	UINT8 u1StereoOnOff;
	UINT8 u1BitDepth;
	UINT32 u4BufferSize;
}AUD_PLAYMIXSNDRINGFIFO_INFO;

typedef struct
{
	UINT32 u4Addr;
	UINT32 u4Len;
}AUD_FLUSHDRAM_INFO;

#endif

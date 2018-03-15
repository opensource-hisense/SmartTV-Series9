#ifndef MTK_ALSA_AUD_H
#define MTK_ALSA_AUD_H 1

typedef unsigned int UINT32;
typedef unsigned short UINT16;
typedef unsigned char UINT8;
typedef signed int INT32;
typedef bool BOOL;
typedef unsigned long UPTR;

void AUD_InitALSAPlayback_MixSnd(UINT8 u1StreamId);
void AUD_DeInitALSAPlayback_MixSnd(UINT8 u1StreamId,UINT8 u1StreamId2);
void AUD_InitALSAPlayback_MixSnd2();
UPTR AUD_GetMixSndReadPtr(UINT8 u1StreamId);

#endif

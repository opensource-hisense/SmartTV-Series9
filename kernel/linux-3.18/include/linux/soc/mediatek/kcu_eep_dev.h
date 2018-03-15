#ifndef EEP_DEV_KCU_H
#define EEP_DEV_KCU_H 1
//INT32 EEPROM_GetSize(void);
//INT16 EEPROM_Init(VOID);
//INT32 EEPROM_Read(UINT64 u8Offset, OUT_ONLY UPTR * u4MemPtr, UINT32 u4MemLen);
//INT32 EEPROM_Write(UINT64 u8Offset,IN_ONLY UPTR* u4MemPtr, UINT32 u4MemLen);
#ifdef __KERNEL__
typedef  int32_t INT32;
typedef  int16_t INT16;
typedef uint32_t UINT32;
typedef uint64_t UINT64;
#endif
typedef struct
{
	UINT64 u8Offset;
	UINT32 u4MemLen;
}EEPROM_READ_INFO;

typedef struct
{
	UINT64 u8Offset;
	UINT32 u4MemLen;
}EEPROM_WRITE_INFO;
#endif

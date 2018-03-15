#ifndef MTK_QMU_H
#define MTK_QMU_H

#include "mu3d_hal_osal.h"
#include "mu3d_hal_hw.h"
#include "mu3d_hal_comm.h"
#include "mu3d_hal_usb_drv.h"

#define MAX_SUB_SPD_NUM 25

typedef struct _TGPD {
    DEV_UINT8	flag;
    DEV_UINT8	chksum;
    DEV_UINT16	DataBufferLen; /*Rx Allow Length*/
    struct _TGPD*	pNext;
    void*			pBuf;
    DEV_UINT16	bufLen;
    DEV_UINT8	ExtLength;
    DEV_UINT8	ZTepFlag;
}TGPD, *PGPD;

typedef struct _TBD {
    DEV_UINT8  flag;
    DEV_UINT8  chksum;
    DEV_UINT16  DataBufferLen; /*Rx Allow Length*/  
    struct _TBD *pNext;
    DEV_UINT8 *pBuf;
    DEV_UINT16 bufLen;    
    DEV_UINT8  extLen;
    DEV_UINT8  reserved;
}TBD, *PBD;

typedef struct _GPD_RANGE {	
    PGPD	pNext;
    PGPD	pStart;
    PGPD	pEnd;
}GPD_R, *RGPD;

typedef struct _BD_RANGE {	
    PBD	pNext;
    PBD	pStart;
    PBD	pEnd;
}BD_R, *RBD;

typedef enum{
	GPD_NORMAL	= 0,
	GPD_SPD1	= 1,
	GPD_SPD2	= 2,
	GPD_RSV
}GPD_TYPE;


typedef struct {
	DEV_UINT16 payload_sz;
	DEV_UINT8 spd2_hdrsz;
	unsigned seq: 4;
	unsigned rsv: 2;
	unsigned eol: 1;
	unsigned igr: 1;
}TSPD_EXT_BODY, *PSPD_EXT_BODY;

typedef struct {
	DEV_UINT16 entry_nr;
	DEV_UINT8 spd1_hdr;
	DEV_UINT8 hdr_room;
}TSPD_EXT_HDR;

typedef enum {
	NON_ERR = 0,
	ZERO_PKTNUM,		//para_pkt_num equals 0
	PKTNUM_MISMATCH_ENTRY,	// para_pkt_num != atcual entry_nr, no longer err
	IGR_AND_EOL,	//igr and eol set simultaneously
	ZERO_LOAD_AND_EOL	//payload == 0 and eol set
}SPD_ERR_TYPE;

typedef struct {
	unsigned zlp: 1;
	unsigned enh: 1;
	unsigned padding: 1;
	unsigned err_type: 3;
	unsigned rsv: 2;
}__attribute__((packed)) FLAG_OF_SPD_ENH;

/*
 * err_para meanings:
 * @PKTNUM_MISMATCH_ENTRY: =actual entry_nr - para_pkt_num
*/
typedef struct USB_SPD_SCAN{
	u8 spd1_nr;
	u8 spd2_nr;
	u8 gpd_nr;
	s8 err_para;	//only valid when flag.err_type is non-zero
	u32 head_len;
	u32 payload_len;	
	u8 hdr_room;
	FLAG_OF_SPD_ENH flag;
}SPD_SCAN_INFO;

typedef enum{
	NON_ERROR = 0,
	NFSPD_WITH_RXGPD, //FSPD(First Entry SPD)->NFSPD(Non-First Entry SPD)->GPD->NFSPD->NFSPD->FSPD
	NFSPD_WITH_BPSGPD, //FSPD-NFSPD-GPD(BPS)-NFSPD-FSPD(IOC)
	NFSPD_WITH_BPSSPD, //FSPD-NFSPD-SPD(BPS)-NFSPD-FSPD(IOC)
	FSPD_LEN_ERR      //FSPD(with allowlen=maxp) - NFSPD-NFSPD-NFSPD-FSPD(IOC)
}RX_SPD_ERR_TYPE;




typedef struct USB_SPD_RX_SCAN{
	u8 zlp;
	u8 test_nr;
	u16 rx_spd_min_size;
	u16 gpd_buf_size;	//only valid when flag.err_type is non-zero
	u16 head_len;
	u32 payload_len;	
	u8 rsv;
	u8 err_type;
	s8 err_para;
}SPD_RX_SCAN_INFO;

typedef struct {
	u16 payload;
	u8 spd2_hdr;
	unsigned seq_num: 4;
	unsigned rsv: 2;
	unsigned eol: 1;
	unsigned igr: 1;	
}SPD_ENTRY_PARA;

typedef struct {
	unsigned bps: 1;
	unsigned ioc: 1;
	unsigned pdt: 2;
	unsigned enh: 1;
	unsigned zlp: 1;	
	unsigned rsv: 2;
	u16 transfer_len;
	u16 pkt_num;
	u8 spd1_hdr;
	u8 hdr_room;
	u8 rsv1;
	u16 rsv2;
	SPD_ENTRY_PARA entry[0];
}GPD_PARA,*pGPD_PARA;



typedef struct {
	u16 spd_rx_len;
	unsigned bps: 1;
	unsigned ioc: 1;
	unsigned host_skip_flag: 1;
	unsigned break_flag: 1;
	unsigned rsv: 4;
}RX_SUB_SPD;

typedef struct {
	unsigned pdt: 2;
	unsigned zlp: 1;
	unsigned flag: 5;
	u16 allow_len;
	u8 skp_type;//type=1:allow len < min size; type=2:allow len < transfer len; type = 3:transfer len > min size ; L
	u8 spd_num;
	u16 skp_xfer_len;
	u16 rsv;
	u16 extension;
	RX_SUB_SPD sub_spd[MAX_SUB_SPD_NUM];
}SPD_RX_PARA, *pSPD_RX_PARA;

typedef struct {
	u8 hdr_room;
	u8 padding;
	u8 zlp;
	u8 ehn;
}TSPD_FLAG_INFO;

#if 0
typedef struct {
	u16 payload;
	u8 hdr;
	unsigned pkt_seq: 5;
	unsigned type: 2;
	unsigned igr: 1;
}SPD_ENTRY_INFO;
#else
//entry format will not include payload size
//device will calculate payload by urb_len minus hdr_sz
//urb_len will exclude padding-0 len
typedef struct {
	u8 hdr_room;
	u8 hdr;		//this field means extension size when type == GPD_NORMAL
	u8 pkt_seq;
	unsigned type: 2;
	unsigned enh: 1;
	unsigned igr: 1;
	unsigned bps: 1;
	unsigned ioc: 1;
	unsigned rsv: 2;
}SPD_ENTRY_INFO;
#endif

#ifdef CAP_QMU_SPD
#define AT_GPD_EXT_LEN					4080
#else
#define AT_GPD_EXT_LEN					256
#endif

#define AT_BD_EXT_LEN 					256
#ifdef CAP_QMU_SPD
#define MAX_GPD_NUM						30	//bobo
#else
#define MAX_GPD_NUM						45	//bobo
#endif
//#define MAX_TX_GPD_NUM						30	//bobo
//#define MAX_RX_GPD_NUM						30	//bobo
#define MAX_BD_NUM						500
#define STRESS_IOC_TH 					8
#define STRESS_GPD_TH 					24
#define RANDOM_STOP_DELAY 				80
#define STRESS_DATA_LENGTH 				1024*64//1024*16
#define MAX_SPD						20	//bobo

#define MAX_QMU_EP 						4




#define TGPD_FLAGS_HWO              	0x01
#define TGPD_IS_FLAGS_HWO(_pd)      	(((TGPD *)_pd)->flag & TGPD_FLAGS_HWO)
#define TGPD_SET_FLAGS_HWO(_pd)     	(((TGPD *)_pd)->flag |= TGPD_FLAGS_HWO)
#define TGPD_CLR_FLAGS_HWO(_pd)     	(((TGPD *)_pd)->flag &= (~TGPD_FLAGS_HWO))
#define TGPD_FORMAT_BDP             	0x02
#define TGPD_IS_FORMAT_BDP(_pd)     	(((TGPD *)_pd)->flag & TGPD_FORMAT_BDP)
#define TGPD_SET_FORMAT_BDP(_pd)    	(((TGPD *)_pd)->flag |= TGPD_FORMAT_BDP)
#define TGPD_CLR_FORMAT_BDP(_pd)    	(((TGPD *)_pd)->flag &= (~TGPD_FORMAT_BDP))
#define TGPD_FORMAT_BPS             	0x04
#define TGPD_IS_FORMAT_BPS(_pd)     	(((TGPD *)_pd)->flag & TGPD_FORMAT_BPS)
#define TGPD_SET_FORMAT_BPS(_pd)    	(((TGPD *)_pd)->flag |= TGPD_FORMAT_BPS)
#define TGPD_CLR_FORMAT_BPS(_pd)    	(((TGPD *)_pd)->flag &= (~TGPD_FORMAT_BPS))
#define TGPD_SET_FLAG(_pd, _flag)   	((TGPD *)_pd)->flag = (((TGPD *)_pd)->flag&(~TGPD_FLAGS_HWO))|(_flag)
#define TGPD_GET_FLAG(_pd)             	(((TGPD *)_pd)->flag & TGPD_FLAGS_HWO)
#define TGPD_SET_CHKSUM(_pd, _n)    	((TGPD *)_pd)->chksum = mu3d_hal_cal_checksum((DEV_UINT8 *)_pd, _n)-1
#define TGPD_SET_CHKSUM_HWO(_pd, _n)    ((TGPD *)_pd)->chksum = mu3d_hal_cal_checksum((DEV_UINT8 *)_pd, _n)-1
#define TGPD_GET_CHKSUM(_pd)        	((TGPD *)_pd)->chksum
#define TGPD_SET_FORMAT(_pd, _fmt)  	((TGPD *)_pd)->flag = (((TGPD *)_pd)->flag&(~TGPD_FORMAT_BDP))|(_fmt)
#define TGPD_GET_FORMAT(_pd)        	((((TGPD *)_pd)->flag & TGPD_FORMAT_BDP)>>1)
#define TGPD_SET_DataBUF_LEN(_pd, _len) ((TGPD *)_pd)->DataBufferLen = _len
#define TGPD_ADD_DataBUF_LEN(_pd, _len) ((TGPD *)_pd)->DataBufferLen += _len
#define TGPD_GET_DataBUF_LEN(_pd)       ((TGPD *)_pd)->DataBufferLen
#define TGPD_SET_NEXT(_pd, _next)   	((TGPD *)_pd)->pNext = (TGPD *)_next;
#define TGPD_GET_NEXT(_pd)          	(TGPD *)((TGPD *)_pd)->pNext
#define TGPD_SET_TBD(_pd, _tbd)     	((TGPD *)_pd)->pBuf = (DEV_UINT8 *)_tbd;\
                                    	TGPD_SET_FORMAT_BDP(_pd)
#define TGPD_GET_TBD(_pd)           	(TBD *)((TGPD *)_pd)->pBuf
#define TGPD_SET_DATA(_pd, _data)   	((TGPD *)_pd)->pBuf = (DEV_UINT8 *)_data
#define TGPD_GET_DATA(_pd)          	(void*)((TGPD *)_pd)->pBuf
#define TGPD_SET_BUF_LEN(_pd, _len) 	((TGPD *)_pd)->bufLen = _len
#define TGPD_ADD_BUF_LEN(_pd, _len) 	((TGPD *)_pd)->bufLen += _len
#define TGPD_GET_BUF_LEN(_pd)       	((TGPD *)_pd)->bufLen
#define TGPD_SET_EXT_LEN(_pd, _len) 	((TGPD *)_pd)->ExtLength = _len
#define TGPD_GET_EXT_LEN(_pd)        	((TGPD *)_pd)->ExtLength
#define TGPD_SET_EPaddr(_pd, _EP)  		((TGPD *)_pd)->ZTepFlag =(((TGPD *)_pd)->ZTepFlag&0xF0)|(_EP)
#define TGPD_GET_EPaddr(_pd)        	((TGPD *)_pd)->ZTepFlag & 0x0F 
#define TGPD_FORMAT_TGL             	0x10
#define TGPD_IS_FORMAT_TGL(_pd)     	(((TGPD *)_pd)->ZTepFlag & TGPD_FORMAT_TGL)
#define TGPD_SET_FORMAT_TGL(_pd)    	(((TGPD *)_pd)->ZTepFlag |=TGPD_FORMAT_TGL)
#define TGPD_CLR_FORMAT_TGL(_pd)    	(((TGPD *)_pd)->ZTepFlag &= (~TGPD_FORMAT_TGL))
#define TGPD_FORMAT_ZLP             	0x20
#define TGPD_IS_FORMAT_ZLP(_pd)     	(((TGPD *)_pd)->ZTepFlag & TGPD_FORMAT_ZLP)
#define TGPD_SET_FORMAT_ZLP(_pd)    	(((TGPD *)_pd)->ZTepFlag |=TGPD_FORMAT_ZLP)
#define TGPD_CLR_FORMAT_ZLP(_pd)    	(((TGPD *)_pd)->ZTepFlag &= (~TGPD_FORMAT_ZLP))
#define TGPD_FORMAT_IOC             	0x80
#define TGPD_IS_FORMAT_IOC(_pd)     	(((TGPD *)_pd)->flag & TGPD_FORMAT_IOC)
#define TGPD_SET_FORMAT_IOC(_pd)    	(((TGPD *)_pd)->flag |=TGPD_FORMAT_IOC) 
#define TGPD_CLR_FORMAT_IOC(_pd)    	(((TGPD *)_pd)->flag &= (~TGPD_FORMAT_IOC))
#define TGPD_SET_TGL(_pd, _TGL)  		((TGPD *)_pd)->ZTepFlag |=(( _TGL) ? 0x10: 0x00)
#define TGPD_GET_TGL(_pd)        		((TGPD *)_pd)->ZTepFlag & 0x10 ? 1:0
#define TGPD_SET_ZLP(_pd, _ZLP)  		((TGPD *)_pd)->ZTepFlag |= ((_ZLP) ? 0x20: 0x00)
#define TGPD_GET_ZLP(_pd)        		((TGPD *)_pd)->ZTepFlag & 0x20 ? 1:0
#define TGPD_GET_EXT(_pd)           	((DEV_UINT8 *)_pd + sizeof(TGPD))

//SPD usage
#define TGPD_FORMAT_PDT					(0x3<<3)
#define TGPD_FORMAT_ENH_SPD				(0x1<<5)
#define TGPD_FORMAT_PDT_MASK					(0x3)
#define TGPD_FORMAT_ENH_SPD_MASK			(0x5)
#define TGPD_FORMAT_SKP_SPD				(0x1<<6)
#define TGPD_FORMAT_SKP_SPD_MASK			(0x6)
#define TGPD_SET_ENH_SPD(_pd)			(((TGPD *)_pd)->flag |= TGPD_FORMAT_ENH_SPD)
#define TGPD_CLR_ENH_SPD(_pd)			(((TGPD *)_pd)->flag &= (~TGPD_FORMAT_ENH_SPD))
#define TGPD_GET_ENH_SPD(_pd)			((((TGPD *)_pd)->flag & TGPD_FORMAT_ENH_SPD)>>TGPD_FORMAT_ENH_SPD_MASK)
#define TGPD_GET_SKP_SPD(_pd)			((((TGPD *)_pd)->flag & TGPD_FORMAT_SKP_SPD)>>TGPD_FORMAT_SKP_SPD_MASK)
#define TGPD_CLR_FORMAT_SKP_SPD(_pd)    	(((TGPD *)_pd)->ZTepFlag &= (~TGPD_FORMAT_SKP_SPD))
#define TGPD_SET_RX_SPD_RSV(_pd, _len) 	((TGPD *)_pd)->ZTepFlag = _len
#define TGPD_GET_RX_SPD_RSV(_pd) 	((TGPD *)_pd)->ZTepFlag 

#define TGPD_GET_FORMAT_PDT(_pd)			((((TGPD *)_pd)->flag & TGPD_FORMAT_PDT)>>TGPD_FORMAT_PDT_MASK)

static inline void TGPD_SET_FORMAT_PDT(TGPD * _pd, DEV_UINT8 _pdt)	
{
	//os_printk(K_DEBUG, "pdt=%d\t", _pdt);
	_pd->flag &= (~TGPD_FORMAT_PDT); 
	_pd->flag |= (_pdt<<TGPD_FORMAT_PDT_MASK);
}


#define TBD_FLAGS_EOL               	0x01
#define TBD_IS_FLAGS_EOL(_bd)       	(((TBD *)_bd)->flag & TBD_FLAGS_EOL)
#define TBD_SET_FLAGS_EOL(_bd)      	(((TBD *)_bd)->flag |= TBD_FLAGS_EOL)
#define TBD_CLR_FLAGS_EOL(_bd)      	(((TBD *)_bd)->flag &= (~TBD_FLAGS_EOL))
#define TBD_SET_FLAG(_bd, _flag)    	((TBD *)_bd)->flag = (DEV_UINT8)_flag
#define TBD_GET_FLAG(_bd)           	((TBD *)_bd)->flag
#define TBD_SET_CHKSUM(_pd, _n)     	((TBD *)_pd)->chksum = mu3d_hal_cal_checksum((DEV_UINT8 *)_pd, _n)
#define TBD_GET_CHKSUM(_pd)         	((TBD *)_pd)->chksum
#define TBD_SET_DataBUF_LEN(_pd, _len) 	((TBD *)_pd)->DataBufferLen = _len
#define TBD_GET_DataBUF_LEN(_pd)       	((TBD *)_pd)->DataBufferLen
#define TBD_SET_NEXT(_bd, _next)    	((TBD *)_bd)->pNext = (TBD *)_next
#define TBD_GET_NEXT(_bd)           	(TBD *)((TBD *)_bd)->pNext
#define TBD_SET_DATA(_bd, _data)    	((TBD *)_bd)->pBuf = (DEV_UINT8 *)_data
#define TBD_GET_DATA(_bd)           	(DEV_UINT8*)((TBD *)_bd)->pBuf
#define TBD_SET_BUF_LEN(_bd, _len)  	((TBD *)_bd)->bufLen = _len
#define TBD_ADD_BUF_LEN(_bd, _len)  	((TBD *)_bd)->bufLen += _len
#define TBD_GET_BUF_LEN(_bd)        	((TBD *)_bd)->bufLen
#define TBD_SET_EXT_LEN(_bd, _len)  	((TBD *)_bd)->extLen = _len
#define TBD_ADD_EXT_LEN(_bd, _len)  	((TBD *)_bd)->extLen += _len
#define TBD_GET_EXT_LEN(_bd)        	((TBD *)_bd)->extLen
#define TBD_GET_EXT(_bd)            	((DEV_UINT8 *)_bd + sizeof(TBD))



#undef EXTERN

#ifdef _MTK_QMU_DRV_EXT_
#define EXTERN 
#else 
#define EXTERN extern
#endif


EXTERN DEV_UINT8 is_bdp;
EXTERN DEV_UINT32 gpd_buf_size;
EXTERN DEV_UINT16 bd_buf_size;
EXTERN DEV_UINT8 bBD_Extension;
EXTERN DEV_UINT8 bGPD_Extension;
EXTERN DEV_UINT32 g_dma_buffer_size;
EXTERN PGPD Rx_gpd_head[15];
EXTERN PGPD Tx_gpd_head[15];
EXTERN PGPD Rx_gpd_end[15];
EXTERN PGPD Tx_gpd_end[15];
EXTERN PGPD Rx_gpd_last[15];
EXTERN PGPD Tx_gpd_last[15];
EXTERN GPD_R Rx_gpd_List[15];
EXTERN GPD_R Tx_gpd_List[15];
EXTERN BD_R Rx_bd_List[15];
EXTERN BD_R Tx_bd_List[15];
EXTERN DEV_UINT32 Rx_gpd_Offset[15];
EXTERN DEV_UINT32 Tx_gpd_Offset[15];
EXTERN DEV_UINT32 Rx_bd_Offset[15];
EXTERN DEV_UINT32 Tx_bd_Offset[15];
EXTERN char g_usb_dir[2][4];
EXTERN char g_usb_type[4][5];
EXTERN char g_u1u2_type[4][10];
EXTERN char g_u1u2_opt[7][10];
EXTERN DEV_UINT8 spd_tx_err;
EXTERN DEV_UINT8 RX_SPD_SKP;
EXTERN DEV_UINT8 RX_SPD_OVERSZ;
EXTERN DEV_UINT16 RX_SKP_XFE_LEN;


EXTERN void mu3d_hal_resume_qmu(DEV_INT32 Q_num,  USB_DIR dir);
EXTERN void mu3d_hal_stop_qmu(DEV_INT32 Q_num,  USB_DIR dir);
EXTERN TGPD* mu3d_hal_prepare_tx_gpd(TGPD* gpd, dma_addr_t pBuf, DEV_UINT32 data_length, DEV_UINT8 ep_num, DEV_UINT8 _is_bdp, DEV_UINT8 isHWO,DEV_UINT8 ioc, DEV_UINT8 bps,DEV_UINT8 zlp);
EXTERN TGPD* mu3d_hal_prepare_rx_gpd(TGPD*gpd, dma_addr_t pBuf, DEV_UINT32 data_len, DEV_UINT8 ep_num, DEV_UINT8 _is_bdp, DEV_UINT8 isHWO, DEV_UINT8 ioc, DEV_UINT8 bps,DEV_UINT32 cMaxPacketSize);
EXTERN void mu3d_hal_insert_transfer_gpd(DEV_INT32 ep_num,USB_DIR dir, dma_addr_t buf, DEV_UINT32 count, DEV_UINT8 isHWO, DEV_UINT8 ioc, DEV_UINT8 bps,DEV_UINT8 zlp, DEV_UINT32 cMaxPacketSize );
EXTERN void mu3d_hal_alloc_qmu_mem(void);
EXTERN void mu3d_hal_init_qmu(void);
EXTERN void mu3d_hal_start_qmu(DEV_INT32 Q_num,  USB_DIR dir);
EXTERN void mu3d_hal_flush_qmu(DEV_INT32 Q_num,  USB_DIR dir);
EXTERN void mu3d_hal_restart_qmu(DEV_INT32 Q_num,  USB_DIR dir);
EXTERN void mu3d_hal_send_stall(DEV_INT32 Q_num,  USB_DIR dir);
EXTERN DEV_UINT8 mu3d_hal_cal_checksum(DEV_UINT8 *data, DEV_INT32 len);
EXTERN void *mu3d_hal_gpd_virt_to_phys(void *vaddr,USB_DIR dir,DEV_UINT32 num);
#ifdef CAP_QMU_SPD
EXTERN void dev_spd_scan(SPD_SCAN_INFO *spd_scan_info, DEV_UINT8 tx, DEV_UINT8 rx);
EXTERN void dev_spd_tx_loopback(SPD_SCAN_INFO *spd_scan_info, DEV_UINT8 tx_ep, DEV_UINT8 rx_ep);
EXTERN DEV_UINT32 mu3d_hal_insert_spd(TSPD_FLAG_INFO *spd_flag, void *data, DEV_UINT8 ep_num, void *buffer[], u16 payload[]);
EXTERN DEV_UINT32 mu3d_hal_insert_spd_tx(void *data, void *rxbuf,DEV_UINT8 ep_num, pGPD_PARA para,DEV_UINT8 padding);
#endif
#ifdef RX_QMU_SPD
EXTERN void dev_spd_rx_loopback(SPD_RX_SCAN_INFO *spd_scan_info, DEV_UINT8 tx_ep, DEV_UINT8 rx_ep);
EXTERN void dev_spd_rx_err_test(SPD_RX_SCAN_INFO *spd_scan_info, DEV_UINT8 tx_ep, DEV_UINT8 rx_ep);
EXTERN DEV_UINT32 mu3d_hal_insert_spd_rx(DEV_INT32 ep_num,dma_addr_t buf, pSPD_RX_PARA para,DEV_UINT16 allow_len,DEV_UINT8 rsv,DEV_UINT32 cMaxPacketSize);
#endif

#undef EXTERN

#endif


#include "../mu3d_hal/mu3d_hal_osal.h"
#include "../mu3d_hal/mu3d_hal_qmu_drv.h"
#define _QMU_DRV_EXT_
#include "mu3d_test_qmu_drv.h"
#undef _QMU_DRV_EXT_
#include "mu3d_test_usb_drv.h"
#include "../mu3d_hal/mu3d_hal_hw.h"
#include "mu3d_test_unified.h"


#define TX_FIFO_NUM	 5     /* including ep0 */
#define RX_FIFO_NUM 5     /* including ep0 */
DEV_UINT32 gpd_num;
DEV_UINT8* qmu_loopback_buffer;
DEV_UINT8* stress_dma_buffer=0;
DEV_UINT8* stress_dma_buffer_end=0;
#ifdef _STRESS_ADD_BYPASS
static DEV_UINT8 _fgbypassGPD = 0;
#endif

DEV_UINT8 Rx_gpd_IOC[15];
spinlock_t	lock;


void qmu_proc(DEV_UINT32 wQmuVal);
void set_gpd_hwo(USB_DIR dir,DEV_INT32 Q_num);
void resume_gpd_hwo(USB_DIR dir,DEV_INT32 Q_num);



void dev_insert_stress_gpd(USB_DIR dir,DEV_INT32 ep_num,DEV_UINT8 IOC){
    struct USB_REQ *req;
    DEV_UINT32  maxp;
    DEV_UINT8 zlp;

    req = mu3d_hal_get_req(ep_num, dir);
    zlp = (USB_ReadCsr32(U3D_TX1CSR1, ep_num) & TYPE_ISO) ? false : true;
    maxp = USB_ReadCsr32(U3D_RX1CSR0, ep_num) & RX_RXMAXPKTSZ;

#ifdef _STRESS_ADD_BYPASS
    _fgbypassGPD++;
    if(!(_fgbypassGPD%5)){
        mu3d_hal_insert_transfer_gpd(ep_num, dir, (void *)req->dma_adr, req->count, true, IOC, true, zlp, maxp);
    }
    else
#endif
    {
        mu3d_hal_insert_transfer_gpd(ep_num,dir, req->dma_adr, req->count, true, IOC, false, zlp, maxp);

        if(req->dma_adr >= (dma_addr_t)stress_dma_buffer_end){
            req->dma_adr = (dma_addr_t)stress_dma_buffer;
        }
        else{
            req->dma_adr += TransferLength;
        }
    }

}


/**
 * dev_resume_stress_gpd_hwo - write h/w own bit and then resume gpd.
 * 
 */
void dev_resume_stress_gpd_hwo(USB_DIR dir,DEV_INT32 ep_num){

    resume_gpd_hwo(dir,ep_num);
    mu3d_hal_resume_qmu(ep_num, dir);
}

/**
 * dev_resume_stress_gpd_hwo - insert gpd only with writing h/w own bit.
 * 
 */
void dev_insert_stress_gpd_hwo(USB_DIR dir,DEV_INT32 ep_num){

    set_gpd_hwo(dir,ep_num);
    mu3d_hal_resume_qmu(ep_num, dir);
}


void dev_start_stress(USB_DIR dir,DEV_INT32 ep_num){
    mu3d_hal_resume_qmu(ep_num, dir);
}


/**
 * dev_prepare_gpd_short - prepare tx or rx gpd for qmu
 * @args - arg1: gpd number, arg2: dir, arg3: ep number, arg4: data buffer
 */
void dev_prepare_gpd_short(DEV_INT32 num,USB_DIR dir,DEV_INT32 ep_num,DEV_UINT8* buf){
    struct USB_REQ *req;
    DEV_UINT32 i, maxp;
    dma_addr_t mapping;
    DEV_UINT8 IOC, zlp;

    os_printk(K_ERR,"%s\r\n", __func__);
    os_printk(K_ERR,"buf[%d] :%p\r\n",ep_num,buf);
    os_printk(K_ERR,"dir :%x\r\n",dir);

    req = mu3d_hal_get_req(ep_num, dir);
    req->buf = buf;
    req->count=TransferLength;
    os_memset(req->buf, 0xFF , g_dma_buffer_size);
    mapping = dma_map_single(NULL, req->buf,g_dma_buffer_size, DMA_BIDIRECTIONAL);
    dma_sync_single_for_device(NULL, mapping, g_dma_buffer_size, DMA_BIDIRECTIONAL);
    req->dma_adr = mapping;
    stress_dma_buffer = (void *)req->dma_adr;
    stress_dma_buffer_end = stress_dma_buffer+(MAX_GPD_NUM-1)*TransferLength;
    zlp = (USB_ReadCsr32(U3D_TX1CSR1, ep_num) & TYPE_ISO) ? false : true;
    maxp = USB_ReadCsr32(U3D_RX1CSR0, ep_num) & RX_RXMAXPKTSZ;


    if(dir==USB_TX){

        mu3d_hal_insert_transfer_gpd(ep_num,dir, req->dma_adr, 256, true, false, false, zlp, maxp);

        req->dma_adr += TransferLength;
        for(i=1;i<num;i++){
            IOC = ((i%STRESS_IOC_TH)==(STRESS_IOC_TH/2)) ? true : false;
            mu3d_hal_insert_transfer_gpd(ep_num,dir, req->dma_adr, req->count, true, IOC, false, zlp, maxp);

            if(req->dma_adr >= (u32)stress_dma_buffer_end){
                req->dma_adr = (u32)stress_dma_buffer;
            }
            else{
                req->dma_adr += TransferLength;
            }
        }
    }
    else{
        for(i=0;i<num;i++){
            IOC = ((i%STRESS_IOC_TH)==(STRESS_IOC_TH/2)) ? true : false;
            mu3d_hal_insert_transfer_gpd(ep_num,dir, req->dma_adr, req->count, true, IOC, false, zlp, maxp);

            if(req->dma_adr >= (u32)stress_dma_buffer_end){
                req->dma_adr = (u32)stress_dma_buffer;
            }
            else{
                req->dma_adr += TransferLength;
            }
        }
    }
    dma_sync_single_for_cpu(NULL,mapping,g_dma_buffer_size,DMA_BIDIRECTIONAL);
    dma_unmap_single(NULL, mapping, g_dma_buffer_size,DMA_BIDIRECTIONAL);

    if(dir==USB_TX)
        os_printk(K_ERR,"TX[%d] GPD Start : 0x%08X\n", ep_num, os_readl(USB_QMU_TQSAR(ep_num)));
    else
        os_printk(K_ERR,"RX[%d] GPD Start : 0x%08X\n", ep_num, os_readl(USB_QMU_RQSAR(ep_num)));

}


/**
 * dev_prepare_gpd - prepare tx or rx gpd for qmu
 * @args - arg1: gpd number, arg2: dir, arg3: ep number, arg4: data buffer
 */
void dev_prepare_gpd(DEV_INT32 num,USB_DIR dir,DEV_INT32 ep_num,DEV_UINT8* buf){
    struct USB_REQ *req;
    DEV_UINT32 i, maxp;
    dma_addr_t mapping;
    DEV_UINT8 IOC, zlp;

    os_printk(K_ERR,"%s\r\n", __func__);
    os_printk(K_ERR,"buf[%d] :%p\r\n",ep_num,buf);
    os_printk(K_ERR,"dir :%x\r\n",dir);

    req = mu3d_hal_get_req(ep_num, dir);
    req->buf = buf;
    req->count=TransferLength;
    os_memset(req->buf, 0xFF , g_dma_buffer_size);
    mapping = dma_map_single(NULL, req->buf,g_dma_buffer_size, DMA_BIDIRECTIONAL);
    dma_sync_single_for_device(NULL, mapping, g_dma_buffer_size, DMA_BIDIRECTIONAL);
    req->dma_adr = mapping;
    stress_dma_buffer = (void *)req->dma_adr;
    stress_dma_buffer_end = stress_dma_buffer+(MAX_GPD_NUM-1)*TransferLength;
    zlp = (USB_ReadCsr32(U3D_TX1CSR1, ep_num) & TYPE_ISO) ? false : true;
    maxp = USB_ReadCsr32(U3D_RX1CSR0, ep_num) & RX_RXMAXPKTSZ;

    for(i=0;i<num;i++){
        IOC = ((i%STRESS_IOC_TH)==(STRESS_IOC_TH/2)) ? true : false;
        mu3d_hal_insert_transfer_gpd(ep_num,dir, req->dma_adr, req->count, true, IOC, false, zlp, maxp);

        if(req->dma_adr >= (u32)stress_dma_buffer_end){
            req->dma_adr = (u32)stress_dma_buffer;
        }
        else{
            req->dma_adr += TransferLength;
        }
    }
    dma_sync_single_for_cpu(NULL,mapping,g_dma_buffer_size,DMA_BIDIRECTIONAL);
    dma_unmap_single(NULL, mapping, g_dma_buffer_size,DMA_BIDIRECTIONAL);

    if(dir==USB_TX)
        os_printk(K_ERR,"TX[%d] GPD Start : 0x%08X\n", ep_num, os_readl(USB_QMU_TQSAR(ep_num)));
    else
        os_printk(K_ERR,"RX[%d] GPD Start : 0x%08X\n", ep_num, os_readl(USB_QMU_RQSAR(ep_num)));

}


/**
 * dev_prepare_gpd - prepare tx or rx gpd for qmu stress test
 * @args - arg1: gpd number, arg2: dir, arg3: ep number, arg4: data buffer
 */
void dev_prepare_stress_gpd(DEV_INT32 num,USB_DIR dir,DEV_INT32 ep_num,DEV_UINT8* buf){
    struct USB_REQ *req;
    DEV_UINT32 i;
    dma_addr_t mapping;
    DEV_UINT8 IOC;

    os_printk(K_ERR,"dev_prepare_gpd\r\n");
    os_printk(K_ERR,"buf[%d] :%p\r\n",ep_num,buf);
    os_printk(K_ERR,"dir :%x\r\n",dir);

    req = mu3d_hal_get_req(ep_num, dir);
    req->buf = buf;
    req->count=TransferLength;

    mapping = dma_map_single(NULL, req->buf,g_dma_buffer_size, DMA_BIDIRECTIONAL);
    dma_sync_single_for_device(NULL, mapping, g_dma_buffer_size, DMA_BIDIRECTIONAL);
    req->dma_adr = mapping;

    os_ms_delay(100);
    stress_dma_buffer = (void *)req->dma_adr;
    stress_dma_buffer_end = stress_dma_buffer+(num-1)*TransferLength;

    for(i=0;i<num;i++){

        if(dir==USB_RX){
            IOC = (i==(num-1)) ? true : false;
        }
        insert_stress_gpd(ep_num,dir, req->dma_adr, req->count, false, IOC);

        if(req->dma_adr >= (u32)stress_dma_buffer_end){
            req->dma_adr = (u32)stress_dma_buffer;
        }
        else{
            req->dma_adr += TransferLength;
        }
    }
    Rx_gpd_IOC[ep_num] =0;
    if(dir==USB_TX)
        os_printk(K_ERR,"TX[%d] GPD Start : 0x%08X\n", ep_num, os_readl(USB_QMU_TQSAR(ep_num)));
    else
        os_printk(K_ERR,"RX[%d] GPD Start : 0x%08X\n", ep_num, os_readl(USB_QMU_RQSAR(ep_num)));

}


/**
 * dev_qmu_loopback_ext - loopback scan test 
 * @args - arg1: rx ep number, arg2: tx ep number
 */
void dev_qmu_loopback_ext(DEV_INT32 ep_rx,DEV_INT32 ep_tx){

    struct USB_REQ *treq, *rreq;
    dma_addr_t mapping;
    DEV_UINT32 i, j, gpd_num,bps_num, maxp;
    DEV_UINT8* dma_buf, zlp;

    os_printk(K_INFO,"ep_rx :%d\r\n",ep_rx);
    os_printk(K_INFO,"ep_tx :%d\r\n",ep_tx);
    rreq = mu3d_hal_get_req(ep_rx, USB_RX);
    treq = mu3d_hal_get_req(ep_tx, USB_TX);

    dma_buf = g_loopback_buffer[0];
#ifdef BOUNDARY_4K		
    qmu_loopback_buffer = g_loopback_buffer[0]+(0x1000-(DEV_INT32)g_loopback_buffer[0]%0x1000)-0x20+bDramOffset;
    treq->buf =rreq->buf =qmu_loopback_buffer;
#else
    treq->buf =rreq->buf =g_loopback_buffer[0];
#endif

    os_printk(K_INFO,"rreq->buf :%p\r\n",rreq->buf);
    os_printk(K_INFO,"treq->buf :%p\r\n",treq->buf);

    treq->count = rreq->count = TransferLength;//64512;
    rreq->transferCount=0;
    rreq->complete=0;
    treq->complete=0;
    os_memset(dma_buf, 0 , g_dma_buffer_size);
    mapping = dma_map_single(NULL, dma_buf,g_dma_buffer_size, DMA_BIDIRECTIONAL);
    dma_sync_single_for_device(NULL, mapping, g_dma_buffer_size, DMA_BIDIRECTIONAL);
    treq->dma_adr = rreq->dma_adr = mapping;
    gpd_num = (TransferLength/gpd_buf_size);
    if(TransferLength%gpd_buf_size){
        gpd_num++;
    }

    os_printk(K_NOTICE,"gpd_num :%x\r\n",gpd_num);
    zlp = (USB_ReadCsr32(U3D_TX1CSR1, ep_tx) & TYPE_ISO) ? false : true;
    maxp = USB_ReadCsr32(U3D_RX1CSR0, ep_rx) & RX_RXMAXPKTSZ;

    for(i=0 ;i<gpd_num ; i++){

        mu3d_hal_insert_transfer_gpd(ep_rx,USB_RX, (rreq->dma_adr+i*gpd_buf_size), gpd_buf_size, true, true, false, zlp, maxp);
        os_get_random_bytes(&bps_num,1);
        bps_num %= 3;
        for(j=0 ;j<bps_num ;j++){
            mu3d_hal_insert_transfer_gpd(ep_rx,USB_RX, rreq->dma_adr, gpd_buf_size, true,true, true, zlp, maxp);
        }
    }

    os_printk(K_WARNIN,"rx start\r\n");
    mu3d_hal_resume_qmu(ep_rx, USB_RX);
    while(!req_complete(ep_rx, USB_RX));
    os_printk(K_WARNIN, "rx done!Q cnt=%d\n", g_rxq_done_cnt);
    treq->count=TransferLength;
    gpd_num = (TransferLength/gpd_buf_size);

    if(TransferLength%gpd_buf_size){
        gpd_num++;
    }
    for(i=0 ;i<gpd_num ; i++){

        if(treq->count>gpd_buf_size){
            mu3d_hal_insert_transfer_gpd(ep_tx,USB_TX, (treq->dma_adr+i*gpd_buf_size), gpd_buf_size, true, true, false, zlp, maxp);
        }
        else{
            mu3d_hal_insert_transfer_gpd(ep_tx,USB_TX, (treq->dma_adr+i*gpd_buf_size),treq->count , true, true, false, zlp, maxp);
        }

        os_get_random_bytes(&bps_num,1);
        bps_num %= 3;

        for(j=0 ;j<bps_num ;j++){
            mu3d_hal_insert_transfer_gpd(ep_tx,USB_TX, treq->dma_adr, gpd_buf_size, true,true, true, zlp, maxp);
        }
        treq->count-= gpd_buf_size;
    }
    g_u3d_status = READY;

	#if !ISO_UPDATE_TEST
    mu3d_hal_resume_qmu(ep_tx, USB_TX);
    os_printk(K_WARNIN,"tx start...length : %d\r\n",TransferLength);
    while(!req_complete(ep_tx, USB_TX));
    os_printk(K_WARNIN, "tx done!Q cnt=%d\n", g_txq_done_cnt);
	#endif

    mapping = rreq->dma_adr;
    dma_sync_single_for_cpu(NULL,mapping,g_dma_buffer_size,DMA_BIDIRECTIONAL);
    dma_unmap_single(NULL, mapping, g_dma_buffer_size,DMA_BIDIRECTIONAL);

}
/**
 * sw_erdy - sw send an ERDY packet
 * @ ep_num: target endpoint number
 * @ dir: OUT -- 0; IN -- 1; EP0 must be 0
 */

void mu3d_hal_sw_erdy(DEV_UINT8 ep_num, DEV_UINT8 dir)
{
    DEV_UINT32 reg;
    if(!ep_num)
        dir = 0;
    reg = (ep_num << SW_ERDY_EP_NUM_OFST) | (dir << SW_ERDY_EP_DIR_OFST);
    os_writel(U3D_USB3_SW_ERDY, reg);
    os_setmsk(U3D_USB3_SW_ERDY, SW_SEND_ERDY);
}

/**
 * dev_notification - send device notification packet
 * @args - arg1: notification type, arg2: notification value low,ep number,  arg3: notification value high
 */
void dev_notification(DEV_INT8 type,DEV_INT32 valuel,DEV_INT32 valueh){
    DEV_INT32 temp;

    temp = ((type<<4)&DEV_NOTIF_TYPE)|((valuel<<8)&DEV_NOTIF_TYPE_SPECIFIC_LOW);
    os_writel(U3D_DEV_NOTIF_0, temp);
    temp = ((valuel>>24)&0xFF)|((valueh<<8)&0xFFFFFF00);
    os_writel(U3D_DEV_NOTIF_1, temp);
    os_writel(U3D_DEV_NOTIF_0, os_readl(U3D_DEV_NOTIF_0)|SEND_DEV_NOTIF);
    while(os_readl(U3D_DEV_NOTIF_0)&SEND_DEV_NOTIF);
}

/*
 * otg_dev - otg test cases
 * @args - mode: otg test mode
 */
void dev_otg(DEV_UINT8 mode)
{
    printk("1: iddig_a\n");
    printk("2: iddig_a\n");
    printk("3: srp_a\n");
    printk("4: srp_b\n");
    printk("5: hnp_a\n");
    printk("6: hnp_b\n");
}


/**
 * dev_qmu_rx - qmu rx test
 * @args - arg1: rx ep number
 */
void dev_qmu_rx(DEV_INT32 ep_rx){
    struct USB_REQ *req;
    dma_addr_t mapping;
    DEV_UINT32 i, gpd_num, QCR, maxp;
    DEV_UINT8 zlp;

    req = mu3d_hal_get_req(ep_rx, USB_RX);
    req->buf =g_loopback_buffer[0];
    req->count=TransferLength;;
    req->transferCount=0;
    req->complete=0;

    if(cfg_rx_zlp_en)
    {
        QCR = os_readl(U3D_QCR3);
        os_writel(U3D_QCR3, QCR|QMU_RX_ZLP(ep_rx));
    }
    else
    {
        QCR = os_readl(U3D_QCR3);
        os_writel(U3D_QCR3, QCR&~(QMU_RX_ZLP(ep_rx)));
    }

    if(cfg_rx_coz_en)
    {
        QCR = os_readl(U3D_QCR3);
        os_writel(U3D_QCR3, QCR|QMU_RX_COZ(ep_rx));
    }
    else
    {
        QCR = os_readl(U3D_QCR3);
        os_writel(U3D_QCR3, QCR&~(QMU_RX_COZ(ep_rx)));
    }


    os_memset(req->buf, 0 , 1000000);
    mapping = dma_map_single(NULL, req->buf,g_dma_buffer_size, DMA_BIDIRECTIONAL);
    dma_sync_single_for_device(NULL, mapping, g_dma_buffer_size, DMA_BIDIRECTIONAL);
    req->dma_adr = mapping;
    gpd_num = (TransferLength/gpd_buf_size);
    if(cfg_rx_coz_en){
        gpd_num*=2;
    }
    if(TransferLength%gpd_buf_size){
        gpd_num++;
    }
    zlp =  true;
    maxp = USB_ReadCsr32(U3D_RX1CSR0, ep_rx) & RX_RXMAXPKTSZ;
    for(i=0 ;i<gpd_num ; i++){
        mu3d_hal_insert_transfer_gpd(ep_rx,USB_RX, (req->dma_adr+i*gpd_buf_size), gpd_buf_size, true, true, false, zlp, maxp);
    }
    mu3d_hal_resume_qmu(ep_rx, USB_RX);
    os_ms_delay(500);
    rx_done_count=(os_readl(USB_QMU_RQCPR(ep_rx))-os_readl(USB_QMU_RQSAR(ep_rx)))/sizeof(TGPD);
    mapping = req->dma_adr;
    dma_sync_single_for_cpu(NULL,mapping,g_dma_buffer_size,DMA_BIDIRECTIONAL);
    dma_unmap_single(NULL, mapping, g_dma_buffer_size,DMA_BIDIRECTIONAL);

}

/**
 * dev_ep_reset - ep reset  test
 * @args - 
 */
DEV_UINT8 dev_ep_reset(void){

    DEV_UINT32 i, tx_ep_num, rx_ep_num, csr0, csr1, csr2;
    DEV_UINT8 ret;

    tx_ep_num = os_readl(U3D_CAP_EPINFO) & CAP_TX_EP_NUM;
    rx_ep_num = (os_readl(U3D_CAP_EPINFO) & CAP_RX_EP_NUM) >> 8;
    ret = 0;

    for(i=1; i<=tx_ep_num; i++){
        USB_WriteCsr32(U3D_TX1CSR0, i, USB_ReadCsr32(U3D_TX1CSR0, i) | TX_EP_RESET);
        USB_WriteCsr32(U3D_TX1CSR0, i, USB_ReadCsr32(U3D_TX1CSR0, i) &~ TX_EP_RESET);
        csr0 = USB_ReadCsr32(U3D_TX1CSR0, i);
        csr1 = USB_ReadCsr32(U3D_TX1CSR1, i);
        csr2 = USB_ReadCsr32(U3D_TX1CSR2, i);
        USB_WriteCsr32(U3D_TX1CSR0, i, 0x7FFFFFFF);
        USB_WriteCsr32(U3D_TX1CSR1, i, 0xFFFFFFFF);
        USB_WriteCsr32(U3D_TX1CSR2, i, 0xFFFFFFFF);

        if(csr0 == USB_ReadCsr32(U3D_TX1CSR0, i)){
            ret = ERROR;
        }

        if(csr1 == USB_ReadCsr32(U3D_TX1CSR1, i)){
            ret = ERROR;
        }
        if(csr2 == USB_ReadCsr32(U3D_TX1CSR2, i)){
            ret = ERROR;
        }
        USB_WriteCsr32(U3D_TX1CSR0, i, USB_ReadCsr32(U3D_TX1CSR0, i) | TX_EP_RESET);
        USB_WriteCsr32(U3D_TX1CSR0, i, USB_ReadCsr32(U3D_TX1CSR0, i) &~ TX_EP_RESET);
        if(csr0 != USB_ReadCsr32(U3D_TX1CSR0, i)){
            ret = ERROR;
        }

        if(csr1 != USB_ReadCsr32(U3D_TX1CSR1, i)){
            ret = ERROR;
        }
        if(csr2 != USB_ReadCsr32(U3D_TX1CSR2, i)){
            ret = ERROR;
        }
    }
    for(i=1; i<=rx_ep_num; i++){
        USB_WriteCsr32(U3D_RX1CSR0, i, USB_ReadCsr32(U3D_RX1CSR0, i) | RX_EP_RESET);
        USB_WriteCsr32(U3D_RX1CSR0, i, USB_ReadCsr32(U3D_RX1CSR0, i) &~ RX_EP_RESET);
        csr0 = USB_ReadCsr32(U3D_RX1CSR0, i);
        csr1 = USB_ReadCsr32(U3D_RX1CSR1, i);
        csr2 = USB_ReadCsr32(U3D_RX1CSR2, i);
        USB_WriteCsr32(U3D_RX1CSR0, i, 0x7FFFFFFF);
        USB_WriteCsr32(U3D_RX1CSR1, i, 0xFFFFFFFF);
        USB_WriteCsr32(U3D_RX1CSR2, i, 0xFFFFFFFF);
        if(csr0 == USB_ReadCsr32(U3D_RX1CSR0, i)){
            ret = ERROR;
        }
        if(csr1 == USB_ReadCsr32(U3D_RX1CSR1, i)){
            ret = ERROR;
        }
        if(csr2 == USB_ReadCsr32(U3D_RX1CSR2, i)){
            ret = ERROR;
        }
        USB_WriteCsr32(U3D_RX1CSR0, i, USB_ReadCsr32(U3D_RX1CSR0, i) | RX_EP_RESET);
        USB_WriteCsr32(U3D_RX1CSR0, i, USB_ReadCsr32(U3D_RX1CSR0, i) &~ RX_EP_RESET);


        if(csr0 != USB_ReadCsr32(U3D_RX1CSR0, i)){
            ret = ERROR;
        }

        if(csr1 != USB_ReadCsr32(U3D_RX1CSR1, i)){
            ret = ERROR;
        }
        if(csr2 != USB_ReadCsr32(U3D_RX1CSR2, i)){
            ret = ERROR;
        }
    }
    return ret;
}

/**
 * dev_tx_rx - tx/rx  test
 * @args - arg1: rx ep number, arg2: tx ep number
 */
void dev_tx_rx(DEV_INT32 ep_rx,DEV_INT32 ep_tx){

    struct USB_REQ *treq, *rreq;
    dma_addr_t mapping;
    DEV_UINT32 i, gpd_num, maxp;
    DEV_UINT8* dma_buf, zlp;

    os_printk(K_INFO,"ep_rx :%d\r\n",ep_rx);
    os_printk(K_INFO,"ep_tx :%d\r\n",ep_tx);

    rreq = mu3d_hal_get_req(ep_rx, USB_RX);
    treq = mu3d_hal_get_req(ep_tx, USB_TX);
    dma_buf = g_loopback_buffer[0];
#ifdef BOUNDARY_4K	
    qmu_loopback_buffer = g_loopback_buffer[0]+(0x1000-(DEV_INT32)g_loopback_buffer[0]%0x1000)-0x20+bDramOffset;
    treq->buf =rreq->buf =qmu_loopback_buffer;
#else
    treq->buf =rreq->buf =g_loopback_buffer[0];
#endif

    os_printk(K_INFO,"rreq->buf :%p\r\n",rreq->buf);
    os_printk(K_INFO,"treq->buf :%p\r\n",treq->buf);

    rreq->count=TransferLength;
    rreq->transferCount=0;
    rreq->complete=0;
    treq->complete=0;
    os_memset(rreq->buf, 0 , 1000000);
    mapping = dma_map_single(NULL, dma_buf,g_dma_buffer_size, DMA_BIDIRECTIONAL);
    dma_sync_single_for_device(NULL, mapping, g_dma_buffer_size, DMA_BIDIRECTIONAL);
    treq->dma_adr=rreq->dma_adr=mapping;

    gpd_num = (TransferLength/gpd_buf_size);
    if(TransferLength%gpd_buf_size){
        gpd_num++;
    }

    zlp = (USB_ReadCsr32(U3D_TX1CSR1, ep_tx) & TYPE_ISO) ? false : true;
    maxp = USB_ReadCsr32(U3D_RX1CSR0, ep_rx) & RX_RXMAXPKTSZ;

    for(i=0 ;i<gpd_num ; i++){
        mu3d_hal_insert_transfer_gpd(ep_rx,USB_RX, (rreq->dma_adr+i*gpd_buf_size), gpd_buf_size, true, true, false, zlp, maxp);
    }

    //no extension part, so it is OK to prepare GPD in advance
    //Must prepare GPD before flush QMU
    treq->count=TransferLength;
    for(i=0 ;i<gpd_num ; i++){

        os_printk(K_INFO,"treq->count : %d\r\n",treq->count);
        if(treq->count>gpd_buf_size){
            mu3d_hal_insert_transfer_gpd(ep_tx,USB_TX, (treq->dma_adr+i*gpd_buf_size), gpd_buf_size, true, true, false, zlp, maxp);
        }
        else{
            mu3d_hal_insert_transfer_gpd(ep_tx,USB_TX, (treq->dma_adr+i*gpd_buf_size),treq->count , true, true, false, zlp, maxp);
        }
        treq->count-= gpd_buf_size;
    }

    mu3d_hal_resume_qmu(ep_rx, USB_RX);
    os_printk(K_NOTICE,"rx start\r\n");
    while(!req_complete(ep_rx, USB_RX));
    os_printk(K_NOTICE,"rx complete...\r\n");

    treq->count=TransferLength;
    gpd_num = (TransferLength/gpd_buf_size);

    if(TransferLength%gpd_buf_size){
        gpd_num++;
    }

    mu3d_hal_resume_qmu(ep_tx, USB_TX);
    os_printk(K_NOTICE,"tx start\r\n");
    while(!req_complete(ep_tx, USB_TX));
    os_printk(K_NOTICE,"tx complete...\r\n");

    mapping = rreq->dma_adr;
    dma_sync_single_for_cpu(NULL,mapping,g_dma_buffer_size,DMA_BIDIRECTIONAL);
    dma_unmap_single(NULL, mapping, g_dma_buffer_size,DMA_BIDIRECTIONAL);
}

/**
 * dev_qmu_loopback - qmu loopback test
 * @args - arg1: dma burst, arg2: dma limiter
 */
void dev_set_dma_busrt_limiter(DEV_INT8 busrt,DEV_INT8 limiter){
    DEV_UINT32 temp;

    temp = ((busrt<<DMA_BURST_OFST)&DMA_BURST) | ((limiter<<DMALIMITER_OFST)&DMALIMITER);

    os_writel(U3D_TXDMARLCOUNT, os_readl(U3D_TXDMARLCOUNT)|temp);
    os_writel(U3D_RXDMARLCOUNT, os_readl(U3D_RXDMARLCOUNT)|temp);
}

/**
 * dev_qmu_loopback - qmu loopback test
 * @args - arg1: rx ep number, arg2: tx ep number
 */
void dev_qmu_loopback(DEV_INT32 ep_rx,DEV_INT32 ep_tx){
    struct USB_REQ *treq, *rreq;
    dma_addr_t mapping;
    DEV_UINT32 i, gpd_num, maxp;
    DEV_UINT8* dma_buf,zlp;

    os_printk(K_INFO,"ep_rx :%d\r\n",ep_rx);
    os_printk(K_INFO,"ep_tx :%d\r\n",ep_tx);

    rreq = mu3d_hal_get_req(ep_rx, USB_RX);
    treq = mu3d_hal_get_req(ep_tx, USB_TX);
    dma_buf = g_loopback_buffer[0];
#ifdef BOUNDARY_4K	
    qmu_loopback_buffer = g_loopback_buffer[0]+(0x1000-(DEV_INT32)g_loopback_buffer[0]%0x1000)-0x20+bDramOffset;
    treq->buf =rreq->buf =qmu_loopback_buffer;
#else
    treq->buf =rreq->buf =g_loopback_buffer[0];
#endif

    os_printk(K_INFO,"rreq->buf :%p\r\n",rreq->buf);
    os_printk(K_INFO,"treq->buf :%p\r\n",treq->buf);

    rreq->count=TransferLength;
    rreq->transferCount=0;
    rreq->complete=0;
    treq->complete=0;
    os_memset(rreq->buf, 0 , g_dma_buffer_size);
    mapping = dma_map_single(NULL, dma_buf,g_dma_buffer_size, DMA_BIDIRECTIONAL);
    dma_sync_single_for_device(NULL, mapping, g_dma_buffer_size, DMA_BIDIRECTIONAL);
    treq->dma_adr=rreq->dma_adr=mapping;

    gpd_num = (TransferLength/gpd_buf_size);
    if(TransferLength%gpd_buf_size){
        gpd_num++;
    }
    os_printk(K_ERR,"gpd_num:%d\r\n",gpd_num);

    zlp = (USB_ReadCsr32(U3D_TX1CSR1, ep_tx) & TYPE_ISO) ? false : true;
    maxp = USB_ReadCsr32(U3D_RX1CSR0, ep_rx) & RX_RXMAXPKTSZ;

    for(i=0 ;i<gpd_num ; i++){
        mu3d_hal_insert_transfer_gpd(ep_rx,USB_RX, (rreq->dma_adr+i*gpd_buf_size), gpd_buf_size, true, true, false, zlp, maxp);
    }

	#if LPM_STRESS
    os_ms_delay(1000);
	#endif
    mu3d_hal_resume_qmu(ep_rx, USB_RX);
    os_printk(K_WARNIN,"rx start\r\n");
    while(!req_complete(ep_rx, USB_RX));
    os_printk(K_WARNIN, "rx done!Q cnt=%d\n", g_rxq_done_cnt);


    treq->count=TransferLength;
    gpd_num = (TransferLength/gpd_buf_size);

    if(TransferLength%gpd_buf_size){
        gpd_num++;
    }

    for(i=0 ;i<gpd_num ; i++){

        os_printk(K_INFO,"treq->count : %d\r\n",treq->count);
        if(treq->count>gpd_buf_size){
            mu3d_hal_insert_transfer_gpd(ep_tx,USB_TX, (treq->dma_adr+i*gpd_buf_size), gpd_buf_size, true, true, false, zlp, maxp);
        }
        else{
            mu3d_hal_insert_transfer_gpd(ep_tx,USB_TX, (treq->dma_adr+i*gpd_buf_size),treq->count , true, true, false, zlp, maxp);
        }
        treq->count-= gpd_buf_size;
    }
    g_u3d_status = READY;

#if LPM_STRESS
    os_ms_delay(1000);
#endif
    os_setmsk(U3D_QCR4, SPD_EN(ep_tx));	//tmp add
    os_setmsk(U3D_QCR2, QMU_TX_ZLP(ep_tx));
    os_setmsk(U3D_QCR4, SPD2_PADDING_EN(ep_tx));
    os_printk(K_INFO, "U3D_QCR2=0x%08x, U3D_QCR4=0x%08x\n", \
              os_readl(U3D_QCR2), os_readl(U3D_QCR4));

#if !ISO_UPDATE_TEST	
    mu3d_hal_resume_qmu(ep_tx, USB_TX);
    os_printk(K_WARNIN,"tx start...length : %d\r\n",TransferLength);
    while(!req_complete(ep_tx, USB_TX));
    os_printk(K_WARNIN, "tx done!Q cnt=%d\n", g_txq_done_cnt);
#endif
    mapping = rreq->dma_adr;
    dma_sync_single_for_cpu(NULL,mapping,g_dma_buffer_size,DMA_BIDIRECTIONAL);
    dma_unmap_single(NULL, mapping, g_dma_buffer_size,DMA_BIDIRECTIONAL);

}


#ifdef RX_QMU_SPD
/**
 * dev_spd_loopback - spd loopback transfer 
 */
//static SPD_ERR_TYPE spd_force_err;
void dev_spd_rx_err_test(SPD_RX_SCAN_INFO *spd_scan_info, DEV_UINT8 tx_ep, DEV_UINT8 rx_ep)
{
    u16 test_nr;
    u8 zlp;
    struct USB_REQ *treq, *rreq;
    dma_addr_t mapping, tx_handle;
    DEV_UINT32 i, j,gpd_num, maxp, data_in_cnt;
    DEV_UINT8 *dma_buf, *payload_buf, *tx_buf, *spd_head_buf,*src;
    DEV_INT32 ep_num,QCR;
    RX_SUB_SPD *sub_spd;
    dma_addr_t req_buf;
    pSPD_RX_PARA para[MAX_SPD];
    u16 allow_len, rx_min_size,rsv_padding,spd_len;
    u8 break_idx,buf_num;
    TGPD *gpd;


    ep_num = tx_ep;
    test_nr = spd_scan_info->test_nr;
    zlp = spd_scan_info->zlp;
    allow_len = spd_scan_info->gpd_buf_size;
    rx_min_size = spd_scan_info->rx_spd_min_size;


    treq = mu3d_hal_get_req(tx_ep, USB_TX);
    rreq = mu3d_hal_get_req(rx_ep, USB_RX);
    dma_buf = g_loopback_buffer[0];
    payload_buf = g_loopback_buffer[1];
    treq->count = rreq->count = 0;
    treq->transferCount = rreq->transferCount = 0;
    treq->complete = rreq->complete = 0;
    os_memset(rreq->buf, 0 , g_dma_buffer_size);

    /*alloc memory*/
    mapping = dma_map_single(NULL, dma_buf,g_dma_buffer_size, DMA_BIDIRECTIONAL);
    dma_sync_single_for_device(NULL, mapping, g_dma_buffer_size, DMA_BIDIRECTIONAL);
    rreq->dma_adr = mapping;
    req_buf = rreq->dma_adr;

    tx_handle = dma_map_single(NULL, payload_buf,g_dma_buffer_size, DMA_BIDIRECTIONAL);
    dma_sync_single_for_cpu(NULL, tx_handle, g_dma_buffer_size, DMA_BIDIRECTIONAL);
    treq->dma_adr = tx_handle;

    spd_head_buf = os_mem_alloc(1024*4);
    tx_buf = payload_buf;

    /*init*/
    maxp = USB_ReadCsr32(U3D_RX1CSR0, rx_ep) & RX_RXMAXPKTSZ;
    i = 0;
    gpd_buf_size = 0x6000;
    is_bdp = bGPD_Extension = 0;
    g_u3d_status = BUSY;
    g_rxq_done_cnt = g_txq_done_cnt = 0;
    RX_SPD_SKP = 0;
    RX_SPD_OVERSZ = 0;
    RX_SKP_XFE_LEN =0;
    break_idx = 0xff;
    buf_num = 1;



    /*Get spd test pattern header info */
    os_printk(K_ERR, "Get spd test pattern header info\n");
    mu3d_hal_insert_transfer_gpd(rx_ep, USB_RX, req_buf, spd_scan_info->head_len, \
                                 true, true, false, zlp, maxp);
    mu3d_hal_resume_qmu(rx_ep, USB_RX);
    while(!req_complete(rx_ep, USB_RX));
    dma_sync_single_for_cpu(NULL, req_buf, rreq->transferCount, DMA_BIDIRECTIONAL);
    if(rreq->transferCount != spd_scan_info->head_len)
    {
        os_printk(K_ERR, "******Error : head_len=%d , expected head_len =%d \n",
                  rreq->transferCount, spd_scan_info->head_len);
    }
    rreq->transferCount = 0;
    rreq->complete = false;
    os_memcpy(spd_head_buf,os_phys_to_virt(req_buf),spd_scan_info->head_len);


    /*Prepare RX SPD to get spd data pattern*/
    os_printk(K_ERR, "Get spd data pattern\n");
    //enable spd feature
    os_setmsk(U3D_QCR4, SPD_EN(ep_num));


    if(zlp)
    {
        QCR = os_readl(U3D_QCR3);
        os_writel(U3D_QCR3, QCR|QMU_RX_ZLP(rx_ep));
    }
    else
    {
        QCR = os_readl(U3D_QCR3);
        os_writel(U3D_QCR3, QCR&~(QMU_RX_ZLP(rx_ep)));
    }

    os_writel(U3D_RXQ_SPD_MS1, rx_min_size);
    os_printk(K_ERR, "rx_min_size=%d \n", os_readl(U3D_RXQ_SPD_MS1));


    data_in_cnt  = 0;


    for(i=0 ;i<test_nr ; i++){

        para[i]=spd_head_buf+i*sizeof(SPD_RX_PARA);
        os_printk(K_ERR, "****** para[%d]->pdt =%d \n", i, para[i]->pdt);


        rsv_padding = ceiling(spd_scan_info->rsv, SPD_PADDING_BOUNDARY);

        for(j=0; j<para[i]->spd_num; j++){

            sub_spd = &(para[i]->sub_spd[j]);

            gpd = Rx_gpd_end[ep_num];
            os_printk(K_ERR,"RX gpd :%x\r\n",(unsigned int)gpd);
            gpd->flag = 0;
            TGPD_SET_FORMAT_PDT(gpd, para[i]->pdt);
            TGPD_SET_RX_SPD_RSV(gpd, spd_scan_info->rsv);

            if(j==0){
                if((sub_spd->break_flag)&&(spd_scan_info->err_type == FSPD_LEN_ERR))
                    mu3d_hal_prepare_rx_gpd(gpd, req_buf, maxp, ep_num,is_bdp, true,sub_spd->ioc, sub_spd->bps,maxp);
                else
                    mu3d_hal_prepare_rx_gpd(gpd, req_buf, allow_len, ep_num,is_bdp, true,sub_spd->ioc, sub_spd->bps,maxp);
            }
            else{
                if(sub_spd->break_flag){
                    break_idx = j;

                    if(spd_scan_info->err_type == NFSPD_WITH_RXGPD){
                        gpd->flag = 0;
                        mu3d_hal_prepare_rx_gpd(gpd, req_buf+gpd_buf_size, allow_len,ep_num,is_bdp, true,sub_spd->ioc, sub_spd->bps,maxp);
                        buf_num++;
                    }
                    if(spd_scan_info->err_type == NFSPD_WITH_BPSGPD){
                        gpd->flag = 0;
                        mu3d_hal_prepare_rx_gpd(gpd, req_buf+gpd_buf_size, allow_len,ep_num,is_bdp, true,sub_spd->ioc, sub_spd->bps,maxp);
                    }
                    if(spd_scan_info->err_type == NFSPD_WITH_BPSSPD){
                        os_printk(K_ERR,"pdt2 :%x\r\n",TGPD_GET_FORMAT_PDT(gpd));
                        mu3d_hal_prepare_rx_gpd(gpd, req_buf+gpd_buf_size, allow_len,ep_num,is_bdp, true,sub_spd->ioc, sub_spd->bps,maxp);
                    }
                }
                else{
                    mu3d_hal_prepare_rx_gpd(gpd, 0, 0, ep_num,is_bdp, true,sub_spd->ioc, sub_spd->bps,maxp);
                }
            }

        }

        mu3d_hal_resume_qmu(rx_ep, USB_RX);
        while(!req_complete(rx_ep, USB_RX));
        dma_sync_single_for_cpu(NULL, req_buf, (gpd_buf_size*buf_num), DMA_BIDIRECTIONAL);
        os_printk(K_ERR,"i: %d, transferCount :%d\r\n",i,rreq->transferCount);




        src = os_phys_to_virt(req_buf)+rsv_padding;
        os_printk(K_ERR,"tx_buf	: %x physical : %x \r\n",tx_buf,treq->dma_adr);

        if(rreq->transferCount == 0){
            if(spd_scan_info->err_type!=FSPD_LEN_ERR)
                os_printk(K_ERR,"ERR_type:NFSPD_WITH_RXGPD error \r\n");
            else if ((RX_SPD_SKP== 0)||(RX_SKP_XFE_LEN!=0))
                os_printk(K_ERR,"ERR_type:NFSPD_WITH_RXGPD error \r\n");
        }
        else{
            for(j=0;j<para[i]->spd_num;j++){
                sub_spd = &(para[i]->sub_spd[j]);

                if(j==break_idx){

                    if(spd_scan_info->err_type==NFSPD_WITH_RXGPD){
                        if((RX_SPD_SKP== 0)||(RX_SKP_XFE_LEN!=0))
                            os_printk(K_ERR,"ERR_type:NFSPD_WITH_RXGPD error \r\n");
                        rsv_padding = 0; //GPD
                    }
                    if((spd_scan_info->err_type==NFSPD_WITH_BPSGPD)||(spd_scan_info->err_type==NFSPD_WITH_BPSSPD)){
                        if(RX_SPD_SKP)
                            os_printk(K_ERR,"ERR_type:NFSPD_WITH_BPSGPD error \r\n");
                        continue;
                    }
                    src = os_phys_to_virt(req_buf)+gpd_buf_size+ rsv_padding;
                    os_memcpy(tx_buf,src,sub_spd->spd_rx_len);
                    tx_buf += sub_spd->spd_rx_len;
                    break;
                }

                if(sub_spd->bps)
                    continue;

                os_memcpy(tx_buf,src,sub_spd->spd_rx_len);

                src += (ceiling(sub_spd->spd_rx_len, SPD_PADDING_BOUNDARY) + rsv_padding);
                tx_buf += sub_spd->spd_rx_len;
            }

            req_buf += (gpd_buf_size*buf_num);
            buf_num = 1;
            break_idx = 0xff;
            data_in_cnt += rreq->transferCount;
            rreq->complete = false;
            rreq->transferCount = 0;
            RX_SPD_SKP = 0;
            RX_SPD_OVERSZ = 0;
            RX_SKP_XFE_LEN =0;
        }

    }

    //	os_printk(K_ERR, "rx done!Q cnt=%d\n", g_rxq_done_cnt);
    if(data_in_cnt != spd_scan_info->payload_len)
    {
        os_printk(K_ERR, "******Error : payload_len=%d , expected payload_len =%d \n",
                  data_in_cnt, spd_scan_info->payload_len);
    }



    /*Prepare  gpd for tx*/
    treq->count=spd_scan_info->payload_len;
    gpd_num = (spd_scan_info->payload_len/gpd_buf_size);

    if((spd_scan_info->payload_len)%gpd_buf_size){
        gpd_num++;
    }

    for(i=0 ;i<gpd_num ; i++){

        if(treq->count>gpd_buf_size){
            mu3d_hal_insert_transfer_gpd(tx_ep,USB_TX, (treq->dma_adr+i*gpd_buf_size), gpd_buf_size, true, true, false, zlp, maxp);
        }
        else{
            mu3d_hal_insert_transfer_gpd(tx_ep,USB_TX, (treq->dma_adr+i*gpd_buf_size),treq->count , true, true, false, zlp, maxp);
        }
        treq->count-= gpd_buf_size;
    }

    g_u3d_status = READY;


    os_setmsk(U3D_QCR2, QMU_TX_ZLP(tx_ep));

    mu3d_hal_resume_qmu(ep_num, USB_TX);
    //os_ms_delay(15);

    //dump_current_gpd(K_ERR, 1, USB_TX);
    while(!req_complete(ep_num, USB_TX));

    os_printk(K_ERR, "tx done!Q cnt=%d\n", g_txq_done_cnt);
    {
        PGPD gpd_ld, gpd_ld_rc, gpd_current;
        gpd_current = (PGPD) os_readl(USB_QMU_TQCPR(ep_num));
        gpd_ld = (TGPD*)os_readl(USB_QMU_TQLDPR(ep_num));
        gpd_ld_rc = (TGPD*)os_readl(USB_QMU_TQLDPR_RC(ep_num));
        os_printk(K_ERR, "Tx gpd_ld_rc@%p, gpd_ld@%p, gpd_current@%p\n", \
                  gpd_ld_rc, gpd_ld, gpd_current);
        gpd_ld = (TGPD*)os_readl(USB_QMU_TQLDPR(ep_num));
        gpd_ld_rc = (TGPD*)os_readl(USB_QMU_TQLDPR_RC(ep_num));
        os_printk(K_ERR, "***gpd_ld@%p, gpd_ld_rc@%p\n", gpd_ld, gpd_ld_rc);
    }
    treq->complete = false;

    kfree(spd_head_buf);
}



/**
 * dev_spd_loopback - spd loopback transfer 
 */
//static SPD_ERR_TYPE spd_force_err;
void dev_spd_rx_loopback(SPD_RX_SCAN_INFO *spd_scan_info, DEV_UINT8 tx_ep, DEV_UINT8 rx_ep)
{
    u16 test_nr;
    u8 zlp;
    struct USB_REQ *treq, *rreq;
    dma_addr_t mapping, tx_handle;
    DEV_UINT32 i, j,gpd_num, maxp, data_in_cnt;
    DEV_UINT8 *dma_buf, *payload_buf, *tx_buf, *spd_head_buf,*src;
    DEV_INT32 ep_num,QCR;
    RX_SUB_SPD *sub_spd;
    dma_addr_t req_buf;
    pSPD_RX_PARA para[MAX_SPD];
    u16 allow_len, rx_min_size,rsv_padding,spd_len;
    u8 skpid,skp_type;

    ep_num = tx_ep;
    test_nr = spd_scan_info->test_nr;
    zlp = spd_scan_info->zlp;
    allow_len = spd_scan_info->gpd_buf_size;
    rx_min_size = spd_scan_info->rx_spd_min_size;


    treq = mu3d_hal_get_req(tx_ep, USB_TX);
    rreq = mu3d_hal_get_req(rx_ep, USB_RX);
    dma_buf = g_loopback_buffer[0];
    payload_buf = g_loopback_buffer[1];
    treq->count = rreq->count = 0;
    treq->transferCount = rreq->transferCount = 0;
    treq->complete = rreq->complete = 0;
    os_memset(rreq->buf, 0 , g_dma_buffer_size);

    /*alloc memory*/
    mapping = dma_map_single(NULL, dma_buf,g_dma_buffer_size, DMA_BIDIRECTIONAL);
    dma_sync_single_for_device(NULL, mapping, g_dma_buffer_size, DMA_BIDIRECTIONAL);
    rreq->dma_adr = mapping;
    req_buf = rreq->dma_adr;

    tx_handle = dma_map_single(NULL, payload_buf,g_dma_buffer_size, DMA_BIDIRECTIONAL);
    dma_sync_single_for_cpu(NULL, tx_handle, g_dma_buffer_size, DMA_BIDIRECTIONAL);
    treq->dma_adr = tx_handle;

    spd_head_buf = os_mem_alloc(1024*4);
    tx_buf = payload_buf;

    /*init*/
    maxp = USB_ReadCsr32(U3D_RX1CSR0, rx_ep) & RX_RXMAXPKTSZ;
    i = 0;
    gpd_buf_size = 0x6000;
    is_bdp = bGPD_Extension = 0;
    g_u3d_status = BUSY;
    g_rxq_done_cnt = g_txq_done_cnt = 0;
    RX_SPD_SKP = 0;
    RX_SPD_OVERSZ = 0;
    RX_SKP_XFE_LEN =0;
    skpid = 0xff;
    skp_type = 0xFE;



    /*Get spd test pattern header info */
    os_printk(K_ERR, "Get spd test pattern header info\n");
    mu3d_hal_insert_transfer_gpd(rx_ep, USB_RX, req_buf, spd_scan_info->head_len, \
                                 true, true, false, zlp, maxp);
    mu3d_hal_resume_qmu(rx_ep, USB_RX);
    while(!req_complete(rx_ep, USB_RX));
    dma_sync_single_for_cpu(NULL, req_buf, rreq->transferCount, DMA_BIDIRECTIONAL);
    if(rreq->transferCount != spd_scan_info->head_len)
    {
        os_printk(K_ERR, "******Error : head_len=%d , expected head_len =%d \n",
                  rreq->transferCount, spd_scan_info->head_len);
    }
    rreq->transferCount = 0;
    rreq->complete = false;
    os_memcpy(spd_head_buf,os_phys_to_virt(req_buf),spd_scan_info->head_len);


    /*Prepare RX SPD to get spd data pattern*/
    os_printk(K_ERR, "Get spd data pattern\n");
    //enable spd feature
    os_setmsk(U3D_QCR4, SPD_EN(ep_num));


    if(zlp)
    {
        QCR = os_readl(U3D_QCR3);
        os_writel(U3D_QCR3, QCR|QMU_RX_ZLP(rx_ep));
    }
    else
    {
        QCR = os_readl(U3D_QCR3);
        os_writel(U3D_QCR3, QCR&~(QMU_RX_ZLP(rx_ep)));
    }

    os_writel(U3D_RXQ_SPD_MS1, rx_min_size);
    os_printk(K_ERR, "rx_min_size=%d \n", os_readl(U3D_RXQ_SPD_MS1));


    data_in_cnt  = 0;


    for(i=0 ;i<test_nr ; i++){

        para[i]=spd_head_buf+i*sizeof(SPD_RX_PARA);
        os_printk(K_ERR, "****** para[%d]->pdt =%d \n", i, para[i]->pdt);

        if(para[i]->pdt > GPD_SPD2){
            break;
        }

        if(para[i]->pdt == GPD_NORMAL){
            rsv_padding = 0;
            bGPD_Extension = para[i]->extension;
            if(bGPD_Extension) {
                os_printk(K_ERR, "bGPD_Extension=%d\n", bGPD_Extension);
                bGPD_Extension = 0;
            }
            mu3d_hal_insert_transfer_gpd(rx_ep,USB_RX, req_buf, allow_len,true,para[i]->sub_spd[0].ioc, para[i]->sub_spd[0].bps, zlp, maxp); 	//rx gpd zlp =1 due to host send zlp
        }
        else{ //spd1
            rsv_padding = ceiling(spd_scan_info->rsv, SPD_PADDING_BOUNDARY);
            mu3d_hal_insert_spd_rx(rx_ep, req_buf, para[i],allow_len,spd_scan_info->rsv, maxp);

        }
        mu3d_hal_resume_qmu(rx_ep, USB_RX);
        while(!req_complete(rx_ep, USB_RX));
        dma_sync_single_for_cpu(NULL, req_buf, allow_len, DMA_BIDIRECTIONAL);
        os_printk(K_ERR,"i: %d, transferCount :%d\r\n",i,rreq->transferCount);

        if(RX_SPD_SKP){

            if (!para[i]->skp_type)  {
                if((i==skpid +1)&& (skp_type==2))
                    os_printk(K_ERR,"SKP type2 case\r\n");
                else
                    os_printk(K_ERR,"i: %d, SKP TEST error\r\n",i);

            }
            else{
                skp_type = para[i]->skp_type;
                skpid = i;
                os_printk(K_ERR,"SKP test:skpid = %d type=%d\r\n",skpid,skp_type);
                switch(skp_type){

                case 1:
                    if((para[i]->skp_xfer_len != RX_SKP_XFE_LEN)&&(RX_SKP_XFE_LEN!=0))
                        os_printk(K_ERR,"SKP type1 error , en expec=%d ; actual=%d  \r\n",para[i]->skp_xfer_len,RX_SKP_XFE_LEN);
                    break;

                case 2:
                    if(RX_SPD_OVERSZ!= 1)
                        os_printk(K_ERR,"SKP type2 error , SKP oversz  error  \r\n");
                    if(para[i]->skp_xfer_len != RX_SKP_XFE_LEN)
                        os_printk(K_ERR,"SKP type2 error :skp_xfer_len expec=%d ; actual=%d  \r\n",para[i]->skp_xfer_len,RX_SKP_XFE_LEN);
                    break;

                default:
                    break;

                }

            }
        }


        src = os_phys_to_virt(req_buf)+rsv_padding;
        os_printk(K_ERR,"rsv_padding=%d\r\n",rsv_padding);
        os_printk(K_ERR,"tx_buf	: %x physical : %x \r\n",tx_buf,treq->dma_adr);
        for(j=0;j<para[i]->spd_num;j++){
            sub_spd = &(para[i]->sub_spd[j]);

            if(sub_spd->bps)
                continue;

            if((para[i]->skp_type)&&(para[i]->skp_type!=3) &&(j == (para[i]->spd_num -1)))
                spd_len = para[i]->skp_xfer_len;
            else
                spd_len = sub_spd->spd_rx_len+para[i]->extension;

            os_memcpy(tx_buf,src,spd_len);

            src += (ceiling(spd_len, SPD_PADDING_BOUNDARY) + rsv_padding);
            tx_buf += spd_len;
        }


        req_buf += gpd_buf_size;
        ///tx_buf +=  gpd_buf_size;
        data_in_cnt += rreq->transferCount;
        rreq->complete = false;
        rreq->transferCount = 0;
        RX_SPD_SKP = 0;
        RX_SPD_OVERSZ = 0;
        RX_SKP_XFE_LEN =0;
    }

    //	os_printk(K_ERR, "rx done!Q cnt=%d\n", g_rxq_done_cnt);
    if(data_in_cnt != spd_scan_info->payload_len)
    {
        os_printk(K_ERR, "******Error : payload_len=%d , expected payload_len =%d \n",
                  data_in_cnt, spd_scan_info->payload_len);
    }



    /*Prepare  gpd for tx*/
    treq->count=spd_scan_info->payload_len;
    gpd_num = (spd_scan_info->payload_len/gpd_buf_size);

    if((spd_scan_info->payload_len)%gpd_buf_size){
        gpd_num++;
    }

    for(i=0 ;i<gpd_num ; i++){

        if(treq->count>gpd_buf_size){
            mu3d_hal_insert_transfer_gpd(tx_ep,USB_TX, (treq->dma_adr+i*gpd_buf_size), gpd_buf_size, true, true, false, true, maxp);
        }
        else{
            mu3d_hal_insert_transfer_gpd(tx_ep,USB_TX, (treq->dma_adr+i*gpd_buf_size),treq->count , true, true, false, true, maxp);
        }
        treq->count-= gpd_buf_size;
    }

    g_u3d_status = READY;


    os_setmsk(U3D_QCR2, QMU_TX_ZLP(tx_ep));

    mu3d_hal_resume_qmu(ep_num, USB_TX);
    //os_ms_delay(15);

    //dump_current_gpd(K_ERR, 1, USB_TX);
    while(!req_complete(ep_num, USB_TX));

    os_printk(K_ERR, "tx done!Q cnt=%d\n", g_txq_done_cnt);
    {
        PGPD gpd_ld, gpd_ld_rc, gpd_current;
        gpd_current = (PGPD) os_readl(USB_QMU_TQCPR(ep_num));
        gpd_ld = (TGPD*)os_readl(USB_QMU_TQLDPR(ep_num));
        gpd_ld_rc = (TGPD*)os_readl(USB_QMU_TQLDPR_RC(ep_num));
        os_printk(K_ERR, "Tx gpd_ld_rc@%p, gpd_ld@%p, gpd_current@%p\n", \
                  gpd_ld_rc, gpd_ld, gpd_current);
        gpd_ld = (TGPD*)os_readl(USB_QMU_TQLDPR(ep_num));
        gpd_ld_rc = (TGPD*)os_readl(USB_QMU_TQLDPR_RC(ep_num));
        os_printk(K_ERR, "***gpd_ld@%p, gpd_ld_rc@%p\n", gpd_ld, gpd_ld_rc);
    }
    treq->complete = false;

    kfree(spd_head_buf);
}

#endif

#ifdef CAP_QMU_SPD
u32 get_spd_in_len(pGPD_PARA para)
{
    int i;
    u32 in_len=0;
    SPD_ENTRY_PARA *entry;
    in_len = 0;
    if (!para)
        return in_len;
    if(GPD_NORMAL == para->pdt) {
        in_len = para->transfer_len + para->spd1_hdr;
        return in_len;
    }
    for(i=0, entry=para->entry; i<para->pkt_num; i++, entry++) {
        if(GPD_SPD1 == para->pdt) {
            entry->spd2_hdr = para->spd1_hdr;
        }
        //printk(KERN_ERR );
        in_len += (entry->payload+entry->spd2_hdr);
    }
    return in_len;
}




/**
 * dev_spd_loopback - spd loopback transfer 
 */
//static SPD_ERR_TYPE spd_force_err;
void dev_spd_tx_loopback(SPD_SCAN_INFO *spd_scan_info, DEV_UINT8 tx_ep, DEV_UINT8 rx_ep)
{
 
    u32 payload_len;
    u8 zlp, padding, ehn, hdr_room;
    struct USB_REQ *treq, *rreq;
    dma_addr_t mapping, tx_handle;
    DEV_UINT32 i, gpd_num, size,maxp, spd_len,data_in_cnt;
    DEV_UINT8 *dma_buf, *payload_buf, *spd_data_ptr, *data_buf,*spd_head_buf,*tmp;
    DEV_INT32 ep_num;
    SPD_ENTRY_PARA *entry;
    dma_addr_t req_buf;
    pGPD_PARA para[MAX_SPD];
    TGPD *gpd;
	
    ep_num = tx_ep;
    gpd_num = spd_scan_info->gpd_nr;
    zlp = spd_scan_info->flag.zlp;
    ehn = spd_scan_info->flag.enh;
    padding = spd_scan_info->flag.padding;
    if (ehn)
        hdr_room = spd_scan_info->hdr_room;
    else
        hdr_room = 0;


    treq = mu3d_hal_get_req(tx_ep, USB_TX);
    rreq = mu3d_hal_get_req(rx_ep, USB_RX);
    dma_buf = g_loopback_buffer[0];
    payload_buf = g_loopback_buffer[1];
    treq->count = rreq->count = 0;
    treq->transferCount = rreq->transferCount = 0;
    treq->complete = rreq->complete = 0;
    os_memset(rreq->buf, 0 , g_dma_buffer_size);

    /*alloc memory*/
    mapping = dma_map_single(NULL, dma_buf,g_dma_buffer_size, DMA_BIDIRECTIONAL);
    dma_sync_single_for_device(NULL, mapping, g_dma_buffer_size, DMA_BIDIRECTIONAL);
    rreq->dma_adr = mapping;
    req_buf = rreq->dma_adr;

    tx_handle = dma_map_single(NULL, payload_buf,g_dma_buffer_size, DMA_BIDIRECTIONAL);
    dma_sync_single_for_cpu(NULL, tx_handle, g_dma_buffer_size, DMA_BIDIRECTIONAL);
    treq->dma_adr = tx_handle;

    spd_head_buf = os_mem_alloc(1024*4);
    tmp  = spd_head_buf;
    spd_data_ptr = payload_buf;

    /*get info*/
    maxp = USB_ReadCsr32(U3D_RX1CSR0, rx_ep) & RX_RXMAXPKTSZ;
    i = 0;
    gpd_buf_size = 64000;
    is_bdp = bGPD_Extension = 0;
    g_u3d_status = BUSY;
    g_rxq_done_cnt = g_txq_done_cnt = 0;


    /*Get spd test pattern header info */
    os_printk(K_ERR, "Get spd test pattern header info\n");
    mu3d_hal_insert_transfer_gpd(rx_ep, USB_RX, req_buf, spd_scan_info->head_len, \
                                 true, true, false, zlp, maxp);
    mu3d_hal_resume_qmu(rx_ep, USB_RX);
    while(!req_complete(rx_ep, USB_RX));
    dma_sync_single_for_cpu(NULL, req_buf, rreq->transferCount, DMA_BIDIRECTIONAL);
    if(rreq->transferCount != spd_scan_info->head_len)
    {
        os_printk(K_ERR, "******Error : head_len=%d , expected head_len =%d \n",
                  rreq->transferCount, spd_scan_info->head_len);
    }
    rreq->transferCount = 0;
    rreq->complete = false;
    os_memcpy(spd_head_buf,os_phys_to_virt(req_buf),spd_scan_info->head_len);

    /*Get spd data pattern*/
    os_printk(K_ERR, "Get spd data pattern\n");
    data_in_cnt  = 0;
    for(i=0 ;i<gpd_num ; i++){
        mu3d_hal_insert_transfer_gpd(rx_ep,USB_RX, req_buf, gpd_buf_size, true, true, false, zlp, maxp); 	//rx gpd zlp =1 due to host send zlp
        mu3d_hal_resume_qmu(rx_ep, USB_RX);
        while(!req_complete(rx_ep, USB_RX));
        dma_sync_single_for_cpu(NULL, req_buf, rreq->transferCount, DMA_BIDIRECTIONAL);
        os_printk(K_ERR,"i: %d, transferCount :%d\r\n",i,rreq->transferCount);
        req_buf += rreq->transferCount;
        data_in_cnt += rreq->transferCount;
        rreq->complete = false;
        rreq->transferCount = 0;
    }
    os_printk(K_NOTICE, "rx done!Q cnt=%d\n", g_rxq_done_cnt);
    if(data_in_cnt != spd_scan_info->payload_len)
    {
        os_printk(K_ERR, "******Error : payload_len=%d , expected payload_len =%d \n",
                  data_in_cnt, spd_scan_info->payload_len);
    }

    data_buf = dma_buf;

    /*Prepare spd& gpd for tx*/
    for (i=0; i < gpd_num; i++){

        para[i]=spd_head_buf;
        os_printk(K_ERR, "seq#[%d]spd_type=%d \n", i, para[i]->pdt );

        spd_len = 	get_spd_in_len(para[i]);

        if(para[i]->pdt > GPD_SPD2){
            os_printk(K_ERR, "******Error : para[%d]->pdt =%d \n", i, para[i]->pdt );
            break;
        }

        if(para[i]->pdt == GPD_NORMAL){
            size = sizeof(GPD_PARA);

            bGPD_Extension = para[i]->spd1_hdr;
            if(bGPD_Extension) {
                os_printk(K_ERR, "bGPD_Extension=%d\n", bGPD_Extension);
                bGPD_Extension = 0;
            }
            payload_len = spd_len;
            os_memcpy(spd_data_ptr,data_buf,payload_len);
            mu3d_hal_insert_transfer_gpd(tx_ep, USB_TX, (dma_addr_t) os_virt_to_phys(spd_data_ptr),
                                         payload_len, true, para[i]->ioc, para[i]->bps, zlp, maxp);

        }
        else{//spd1 & spd2
            gpd = Tx_gpd_end[tx_ep];
            size = sizeof(GPD_PARA) + (sizeof(SPD_ENTRY_PARA)*(para[i]->pkt_num));
            payload_len = mu3d_hal_insert_spd_tx(spd_data_ptr, data_buf,tx_ep, para[i],padding);

            if(spd_scan_info->flag.err_type){
                
                TSPD_EXT_HDR *spd_ext_hdr;
                TSPD_EXT_BODY *spd_ext_body;
              
                spd_ext_hdr = (void *) ((u32) gpd + sizeof(TGPD));
                switch (spd_scan_info->flag.err_type) {
                case ZERO_PKTNUM:
                    spd_ext_hdr->entry_nr = 0;
                    break;
                case PKTNUM_MISMATCH_ENTRY:
                    //para_pkt_num = actual entry_nr - err_para
                    spd_ext_hdr->entry_nr = spd_ext_hdr->entry_nr - spd_scan_info->err_para;
                    break;
                case IGR_AND_EOL:
                    //TODO: maybe can set igr/eol @ position err_para?
                    spd_ext_body = (void *) ((u32) spd_ext_hdr + sizeof(TSPD_EXT_HDR));
                    spd_ext_body->igr = spd_ext_body->eol = 1;
                    break;
                case ZERO_LOAD_AND_EOL:
                    //TODO: maybe can set igr/eol @ position err_para?
                    spd_ext_body = (void *) ((u32) spd_ext_hdr + sizeof(TSPD_EXT_HDR));
                    spd_ext_body->eol = 1;
                    spd_ext_body->payload_sz = 0;
                    break;
                default:
                    break;
                }
            }


        }
        os_printk(K_ERR, "payload_len=%d, spd_data_ptr=%p\n", payload_len, spd_data_ptr);
        spd_head_buf+=size;
        spd_data_ptr+=ceiling(payload_len, SPD_PADDING_BOUNDARY);//payload_len;
        data_buf += spd_len;
        g_u3d_status = READY;
    }


    //enable spd feature and padding
    os_setmsk(U3D_QCR4, SPD_EN(ep_num));
    if (zlp){
        os_setmsk(U3D_QCR2, QMU_TX_ZLP(ep_num));
    }else{
        os_clrmsk(U3D_QCR2, QMU_TX_ZLP(ep_num));
    }

    if (padding){
        os_setmsk(U3D_QCR4, SPD2_PADDING_EN(ep_num));
    }else{
        os_clrmsk(U3D_QCR4, SPD2_PADDING_EN(ep_num));
    }

    os_printk(K_ERR, "zlp=%d\tehn=%d\tpadding=%d\n", zlp, ehn, padding);

    treq->complete = false;

    mu3d_hal_resume_qmu(ep_num, USB_TX);
    //os_ms_delay(15);

    //dump_current_gpd(K_ERR, 1, USB_TX);
    while(!req_complete(ep_num, USB_TX)){
        if(spd_tx_err)
            break;
    }
    if(!spd_scan_info->flag.err_type && spd_tx_err)
        os_printk(K_ERR, "!!!SPD TX Error!!!!\n");
	
    os_printk(K_ERR, "tx done!Q cnt=%d\n", g_txq_done_cnt);
    {
        PGPD gpd_ld, gpd_ld_rc, gpd_current;
        gpd_current = (PGPD) os_readl(USB_QMU_TQCPR(ep_num));
        gpd_ld = (TGPD*)os_readl(USB_QMU_TQLDPR(ep_num));
        gpd_ld_rc = (TGPD*)os_readl(USB_QMU_TQLDPR_RC(ep_num));
        os_printk(K_ERR, "Tx gpd_ld_rc@%p, gpd_ld@%p, gpd_current@%p\n", \
                  gpd_ld_rc, gpd_ld, gpd_current);
        gpd_ld = (TGPD*)os_readl(USB_QMU_TQLDPR(ep_num));
        gpd_ld_rc = (TGPD*)os_readl(USB_QMU_TQLDPR_RC(ep_num));
        os_printk(K_ERR, "***gpd_ld@%p, gpd_ld_rc@%p\n", gpd_ld, gpd_ld_rc);
    }
    treq->complete = false;
    kfree(tmp);
}


#endif

/**
 * prepare_tx_stress_gpd - prepare tx gpd/bd for stress
 * @args - arg1: gpd address, arg2: data buffer address, arg3: data length, arg4: ep number, arg5: with bd or not, arg6: write hwo bit or not,  arg7: write ioc bit or not
 */
TGPD* prepare_tx_stress_gpd(TGPD* gpd, dma_addr_t pBuf, DEV_UINT32 data_length, DEV_UINT8 ep_num, DEV_UINT8 _is_bdp, DEV_UINT8 isHWO,DEV_UINT8 IOC){
    DEV_UINT32  offset;
    DEV_INT32 i,bd_num;

    TBD * bd_next;
    TBD * bd_head, *bd;
    DEV_UINT32 length;
    DEV_UINT8* pBuffer;
    DEV_UINT8 *_tmp;

    os_printk(K_INFO,"mu3d_hal_prepare_tx_gpd\r\n");

    if(data_length<= bGPD_Extension){
        _is_bdp=0;
    }

    if(!_is_bdp){

        TGPD_SET_DATA(gpd, pBuf+bGPD_Extension);
        TGPD_CLR_FORMAT_BDP(gpd);
    }
    else{

        bd_head=(TBD*)get_bd(USB_TX,ep_num);
        os_printk(K_INFO,"Malloc Tx 01 (BD) : 0x%x\r\n", (DEV_UINT32)bd_head);
        bd=bd_head;
        os_memset(bd, 0, sizeof(TBD)+bBD_Extension);
        length=data_length-bGPD_Extension;
        pBuffer= (DEV_UINT8*)(pBuf+bGPD_Extension);
        offset=bd_buf_size+bBD_Extension;
        bd_num = (!(length%offset)) ? (length/offset) : ((length/offset)+1);

        os_printk(K_INFO,"bd_num : 0x%x\r\n", (DEV_UINT32)bd_num);

        if(offset>length){
            offset=length;
        }

        for(i=0; i<bd_num; i++){
            if(i==(bd_num-1)){
                if(length<bBD_Extension){
                    TBD_SET_EXT_LEN(bd, length);
                    TBD_SET_BUF_LEN(bd, 0);
                    TBD_SET_DATA(bd, pBuffer+bBD_Extension);
                }
                else{
                    TBD_SET_EXT_LEN(bd, bBD_Extension);
                    TBD_SET_BUF_LEN(bd, length-bBD_Extension);
                    TBD_SET_DATA(bd, pBuffer+bBD_Extension);
                }

                TBD_SET_FLAGS_EOL(bd);
                TBD_SET_NEXT(bd, 0);
                TBD_SET_CHKSUM(bd, CHECKSUM_LENGTH);
                if(bBD_Extension){
                    dma_sync_single_for_cpu(NULL,(dma_addr_t)pBuf,g_dma_buffer_size,DMA_BIDIRECTIONAL);
                    _tmp=TBD_GET_EXT(bd);
                    os_memcpy(_tmp, os_phys_to_virt((dma_addr_t)pBuffer), bBD_Extension);
                    dma_sync_single_for_device(NULL, (dma_addr_t)pBuf, g_dma_buffer_size, DMA_BIDIRECTIONAL);
                }
                os_printk(K_INFO,"BD number %d\r\n", i+1);
                data_length=length+bGPD_Extension;
                length = 0;

                break;
            }else{

                TBD_SET_EXT_LEN(bd, bBD_Extension);
                TBD_SET_BUF_LEN(bd, offset-bBD_Extension);
                TBD_SET_DATA(bd, pBuffer+bBD_Extension);
                TBD_CLR_FLAGS_EOL(bd);
                bd_next = (TBD*)get_bd(USB_TX,ep_num);
                os_memset(bd_next, 0, sizeof(TBD)+bBD_Extension);
                TBD_SET_NEXT(bd, bd_virt_to_phys(bd_next,USB_TX,ep_num));
                TBD_SET_CHKSUM(bd, CHECKSUM_LENGTH);

                if(bBD_Extension){
                    dma_sync_single_for_cpu(NULL,(dma_addr_t)pBuf,g_dma_buffer_size,DMA_BIDIRECTIONAL);
                    _tmp=TBD_GET_EXT(bd);
                    os_memcpy(_tmp, os_phys_to_virt((dma_addr_t)pBuffer), bBD_Extension);
                    dma_sync_single_for_device(NULL, (dma_addr_t)pBuf, g_dma_buffer_size, DMA_BIDIRECTIONAL);
                }
                length -= offset;
                pBuffer += offset;
                bd=bd_next;
            }
        }

        TGPD_SET_DATA(gpd, bd_virt_to_phys(bd_head,USB_TX,ep_num));
        TGPD_SET_FORMAT_BDP(gpd);
    }

    os_printk(K_INFO,"GPD data_length %d \r\n", (data_length-bGPD_Extension));

    if(data_length<bGPD_Extension){
        TGPD_SET_BUF_LEN(gpd, 0);
        TGPD_SET_EXT_LEN(gpd, data_length);
    }
    else{
        TGPD_SET_BUF_LEN(gpd, data_length-bGPD_Extension);
        TGPD_SET_EXT_LEN(gpd, bGPD_Extension);
    }

    if(bGPD_Extension){

        dma_sync_single_for_cpu(NULL,(u32)pBuf,g_dma_buffer_size,DMA_BIDIRECTIONAL);
        _tmp=TGPD_GET_EXT(gpd);
        os_memcpy(_tmp, os_phys_to_virt(pBuf), bGPD_Extension);
        dma_sync_single_for_device(NULL, (u32)pBuf, g_dma_buffer_size, DMA_BIDIRECTIONAL);
    }
    if(!(USB_ReadCsr32(U3D_TX1CSR1, ep_num) & TYPE_ISO)){
        TGPD_SET_FORMAT_ZLP(gpd);
    }

    if(IOC){

        TGPD_SET_FORMAT_IOC(gpd);
    }
    else{

        TGPD_CLR_FORMAT_IOC(gpd);
    }

    //Create next GPD
    Tx_gpd_end[ep_num]=get_gpd(USB_TX ,ep_num);
    os_printk(K_INFO,"Malloc Tx 01 (GPD+EXT) (Tx_gpd_end) : 0x%x\r\n", (DEV_UINT32)Tx_gpd_end[ep_num]);
    TGPD_SET_NEXT(gpd, mu3d_hal_gpd_virt_to_phys(Tx_gpd_end[ep_num],USB_TX,ep_num));

    if(isHWO){
        TGPD_SET_CHKSUM(gpd, CHECKSUM_LENGTH);

        TGPD_SET_FLAGS_HWO(gpd);

    }else{
        TGPD_CLR_FLAGS_HWO(gpd);
        TGPD_SET_CHKSUM_HWO(gpd, CHECKSUM_LENGTH);
    }

    #if defined(USB_RISC_CACHE_ENABLED)  
    os_flushinvalidateDcache();
    #endif

    return gpd;
}

/**
 * resume_gpd_hwo - filling tx/rx gpd hwo bit and resmue qmu
 * @args - arg1: dir, arg2: ep number
 */
void resume_gpd_hwo(USB_DIR dir,DEV_INT32 Q_num){
    TGPD* gpd_current;
    DEV_INT32 i;

    if(dir == USB_RX){
        gpd_current = (TGPD*)(os_readl(USB_QMU_RQSAR(Q_num)));
    }
    else{
        gpd_current = (TGPD*)(os_readl(USB_QMU_TQSAR(Q_num)));
    }

    gpd_current = gpd_phys_to_virt(gpd_current,dir,Q_num);

    if(dir == USB_RX){
        gpd_current = (void*)gpd_current+STRESS_IOC_TH*Rx_gpd_IOC[Q_num]*sizeof(TGPD);
    }
    if(dir == USB_TX){
        gpd_current = (void*)gpd_current+STRESS_IOC_TH*Rx_gpd_IOC[Q_num]*sizeof(TGPD);
        gpd_current = (void*)gpd_current+STRESS_IOC_TH*Rx_gpd_IOC[Q_num]*AT_GPD_EXT_LEN;
    }

    for(i=0;i<(STRESS_IOC_TH);i++){
        if(dir == USB_RX){
            TGPD_SET_BUF_LEN(gpd_current, 0);
        }
        TGPD_SET_FLAGS_HWO(gpd_current);
        gpd_current++;
        if(dir == USB_TX){
            gpd_current = (void*)gpd_current+AT_GPD_EXT_LEN;
        }
    }

    if(dir == USB_RX){

        printk("Rx_gpd_IOC[%d] :%d\n", Q_num,Rx_gpd_IOC[Q_num]);
        if(Rx_gpd_IOC[Q_num]>=STRESS_IOC_TH){
            Rx_gpd_IOC[Q_num]=0;
        }
        else{
            Rx_gpd_IOC[Q_num]++;
        }
    }

}

/**
 * set_gpd_hwo - filling tx/rx gpd hwo bit 
 * @args - arg1: dir, arg2: ep number
 */
void set_gpd_hwo(USB_DIR dir,DEV_INT32 Q_num){
    TGPD *gpd_current, *gpd_current_tx;
    DEV_INT32 i;
    int cur_length;

    if(dir == USB_RX){
        gpd_current = (TGPD*)(os_readl(USB_QMU_RQSAR(Q_num)));
        gpd_current_tx = (TGPD*)(os_readl(USB_QMU_TQSAR(Q_num)));
        gpd_current_tx = gpd_phys_to_virt(gpd_current_tx,USB_TX,Q_num);
    }
    else{
        gpd_current = (TGPD*)(os_readl(USB_QMU_TQSAR(Q_num)));
    }
    gpd_current = gpd_phys_to_virt(gpd_current,dir,Q_num);
    for(i=0;i<(MAX_GPD_NUM);i++){
        if(dir == USB_RX){
            //set tx GPD buffer length
            cur_length = TGPD_GET_BUF_LEN(gpd_current);
            TGPD_SET_BUF_LEN(gpd_current_tx, cur_length);
            TGPD_SET_BUF_LEN(gpd_current, 0);
            gpd_current_tx++;
            gpd_current_tx = (void*)gpd_current_tx+AT_GPD_EXT_LEN;

        }
        if(dir == USB_RX){
            TGPD_SET_FLAGS_HWO(gpd_current);
        }
        else{
            TGPD_SET_CHKSUM_HWO(gpd_current,CHECKSUM_LENGTH);
            TGPD_SET_FLAGS_HWO(gpd_current);
        }
        gpd_current++;
        if(dir == USB_TX){
            gpd_current = (void*)gpd_current+AT_GPD_EXT_LEN;
        }
    }

}

/**
 * prepare_rx_stress_gpd - prepare rx gpd/bd for stress
 * @args - arg1: gpd address, arg2: data buffer address, arg3: data length, arg4: ep number, arg5: with bd or not, arg6: write hwo bit or not,  arg7: write ioc bit or not
 */
TGPD* prepare_rx_stress_gpd(TGPD*gpd, dma_addr_t pBuf, DEV_UINT32 data_len, DEV_UINT8 ep_num, DEV_UINT8 _is_bdp, DEV_UINT8 isHWO, DEV_UINT8 IOC){
    DEV_UINT32  offset;
    DEV_INT32 i,bd_num;

    TBD * bd_next;
    TBD * bd_head, *bd;
    DEV_UINT8* pBuffer;

    os_printk(K_DEBUG,"GPD 0x%x\r\n", (DEV_UINT32)gpd);

    if(!_is_bdp){

        TGPD_SET_DATA(gpd, pBuf);
        TGPD_CLR_FORMAT_BDP(gpd);
    }
    else{

        bd_head=(TBD*)get_bd(USB_RX,ep_num);
        os_printk(K_DEBUG,"Malloc Rx 01 (BD) : 0x%x\r\n", (DEV_UINT32)bd_head);
        bd=bd_head;
        os_memset(bd, 0, sizeof(TBD));
        offset=bd_buf_size;
        pBuffer= (DEV_UINT8*)(pBuf);
        bd_num = (!(data_len%offset)) ? (data_len/offset) : ((data_len/offset)+1);

        for(i=0; i<bd_num; i++){

            TBD_SET_BUF_LEN(bd, 0);
            TBD_SET_DATA(bd, pBuffer);
            if(i==(bd_num-1)){
                TBD_SET_DataBUF_LEN(bd, data_len);
                TBD_SET_FLAGS_EOL(bd);
                TBD_SET_NEXT(bd, 0);
                TBD_SET_CHKSUM(bd, CHECKSUM_LENGTH);
                os_printk(K_DEBUG,"BD number %d\r\n", i+1);
                break;
            }else{
                TBD_SET_DataBUF_LEN(bd, offset);
                TBD_CLR_FLAGS_EOL(bd);
                bd_next = (TBD*)get_bd(USB_RX,ep_num);
                os_memset(bd_next, 0, sizeof(TBD));
                TBD_SET_NEXT(bd, bd_virt_to_phys(bd_next,USB_RX,ep_num));
                TBD_SET_CHKSUM(bd, CHECKSUM_LENGTH);
                pBuffer += offset;
                data_len-=offset;
                bd=bd_next;

            }
        }

        TGPD_SET_DATA(gpd, bd_virt_to_phys(bd_head,USB_RX,ep_num));
        TGPD_SET_FORMAT_BDP(gpd);
    }

    os_printk(K_DEBUG,"data_len 0x%x\r\n", data_len);
    TGPD_SET_DataBUF_LEN(gpd, gpd_buf_size);
    TGPD_SET_BUF_LEN(gpd, 0);
    if(IOC)
    {
        TGPD_SET_FORMAT_IOC(gpd);
    }
    else{
        TGPD_CLR_FORMAT_IOC(gpd);
    }

    Rx_gpd_end[ep_num]=get_gpd(USB_RX ,ep_num);
    os_printk(K_DEBUG,"Rx Next GPD 0x%x\r\n",(DEV_UINT32)Rx_gpd_end[ep_num]);
    TGPD_SET_NEXT(gpd, mu3d_hal_gpd_virt_to_phys(Rx_gpd_end[ep_num],USB_RX,ep_num));

    if(isHWO){
        TGPD_SET_CHKSUM(gpd, CHECKSUM_LENGTH);
        TGPD_SET_FLAGS_HWO(gpd);

    }else{
        TGPD_CLR_FLAGS_HWO(gpd);
        TGPD_SET_CHKSUM_HWO(gpd, CHECKSUM_LENGTH);
    }
    os_printk(K_DEBUG,"Rx gpd info { HWO %d, Next_GPD %x ,DataBufferLength %d, DataBuffer %x, Recived Len %d, Endpoint %d, TGL %d, ZLP %d}\r\n",
              (DEV_UINT32)TGPD_GET_FLAG(gpd), (DEV_UINT32)TGPD_GET_NEXT(gpd),
              (DEV_UINT32)TGPD_GET_DataBUF_LEN(gpd), (DEV_UINT32)TGPD_GET_DATA(gpd),
              (DEV_UINT32)TGPD_GET_BUF_LEN(gpd), (DEV_UINT32)TGPD_GET_EPaddr(gpd),
              (DEV_UINT32)TGPD_GET_TGL(gpd), (DEV_UINT32)TGPD_GET_ZLP(gpd));

    return gpd;
}

/**
 * insert_stress_gpd - insert stress gpd/bd 
 * @args - arg1: ep number, arg2: dir, arg3: data buffer, arg4: data length,  arg5: write hwo bit or not,  arg6: write ioc bit or not
 */
void insert_stress_gpd(DEV_INT32 ep_num,USB_DIR dir, dma_addr_t buf, DEV_UINT32 count, DEV_UINT8 isHWO, DEV_UINT8 IOC){

    TGPD* gpd;
    os_printk(K_INFO,"mu3d_hal_insert_transfer_gpd\r\n");
    os_printk(K_INFO,"ep_num :%d\r\n",ep_num);
    os_printk(K_INFO,"dir :%d\r\n",dir);
    os_printk(K_INFO,"buf :%x\r\n",(DEV_UINT32)buf);
    os_printk(K_INFO,"count :%x\r\n",count);

    if(dir == USB_TX){
        TGPD* gpd = Tx_gpd_end[ep_num];
        os_printk(K_INFO,"TX gpd :%p\r\n",gpd);
        prepare_tx_stress_gpd(gpd, buf, count, ep_num,is_bdp, isHWO,IOC);
    }
    else if(dir == USB_RX){
        gpd = Rx_gpd_end[ep_num];
        os_printk(K_INFO,"RX gpd :%p\r\n",gpd);
        prepare_rx_stress_gpd(gpd, buf, count, ep_num,is_bdp, isHWO,IOC);
    }

}

/**
 * mu3d_hal_restart_qmu_no_flush - stop qmu and restart without flushfifo
 * @args - arg1: ep number, arg2: dir, arg3: txq resume method
 */
void mu3d_hal_restart_qmu_no_flush(DEV_INT32 Q_num, USB_DIR dir, DEV_INT8 method){


    TGPD* gpd_current;
    unsigned long flags;

    if(dir == USB_TX){
        spin_lock_irqsave(&lock, flags);
        mu3d_hal_stop_qmu(Q_num, USB_TX);
        while((os_readl(USB_QMU_TQCSR(Q_num)) & (QMU_Q_ACTIVE)));
        if(method==1){// set txpktrdy twice to ensure zlp will be sent
            while((USB_ReadCsr32(U3D_TX1CSR0, Q_num) & TX_FIFOFULL));
            USB_WriteCsr32(U3D_TX1CSR0, Q_num, USB_ReadCsr32(U3D_TX1CSR0, Q_num) &~ TX_DMAREQEN);
            USB_WriteCsr32(U3D_TX1CSR0, Q_num, USB_ReadCsr32(U3D_TX1CSR0, Q_num) | TX_TXPKTRDY);
            USB_WriteCsr32(U3D_TX1CSR0, Q_num, USB_ReadCsr32(U3D_TX1CSR0, Q_num) | TX_DMAREQEN);
            while((USB_ReadCsr32(U3D_TX1CSR0, Q_num) & TX_FIFOFULL));
            USB_WriteCsr32(U3D_TX1CSR0, Q_num, USB_ReadCsr32(U3D_TX1CSR0, Q_num) &~ TX_DMAREQEN);
            USB_WriteCsr32(U3D_TX1CSR0, Q_num, USB_ReadCsr32(U3D_TX1CSR0, Q_num) | TX_TXPKTRDY);
            USB_WriteCsr32(U3D_TX1CSR0, Q_num, USB_ReadCsr32(U3D_TX1CSR0, Q_num) | TX_DMAREQEN);
            os_printk(K_CRIT,"method 01\r\n");
        }
        if(method==2){
            if(!(USB_ReadCsr32(U3D_TX1CSR0, Q_num) & TX_FIFOFULL)){
                USB_WriteCsr32(U3D_TX1CSR0, Q_num, USB_ReadCsr32(U3D_TX1CSR0, Q_num) &~ TX_DMAREQEN);
                USB_WriteCsr32(U3D_TX1CSR0, Q_num, USB_ReadCsr32(U3D_TX1CSR0, Q_num) | TX_TXPKTRDY);
                USB_WriteCsr32(U3D_TX1CSR0, Q_num, USB_ReadCsr32(U3D_TX1CSR0, Q_num) | TX_DMAREQEN);
                printk("STOP TX=> TXPKTRDY\n");
            }
            os_printk(K_CRIT,"method 02\r\n");
        }

        spin_unlock_irqrestore(&lock, flags);
        gpd_current = (TGPD*)(os_readl(USB_QMU_TQCPR(Q_num)));
        if(!gpd_current){
            gpd_current = (TGPD*)(os_readl(USB_QMU_TQSAR(Q_num)));
        }
        os_printk(K_CRIT,"gpd_current(P) %p\r\n", gpd_current);
        gpd_current = gpd_phys_to_virt(gpd_current,USB_TX,Q_num);
        os_printk(K_CRIT,"gpd_current(V) %p\r\n", gpd_current);
        Tx_gpd_end[Q_num] = Tx_gpd_last[Q_num] = gpd_current;
        gpd_ptr_align(dir,Q_num,Tx_gpd_end[Q_num]);
        os_memset(Tx_gpd_end[Q_num], 0 , sizeof(TGPD));
        //free_gpd(dir,Q_num);
        os_writel(USB_QMU_TQSAR(Q_num), mu3d_hal_gpd_virt_to_phys(Tx_gpd_last[Q_num],USB_TX,Q_num));
        mu3d_hal_start_qmu(Q_num, USB_TX);
    }else{

        spin_lock_irqsave(&lock, flags);
        mu3d_hal_stop_qmu(Q_num, USB_RX);
        while((os_readl(USB_QMU_RQCSR(Q_num)) & (QMU_Q_ACTIVE)));
        if(!(USB_ReadCsr32(U3D_RX1CSR0, Q_num) & RX_FIFOEMPTY)){
            os_printk(K_INFO, "fifo not empty %x\r\n", USB_ReadCsr32(U3D_RX1CSR0, Q_num));
            USB_WriteCsr32(U3D_RX1CSR0, Q_num, USB_ReadCsr32(U3D_RX1CSR0, Q_num) | RX_RXPKTRDY);
        }
        spin_unlock_irqrestore(&lock, flags);

        gpd_current = (TGPD*)(os_readl(USB_QMU_RQCPR(Q_num)));
        if(!gpd_current){
            gpd_current = (TGPD*)(os_readl(USB_QMU_RQSAR(Q_num)));
        }
        os_printk(K_CRIT,"gpd_current(P) %p\r\n", gpd_current);
        gpd_current = gpd_phys_to_virt(gpd_current,USB_RX,Q_num);
        os_printk(K_CRIT,"gpd_current(V) %p\r\n", gpd_current);
        Rx_gpd_end[Q_num] = gpd_current;
        gpd_ptr_align(dir,Q_num,Rx_gpd_end[Q_num]);
        //free_gpd(dir,Q_num);
        os_memset(Rx_gpd_end[Q_num], 0 , sizeof(TGPD));
        os_writel(USB_QMU_RQSAR(Q_num), mu3d_hal_gpd_virt_to_phys(Rx_gpd_end[Q_num],USB_RX,Q_num));
        mu3d_hal_start_qmu(Q_num, USB_RX);
    }

}


/**
 * proc_qmu_rx - handle rx ioc event
 * @args - arg1: ep number
 */
void proc_qmu_rx(DEV_INT32 ep_num){

    DEV_UINT32 bufferlength=0,recivedlength=0,i;
    DEV_UINT8 IOC;
#ifdef RX_QMU_SPD
    DEV_UINT8 SKP;
#endif
    struct USB_REQ *req = mu3d_hal_get_req(ep_num, USB_RX);
    TGPD* gpd=(TGPD*)Rx_gpd_last[ep_num];
    TGPD* gpd_current = (TGPD*)(os_readl(USB_QMU_RQCPR(ep_num)));
    TBD*bd;
#ifdef TEST_Q_WAKE_EVENT
    os_printk(MGC_DebugLevel, "spm status=%x\n", os_readl(U3D_QISAR2));
    os_writel(U3D_QISAR2, 0xffffffff);
#endif

    if(g_run_stress){

        if(g_insert_hwo){
            dev_insert_stress_gpd_hwo(USB_RX, ep_num);
            dev_insert_stress_gpd_hwo(USB_TX, ep_num);
        }
        else{
            for(i=0;i<STRESS_IOC_TH;i++){
                IOC = ((i%STRESS_IOC_TH)==(STRESS_IOC_TH/2)) ? true : false;
                dev_insert_stress_gpd(USB_RX,ep_num,IOC);
            }
            mu3d_hal_resume_qmu(ep_num, USB_RX);
        }
    }
    else{

#ifdef TEST_Q_LASTDONE_PTR
        TGPD *gpd_ld = (TGPD*)os_readl(USB_QMU_RQLDPR(ep_num));
        TGPD *gpd_ld_rc = (TGPD*)os_readl(USB_QMU_RQLDPR_RC(ep_num));
        os_printk(MGC_DebugLevel, "Rx gpd_ld_rc@%p, gpd_ld@%p, gpd_current@%p\n", \
                  gpd_ld_rc, gpd_ld, gpd_current);
        gpd_ld_rc = (TGPD*)os_readl(USB_QMU_RQLDPR(ep_num));
        gpd_ld = gpd_phys_to_virt(gpd_ld,USB_RX,ep_num);
        os_printk(MGC_DebugLevel, "***gpd_ld@%p, gpd_ld->next@%p\n", gpd_ld_rc, gpd_ld->pNext);
#endif
        gpd_current = gpd_phys_to_virt(gpd_current,USB_RX,ep_num);
        os_printk(K_INFO,"ep_num : %d ,Rx_gpd_last : 0x%x, gpd_current : 0x%x, gpd_end : 0x%x \r\n",ep_num,(DEV_UINT32)gpd, (DEV_UINT32)gpd_current, (DEV_UINT32)Rx_gpd_end[ep_num]);

        if(gpd==gpd_current){
            return;
        }

        while(gpd != gpd_current && !TGPD_IS_FLAGS_HWO(gpd)){

            if(TGPD_IS_FORMAT_BDP(gpd)){

                bd = TGPD_GET_DATA(gpd);
                bd = bd_phys_to_virt(bd,USB_RX,ep_num);
                while(1){
                    os_printk(K_INFO,"BD : 0x%p\r\n", bd);
                    os_printk(K_INFO,"Buffer Len : 0x%x\r\n",(DEV_UINT32)TBD_GET_BUF_LEN(bd));
                    req->transferCount += TBD_GET_BUF_LEN(bd);
                    os_printk(K_INFO,"Total Len : 0x%x\r\n",req->transferCount);
                    if(TBD_IS_FLAGS_EOL(bd)){
                        break;
                    }
                    bd= TBD_GET_NEXT(bd);
                    bd=bd_phys_to_virt(bd,USB_RX,ep_num);
                }
            }
            else{
                recivedlength = (DEV_UINT32)TGPD_GET_BUF_LEN(gpd);
                bufferlength  = (DEV_UINT32)TGPD_GET_DataBUF_LEN(gpd);
                req->transferCount += recivedlength;
            }

 #ifdef RX_QMU_SPD

             SKP= TGPD_GET_SKP_SPD(gpd);
            if(SKP)
            {
                RX_SKP_XFE_LEN = recivedlength;
		  RX_SPD_SKP = 1;	
		 if(os_readl(U3D_RXQ_SPD_STS1)&RXQ_SPD_OVERSZ)
                  RX_SPD_OVERSZ = 1;
               else
                   RX_SPD_OVERSZ = 0;
                os_printk(K_ERR,"====RX SKP : gpd =%p\n",gpd);
                os_printk(K_ERR,"====RX SKP : OVERSZ Flag =%d RX_SKP_XFE_LEN =%d \n",RX_SPD_OVERSZ,RX_SKP_XFE_LEN);
                os_printk(K_ERR,"====RX SKP : U3D_RXQ_NB_PR1 =%x\n",os_readl(U3D_RXQ_NB_PR1));
            }
            TGPD_CLR_FORMAT_SKP_SPD(gpd);


            if(TGPD_GET_FORMAT_PDT(gpd)==GPD_NORMAL){
                if(RX_SPD_OVERSZ||RX_SPD_SKP)
                    os_printk(K_ERR,"==Error : GPD should clear OVERSZ & SKP\n");				
            }

            os_printk(K_INFO,"INT:Rx gpd info { HWO %d, Current_GPD %x ,DataBufferLength %d, DataBuffer %x, Recived Len %d gpd =%p}\r\n",
                      (DEV_UINT32)TGPD_GET_FLAG(gpd),  (TGPD *)os_readl(USB_QMU_RQCPR(1)),
                      (DEV_UINT32)TGPD_GET_DataBUF_LEN(gpd), (DEV_UINT32)TGPD_GET_DATA(gpd),
                      (DEV_UINT32)TGPD_GET_BUF_LEN(gpd),gpd);

			
#endif

            os_printk(K_INFO,"INT:Rx gpd info { HWO %d, Next_GPD %x ,DataBufferLength %d, DataBuffer %x, Recived Len %d, Endpoint %d, TGL %d, ZLP %d}\r\n",
                      (DEV_UINT32)TGPD_GET_FLAG(gpd), (DEV_UINT32)TGPD_GET_NEXT(gpd),
                      (DEV_UINT32)TGPD_GET_DataBUF_LEN(gpd), (DEV_UINT32)TGPD_GET_DATA(gpd),
                      (DEV_UINT32)TGPD_GET_BUF_LEN(gpd), (DEV_UINT32)TGPD_GET_EPaddr(gpd),
                      (DEV_UINT32)TGPD_GET_TGL(gpd), (DEV_UINT32)TGPD_GET_ZLP(gpd));


            gpd=TGPD_GET_NEXT(gpd);
            gpd=gpd_phys_to_virt(gpd,USB_RX,ep_num);
        }

        Rx_gpd_last[ep_num]=gpd;
        os_printk(K_INFO,"Rx_gpd_last[%d] : 0x%x\r\n", ep_num,(DEV_UINT32)Rx_gpd_last[ep_num]);
        os_printk(K_INFO,"Rx_gpd_end[%d] : 0x%x\r\n", ep_num,(DEV_UINT32)Rx_gpd_end[ep_num]);
        rx_IOC_count++;

        if((!TGPD_IS_FLAGS_HWO(Rx_gpd_last[ep_num])) && (TGPD_GET_NEXT(Rx_gpd_last[ep_num])== NULL))
            req->complete = 1;
#if 0
        if(is_bdp)
        {
            if ((req->transferCount==req->count))
            {
                req->complete = 1;
                os_printk(K_NOTICE,"bdp:recivedlength %d ,bufferlength%d\r\n", recivedlength,bufferlength);
            }
        }
        else
        {
            if ((req->transferCount==req->count)||((recivedlength<bufferlength)&&(recivedlength!=0))) {
                req->complete = 1;
                os_printk(K_INFO,"Rx %d complete,req->count %d,transferCount %d\r\n", ep_num,req->count,req->transferCount);
                os_printk(K_INFO,"recivedlength %d ,bufferlength%d\r\n", recivedlength,bufferlength);
            }
        }
#endif

    }
}

/**
 * proc_qmu_tx - handle tx ioc event
 * @args - arg1: ep number
 */
void proc_qmu_tx(DEV_INT32 ep_num)
{
    struct USB_REQ *req = mu3d_hal_get_req(ep_num, USB_TX);
    TGPD* gpd=Tx_gpd_last[ep_num];
    TGPD* gpd_current = (TGPD*)(os_readl(USB_QMU_TQCPR(ep_num)));
    DEV_UINT32 i;
    DEV_UINT8 IOC;
#ifdef TEST_Q_WAKE_EVENT
    os_printk(MGC_DebugLevel, "spm status=%x\n", os_readl(U3D_QISAR2));
    os_writel(U3D_QISAR2, 0xffffffff);
#endif
    if(g_run_stress){

        if(!g_insert_hwo){
            for(i=0;i<STRESS_IOC_TH;i++){
                IOC = ((i%STRESS_IOC_TH)==(STRESS_IOC_TH/2)) ? true : false;
                dev_insert_stress_gpd(USB_TX,ep_num,IOC);
            }
            mu3d_hal_resume_qmu(ep_num, USB_TX);
        }
    }
    else{
#ifdef TEST_Q_LASTDONE_PTR
        TGPD *gpd_ld = (TGPD*)os_readl(USB_QMU_TQLDPR(ep_num));
        TGPD *gpd_ld_rc = (TGPD*)os_readl(USB_QMU_TQLDPR_RC(ep_num));
        os_printk(MGC_DebugLevel, "Tx gpd_ld_rc@%p, gpd_ld@%p, gpd_current@%p\n", \
                  gpd_ld_rc, gpd_ld, gpd_current);
        gpd_ld = gpd_phys_to_virt(gpd_ld,USB_TX,ep_num);
        gpd_ld_rc = (TGPD*)os_readl(USB_QMU_TQLDPR(ep_num));
        os_printk(MGC_DebugLevel, "***gpd_ld@%p, gpd_ld->next@%p\n", gpd_ld_rc, gpd_ld->pNext);
#endif
        gpd_current = gpd_phys_to_virt(gpd_current,USB_TX,ep_num);
        os_printk(K_INFO, "Tx_gpd_last 0x%x, gpd_current 0x%x, gpd_end 0x%x, \r\n",  \
                  (DEV_UINT32)gpd, (DEV_UINT32)gpd_current, (DEV_UINT32)Tx_gpd_end[ep_num]);

        if(gpd==gpd_current){
            return;
        }
        while(gpd!=gpd_current && !TGPD_IS_FLAGS_HWO(gpd)){
            os_printk(K_INFO, "Tx gpd %x info { HWO %d, BPD %d, Next_GPD %x , DataBuffer %x, BufferLen %d, Endpoint %d}\r\n",
                      (DEV_UINT32)gpd, (DEV_UINT32)TGPD_GET_FLAG(gpd), (DEV_UINT32)TGPD_GET_FORMAT(gpd), (DEV_UINT32)TGPD_GET_NEXT(gpd),
                      (DEV_UINT32)TGPD_GET_DATA(gpd), (DEV_UINT32)TGPD_GET_BUF_LEN(gpd), (DEV_UINT32)TGPD_GET_EPaddr(gpd));
            gpd=TGPD_GET_NEXT(gpd);
            gpd=gpd_phys_to_virt(gpd,USB_TX,ep_num);
        }
        Tx_gpd_last[ep_num]=gpd;
        if (Tx_gpd_end[ep_num] == gpd){
            dump_current_gpd(K_NOTICE, ep_num, USB_TX);
            req->complete = true;
            os_printk(K_NOTICE, "Tx %d complete\r\n", ep_num);
        }
        os_printk(K_INFO, "Tx_gpd_last[%d] : 0x%x\r\n", ep_num,(DEV_UINT32)Tx_gpd_last[ep_num]);
        os_printk(K_INFO, "Tx_gpd_end[%d] : 0x%x\r\n", ep_num,(DEV_UINT32)Tx_gpd_end[ep_num]);
    }
}

/**
 * qmu_handler - handle qmu error events
 * @args - arg1: ep number
 */
void qmu_handler(DEV_UINT32 wQmuVal){
    DEV_INT32 i, wErrVal;

    os_printk(K_INFO, "qmu_handler %x\r\n", wQmuVal);

    if((wQmuVal & RXQ_CSERR_INT)||(wQmuVal & RXQ_LENERR_INT)){
        wErrVal=os_readl(U3D_RQERRIR0);
        os_printk(K_INFO,"U3D_RQERRIR0 : [0x%x]\r\n", wErrVal);
        for(i=1; i<=MAX_QMU_EP; i++){
            if(wErrVal & QMU_RX_CS_ERR(i)){
                os_printk(K_ALET,"Rx %d checksum error!\r\n", i);
                dump_current_gpd(K_ERR, i, USB_RX);
                //while(1);
            }
            if(wErrVal & QMU_RX_LEN_ERR(i)){
                os_printk(K_ALET,"Rx %d recieve length error!\r\n", i);
		g_rx_len_err_cnt++;
                dump_current_gpd(K_ERR, i, USB_RX);
                //while(1);
            }
        }
        os_writel(U3D_RQERRIR0, wErrVal);
    }

    if(wQmuVal & RXQ_ZLPERR_INT){
        wErrVal=os_readl(U3D_RQERRIR1);
        os_printk(K_INFO,"U3D_RQERRIR1 : [0x%x]\r\n", wErrVal);

        for(i=1; i<=MAX_QMU_EP; i++){
            if(wErrVal &QMU_RX_ZLP_ERR(i)){
                os_printk(K_INFO,"Rx %d recieve an zlp packet!\r\n", i);
            }
        }
        os_writel(U3D_RQERRIR1, wErrVal);
    }

    if((wQmuVal & TXQ_CSERR_INT)||(wQmuVal & TXQ_LENERR_INT)){

        wErrVal=os_readl(U3D_TQERRIR0);

        for(i=1; i<=MAX_QMU_EP; i++){
            if(wErrVal &QMU_TX_CS_ERR(i)){
                os_printk(K_ALET,"Tx %d checksum error!\r\n", i);
                dump_current_gpd(K_ERR, i, USB_TX);
                //while(1);
            }

            if(wErrVal &QMU_TX_LEN_ERR(i)){
                os_printk(K_ALET,"Tx %d buffer length error!\r\n", i);
                dump_current_gpd(K_ERR, i, USB_TX);
                //while(1);
            }
        }
        os_writel(U3D_TQERRIR0, wErrVal);

    }
    if(wQmuVal & TXQ_SPD_PKTNUM_INT){
        wErrVal=os_readl(U3D_TQERRIR1);
        for(i=1; i<=MAX_QMU_EP; i++){
            if(wErrVal & QMU_TX_SPD_PKTNUM_ERR(i)){
                os_printk(K_ALET,"Tx %d SPD PKT NUM error!\r\n", i);
                dump_current_gpd(K_ERR, i, USB_TX);
                spd_tx_err = 1;
                //while(1);
            }
        }
        os_writel(U3D_TQERRIR1, wErrVal);

    }

    if(wQmuVal & RXQ_EMPTY_INT){
        DEV_UINT32 wEmptyVal=os_readl(U3D_QEMIR);

        os_printk(K_INFO,"%sQ Empty in QMU mode![0x%x]\r\n",  g_usb_dir[1], wEmptyVal);


        for(i=1; i<=MAX_QMU_EP; i++){

            if(wEmptyVal &QMU_RX_EMPTY(i)){
                os_printk(K_INFO,"Rx %d empty int!\r\n", i);
            }
        }
        os_writel(U3D_QEMIR, wEmptyVal);

    }

    if(wQmuVal & TXQ_EMPTY_INT){
        DEV_UINT32 wEmptyVal=os_readl(U3D_QEMIR);
        os_printk(K_INFO,"%sQ Empty in QMU mode![0x%x]\r\n",  g_usb_dir[0], wEmptyVal);

        for(i=1; i<=MAX_QMU_EP; i++){

            if(wErrVal &QMU_TX_EMPTY(i)){
                os_printk(K_INFO,"Tx %d empty int!\r\n", i);
            }
        }
        os_writel(U3D_QEMIR, wEmptyVal);

    }

}


void qmu_proc(DEV_UINT32 wQmuVal){
    DEV_INT32 i;

    for(i=1; i<5; i++){
        if(wQmuVal & QMU_RX_DONE(i)){
            g_rxq_done_cnt++;
            os_printk(K_INFO, "RxQ[%d] done \n", i);
            proc_qmu_rx(i);
        }
        if(wQmuVal & QMU_TX_DONE(i)){
            g_txq_done_cnt++;
            os_printk(K_INFO, "TxQ[%d] done \n", i);
            //			dump_current_gpd(MGC_DebugLevel, i, USB_TX);
            os_printk(K_INFO, "CPR=%x\n", os_readl(USB_QMU_TQLDPR(i)));
            proc_qmu_tx(i);
        }
    }
}

DEV_INT32 u3d_dev_suspend(void){

    return g_usb_status.suspend;
}


#ifdef EXT_VBUS_DET
irqreturn_t u3d_vbus_rise_handler(int irq, void *dev_id){
             os_printk(K_ERR, "u3d_vbus_rise_handler\n");
             reset_dev(U3D_DFT_SPEED, 0, 1);

             os_writel(FPGA_REG, (os_readl(FPGA_REG) &~ VBUS_MSK ) | VBUS_RISE_BIT);
             return IRQ_HANDLED;
         }

         irqreturn_t u3d_vbus_fall_handler(int irq, void *dev_id)
                  {

                      os_printk(K_ERR, "u3d_vbus_fall_handler\n");

                      os_disableIrq(USB_IRQ);
                      g_usb_irq = 0;

                      os_printk(K_ERR, "power down u2/u3 ports, device module\n");
	#ifdef SUPPORT_U3	
                      os_setmsk(U3D_SSUSB_U3_CTRL_0P, (SSUSB_U3_PORT_DIS | SSUSB_U3_PORT_PDN));
 	#endif
                      os_setmsk(U3D_SSUSB_U2_CTRL_0P, (SSUSB_U2_PORT_DIS | SSUSB_U2_PORT_PDN));
                      os_setmsk(U3D_SSUSB_IP_PW_CTRL2, SSUSB_IP_DEV_PDN);

                      //reset the last request whether its serviced or not
                      u3d_rst_request();

                      os_writel(FPGA_REG, (os_readl(FPGA_REG) &~ VBUS_MSK ) | VBUS_FALL_BIT);
                      return IRQ_HANDLED;
                  }
#endif

                  irqreturn_t u3d_inter_handler(int irq, void *dev_id)
                           {

                               DEV_UINT32 dwIntrUsbValue;
                               DEV_UINT32 dwDmaIntrValue;
                               DEV_UINT32 dwIntrEPValue;
                               DEV_UINT16 wIntrTxValue;
                               DEV_UINT16 wIntrRxValue;
                               DEV_UINT32 wIntrQMUValue;
                               DEV_UINT32 wIntrQMUDoneValue;
                               DEV_UINT32 dwLtssmValue;
                               DEV_UINT32 ep_num;
                               DEV_UINT32 dwTemp;
                               DEV_UINT32 dwLinkIntValue;
                               //	DEV_UINT32 timer;
                               DEV_UINT32 dwLV1Int;
                               DEV_UINT32 dwOtgIntValue;

                               dwLV1Int = os_readl(U3D_LV1ISR);

                               if(dwLV1Int & MAC2_INTR)
                               {
                                   dwIntrUsbValue = os_readl(U3D_COMMON_USB_INTR) & os_readl(U3D_COMMON_USB_INTR_ENABLE);
                               }
                               else
                               {
                                   dwIntrUsbValue = 0;
                               }

	#ifdef SUPPORT_U3
                               if(os_readl(U3D_LV1ISR) & MAC3_INTR)
                               {
                                   dwLtssmValue = os_readl(U3D_LTSSM_INTR) & os_readl(U3D_LTSSM_INTR_ENABLE);
                               }
                               else
                               {
                                   dwLtssmValue = 0;
                               }
	#else
                               dwLtssmValue = 0;
	#endif

                               dwDmaIntrValue = os_readl(U3D_DMAISR)& os_readl(U3D_DMAIER);
                               dwIntrEPValue = os_readl(U3D_EPISR)& os_readl(U3D_EPIER);
                               wIntrQMUValue = os_readl(U3D_QISAR1);
                               wIntrQMUDoneValue = os_readl(U3D_QISAR0) & os_readl(U3D_QIER0);
                               wIntrTxValue = dwIntrEPValue&0xFFFF;
                               wIntrRxValue = (dwIntrEPValue>>16);
                               dwLinkIntValue = os_readl(U3D_DEV_LINK_INTR) & os_readl(U3D_DEV_LINK_INTR_ENABLE);
	#ifdef SUPPORT_OTG
                               dwOtgIntValue = os_readl(U3D_SSUSB_OTG_STS);
	#else
                               dwOtgIntValue = 0;
	#endif

                               os_printk(K_DEBUG,"wIntrQMUDoneValue :%x\n",wIntrQMUDoneValue);
                               os_printk(K_DEBUG,"dwIntrEPValue :%x\n",dwIntrEPValue);

                               os_printk(K_DEBUG,"Interrupt: IntrUsb [%x] IntrTx[%x] IntrRx [%x] IntrDMA[%x] IntrQMU [%x] IntrLTSSM [%x]\r\n", dwIntrUsbValue, wIntrTxValue, wIntrRxValue, dwDmaIntrValue, wIntrQMUValue, dwLtssmValue);

                               if( !(dwIntrUsbValue || dwIntrEPValue || dwDmaIntrValue || wIntrQMUValue || dwLtssmValue || wIntrQMUDoneValue ||dwOtgIntValue) )
                               {
                                   os_printk(K_WARNIN,"[NULL INTR] REG_INTRL1 = 0x%08X\n",(DEV_UINT32)os_readl(U3D_LV1ISR));
                               }

                               os_writel(U3D_QISAR0, wIntrQMUDoneValue);

                               if(os_readl(U3D_LV1ISR) & MAC2_INTR){
                                   os_writel(U3D_COMMON_USB_INTR, dwIntrUsbValue);
                               }
	#ifdef SUPPORT_U3
                               if(os_readl(U3D_LV1ISR) & MAC3_INTR){
                                   os_writel(U3D_LTSSM_INTR, dwLtssmValue);
                               }
	#endif
                               os_writel(U3D_EPISR, dwIntrEPValue);
                               os_writel(U3D_DMAISR, dwDmaIntrValue);
                               os_writel(U3D_DEV_LINK_INTR, dwLinkIntValue);
                               if (dwLtssmValue)
                                   os_printk(K_INFO,"dwLtssmValue : %x\n",dwLtssmValue);
                               if (dwIntrUsbValue)
                                   os_printk(K_INFO,"dwIntrUsbValue : %x\n",dwIntrUsbValue);
                               if (dwDmaIntrValue)
                                   os_printk(K_INFO,"dwDmaIntrValue : %x\n",dwDmaIntrValue);
                               if (wIntrTxValue)
                                   os_printk(K_INFO,"wIntrTxValue : %x\n",wIntrTxValue);
                               if (wIntrRxValue)
                                   os_printk(K_INFO,"wIntrRxValue : %x\n",wIntrRxValue);
                               if (wIntrQMUValue)
                                   os_printk(K_INFO,"wIntrQMUValue : %x\n",wIntrQMUValue);
                               if (wIntrQMUDoneValue)
                                   os_printk(K_INFO,"wIntrQMUDoneValue : %x\n",wIntrQMUDoneValue);
                               if (dwLinkIntValue)
                                   os_printk(K_INFO,"dwLinkIntValue : %x\n",dwLinkIntValue);

                               if (!(dwLtssmValue | dwIntrUsbValue | dwDmaIntrValue | wIntrTxValue | wIntrRxValue | wIntrQMUValue | wIntrQMUDoneValue | dwLinkIntValue||dwOtgIntValue))
                               {
                                   return IRQ_HANDLED;
                               }

                               if(wIntrQMUDoneValue){
                                   qmu_proc(wIntrQMUDoneValue);
                               }
                               if(wIntrQMUValue){
                                   qmu_handler(wIntrQMUValue);
                               }

                               if (dwLinkIntValue & SSUSB_DEV_SPEED_CHG_INTR)
                               {
                                   os_printk(K_ALET,"Speed Change Interrupt\n");

                                   dwTemp = os_readl(U3D_DEVICE_CONF) & SSUSB_DEV_SPEED;
                                   switch (dwTemp)
                                   {
                                   case SSUSB_SPEED_FULL:
#ifdef POWER_SAVING_MODE
                                      /*   when link enters HS/FS, pdn u3 port so that ip_sleep may assert*/
                                      mu3d_hal_pdn_ip_port(0,0,1,0);
#endif
                                       os_printk(K_ALET,"FS\n");
                                       break;
                                   case SSUSB_SPEED_HIGH:
#ifdef POWER_SAVING_MODE
                                      /*   when link enters HS/FS, u3 port so that ip_sleep may assert*/
                                      mu3d_hal_pdn_ip_port(0,0,1,0);
#endif								   	
                                       os_printk(K_ALET,"HS\n");
                                       break;
                                   case SSUSB_SPEED_SUPER:
#ifdef POWER_SAVING_MODE
                                      /*   when link enters HS/FS, dis u2  port so that ip_sleep may assert*/
                                      mu3d_hal_pdn_ip_port(0,1,0,1);
#endif										   	
                                       os_printk(K_ALET,"SS\n");
                                       break;
                                   default:
                                       os_printk(K_ALET,"Invalid\n");
                                   }
                               }

                               /* Check for reset interrupt */
                               if (dwIntrUsbValue & RESET_INTR){
                                   os_printk(K_ALET,"Reset Interrupt\n");

                                   if(os_readl(U3D_POWER_MANAGEMENT) & HS_MODE){
                                       os_printk(K_WARNIN, "Device: High-speed mode\n");
                                       g_usb_status.speed = SSUSB_SPEED_HIGH;
                                   }
                                   else{
                                       os_printk(K_WARNIN, "Device: Full-speed mode\n");
                                       g_usb_status.speed = SSUSB_SPEED_FULL;
                                   }
                                   g_usb_status.reset_received = 1;
		#ifdef SUPPORT_OTG
                                   g_otg_reset = 1;
                                   os_printk(K_ERR,"RESET_INTR\n");
                                   os_printk(K_ERR, "clear OTG events\n");

                                   g_otg_config = 0;
                                   g_otg_srp_reqd = 0;
                                   g_otg_hnp_reqd = 0;
                                   g_otg_b_hnp_enable = 0;

                                   g_otg_vbus_chg = 0;
                                   g_otg_suspend = 0;
                                   g_otg_resume = 0;
                                   g_otg_connect = 0;
                                   g_otg_disconnect = 0;
                                   g_otg_chg_a_role_b = 0;
                                   g_otg_attach_b_role = 0;
                                   os_clrmsk(U3D_DEVICE_CONTROL, HOSTREQ);

		#endif

                                   //leave from suspend state after reset
		#ifdef POWER_SAVING_MODE
		#ifdef SUPPORT_U3
                                   os_writel(U3D_SSUSB_U3_CTRL_0P, os_readl(U3D_SSUSB_U3_CTRL_0P) &~ SSUSB_U3_PORT_PDN);
		#endif
                                   os_writel(U3D_SSUSB_U2_CTRL_0P, os_readl(U3D_SSUSB_U2_CTRL_0P) &~ SSUSB_U2_PORT_PDN);
                                   os_writel(U3D_SSUSB_IP_PW_CTRL2, os_readl(U3D_SSUSB_IP_PW_CTRL2) &~ SSUSB_IP_DEV_PDN);
		#endif		
                                   g_usb_status.suspend = 0;

                                   //set device address to 0 after reset
                                   u3d_set_address(0);
                               }

	#ifdef SUPPORT_U3
                               if (dwLtssmValue){
                                   if(dwLtssmValue & SS_DISABLE_INTR){
                                       os_printk(K_ALET,"Device: SS Disable, %d\n",
                                                 (os_readl(U3D_LTSSM_INFO) & DISABLE_CNT) >> DISABLE_CNT_OFST);

			#ifdef U2_U3_SWITCH
			#ifdef POWER_SAVING_MODE
                                       os_writel(U3D_SSUSB_U2_CTRL_0P, os_readl(U3D_SSUSB_U2_CTRL_0P) &~ SSUSB_U2_PORT_DIS);
			#endif

			#ifdef U2_U3_SWITCH_AUTO
                                       os_setmsk(U3D_POWER_MANAGEMENT, SUSPENDM_ENABLE);
			#else
                                       mu3d_hal_u2dev_connect();
			#endif

                                       u3d_ep0en();
			#endif

                                       dwLtssmValue =0;
                                   }
                                   if(dwLtssmValue & SS_INACTIVE_INTR){
                                       os_printk(K_NOTICE,"****[WARNING]****:SS_INACTIVE_INTR\n");
                                       os_printk(K_NOTICE, "LTSSM=%02x\n", os_readl(U3D_LINK_STATE_MACHINE) & LTSSM);
                                       //os_writel(U3D_USB3_CONFIG, 0);
                                       //os_ms_delay(50);
                                       //os_writel(U3D_USB3_CONFIG, USB3_EN);
                                   }
                                   if(dwLtssmValue & LOOPBACK_INTR){
                                       os_printk(K_NOTICE,"Device: LOOPBACK_INTR\n");
                                       os_printk(K_NOTICE, "LTSSM=%02x\n", os_readl(U3D_LINK_STATE_MACHINE) & LTSSM);
                                   }
                                   if(dwLtssmValue & RECOVERY_INTR){
                                       os_printk(K_NOTICE,"Device: RECOVERY_INTR\n");
                                       os_printk(K_NOTICE, "LTSSM=%02x\n", os_readl(U3D_LINK_STATE_MACHINE) & LTSSM);
                                   }
                                   if(dwLtssmValue & HOT_RST_INTR){
                                       g_hot_rst_cnt++;
                                       os_printk(K_NOTICE,"Device: Hot Reset, %x\n", g_hot_rst_cnt);
                                       os_printk(K_NOTICE, "LTSSM=%02x\n", os_readl(U3D_LINK_STATE_MACHINE) & LTSSM);
                                   }
                                   if(dwLtssmValue & RXDET_SUCCESS_INTR){
                                       os_printk(K_NOTICE,"Device: RX Detect Success\n");
                                   }
                                   if(dwLtssmValue & WARM_RST_INTR){
                                       g_warm_rst_cnt++;
                                       os_printk(K_NOTICE,"Device: Warm Reset, %x\n", g_warm_rst_cnt);
                                   }
                                   if(dwLtssmValue & ENTER_U0_INTR){
                                       os_printk(K_ERR,"Device: Enter U0\n");
                                       g_usb_status.enterU0 = 1;
                                       g_usb_status.speed = SSUSB_SPEED_SUPER;
                                   }

		#ifndef EXT_VBUS_DET
                                   if(dwLtssmValue & VBUS_RISE_INTR){
                                       os_printk(K_ERR,"Device: Vbus Rise\n");
                                       g_usb_status.vbus_valid= 1;
                                   }
                                   if(dwLtssmValue & VBUS_FALL_INTR){
                                       os_printk(K_ERR,"Device: Vbus Fall\n");
                                       g_usb_status.vbus_valid= 0;

			#ifdef U2_U3_SWITCH
                                       os_printk(K_ERR, "SOFTCONN = 0\n");
                                       mu3d_hal_u2dev_disconn();

                                       //Reset HW disable_cnt to 0
                                       os_printk(K_ERR, "Toggle USB3_EN\n");
                                       os_writel(U3D_USB3_CONFIG, 0);
                                       os_ms_delay(50);
                                       os_writel(U3D_USB3_CONFIG, USB3_EN);
			#endif			
                                   }
		#endif

                                   if(dwLtssmValue & ENTER_U1_INTR){
                                       os_printk(K_NOTICE, "enter U1\n");
                                       g_usb_status.suspend = 1;
                                   }
                                   if(dwLtssmValue & ENTER_U2_INTR){
                                       os_printk(K_NOTICE, "enter U2\n");
                                       g_usb_status.suspend = 1;
                                   }
                                   if(dwLtssmValue & ENTER_U3_INTR){
                                       os_printk(K_NOTICE, "enter U3\n");
                                       g_usb_status.suspend = 1;

#ifdef POWER_SAVING_MODE
                                       os_writel(U3D_SSUSB_U3_CTRL_0P, os_readl(U3D_SSUSB_U3_CTRL_0P) | SSUSB_U3_PORT_PDN);
                                       os_writel(U3D_SSUSB_IP_PW_CTRL2, os_readl(U3D_SSUSB_IP_PW_CTRL2) | SSUSB_IP_DEV_PDN);
#endif
                                   }
                                   if(dwLtssmValue & EXIT_U1_INTR){
                                       g_usb_status.suspend = 0;
                                   }
                                   if(dwLtssmValue & EXIT_U2_INTR){
                                       g_usb_status.suspend = 0;
                                   }
                                   if(dwLtssmValue & EXIT_U3_INTR){
                                       g_usb_status.suspend = 0;
#ifdef POWER_SAVING_MODE
                                       os_writel(U3D_SSUSB_U3_CTRL_0P, os_readl(U3D_SSUSB_U3_CTRL_0P) &~ SSUSB_U3_PORT_PDN);
                                       os_writel(U3D_SSUSB_IP_PW_CTRL2, os_readl(U3D_SSUSB_IP_PW_CTRL2) &~ SSUSB_IP_DEV_PDN);
                                       while(!(os_readl(U3D_SSUSB_IP_PW_STS1) & SSUSB_U3_MAC_RST_B_STS));
#endif

                                   }
#ifndef POWER_SAVING_MODE
                                   if(dwLtssmValue & U3_RESUME_INTR){
                                       g_usb_status.suspend= 0;
                                       os_writel(U3D_SSUSB_U3_CTRL_0P, os_readl(U3D_SSUSB_U3_CTRL_0P) &~ SSUSB_U3_PORT_PDN);
                                       os_writel(U3D_SSUSB_IP_PW_CTRL2, os_readl(U3D_SSUSB_IP_PW_CTRL2) &~ SSUSB_IP_DEV_PDN);
                                       while(!(os_readl(U3D_SSUSB_IP_PW_STS1) & SSUSB_U3_MAC_RST_B_STS));
                                       os_writel(U3D_LINK_POWER_CONTROL, os_readl(U3D_LINK_POWER_CONTROL) | UX_EXIT);
                                   }
#endif
                               }
	#endif

                               if(dwIntrUsbValue & DISCONN_INTR){
		#ifdef SUPPORT_OTG
                                   //chiachun
                                   g_otg_disconnect = 1;
                                   //chiachun...
		#endif

                                   os_printk(K_ALET, "Device: Disconnect\n");
#ifdef POWER_SAVING_MODE
                                   os_writel(U3D_SSUSB_IP_PW_CTRL2, os_readl(U3D_SSUSB_IP_PW_CTRL2) & ~SSUSB_IP_DEV_PDN);
		#ifdef SUPPORT_U3
                                   os_writel(U3D_SSUSB_U3_CTRL_0P, os_readl(U3D_SSUSB_U3_CTRL_0P) & ~SSUSB_U3_PORT_PDN);
		#endif
                                   os_writel(U3D_SSUSB_U2_CTRL_0P, os_readl(U3D_SSUSB_U2_CTRL_0P) & ~SSUSB_U2_PORT_PDN);
#endif
                               }

                               if(dwIntrUsbValue & CONN_INTR){
		#ifdef SUPPORT_OTG
                                   g_otg_connect = 1;
		#endif
                                   os_printk(K_ALET, "Device: Connect\n");
                               }

                               if(dwIntrUsbValue & SUSPEND_INTR){
                                   os_printk(K_ALET, "Suspend Interrupt\n");
#ifdef POWER_SAVING_MODE
                                   os_writel(U3D_SSUSB_U2_CTRL_0P, os_readl(U3D_SSUSB_U2_CTRL_0P) | SSUSB_U2_PORT_PDN);
                                   os_writel(U3D_SSUSB_IP_PW_CTRL2, os_readl(U3D_SSUSB_IP_PW_CTRL2) | SSUSB_IP_DEV_PDN);
#endif
                                   g_usb_status.suspend = 1;
#ifdef SUPPORT_OTG
                                   g_otg_suspend = 1;
                                   os_printk(K_ERR, "OTG: suspend\n");
                                   os_printk(K_ERR, "[OTG_D] Probe data = 0x%x\n",(os_readl(SSUSB_SIFSLV_IPPC_BASE+0xc4)));
#endif
                               }

                               if(dwIntrUsbValue & LPM_INTR)
                               {
                                   os_printk(K_NOTICE,"LPM Interrupt\n");

                                   dwTemp = os_readl(U3D_USB20_LPM_PARAMETER);
                                   os_printk(K_NOTICE, "BESL: %x, %x <= %x <= %x\n", (dwTemp>>BESL_OFST)&0xf, \
                                             (dwTemp>>BESLCK_OFST)&0xf, (dwTemp>>BESLCK_U3_OFST)&0xf, (dwTemp>>BESLDCK_OFST)&0xf);

                                   dwTemp = os_readl(U3D_POWER_MANAGEMENT);
                                   os_printk(K_NOTICE, "RWP: %x\n", (dwTemp&LPM_RWP)>>11);
#ifdef POWER_SAVING_MODE
                                   if(!((os_readl(U3D_POWER_MANAGEMENT) & LPM_HRWE))){
                                       os_writel(U3D_SSUSB_U2_CTRL_0P, os_readl(U3D_SSUSB_U2_CTRL_0P) | SSUSB_U2_PORT_PDN);
                                       os_writel(U3D_SSUSB_IP_PW_CTRL2, os_readl(U3D_SSUSB_IP_PW_CTRL2) | SSUSB_IP_DEV_PDN);
                                   }
#endif
                                   if (g_sw_rw)
                                   {
#ifdef POWER_SAVING_MODE
                                       os_writel(U3D_SSUSB_IP_PW_CTRL2, os_readl(U3D_SSUSB_IP_PW_CTRL2) & ~SSUSB_IP_DEV_PDN);
                                       os_writel(U3D_SSUSB_U2_CTRL_0P, os_readl(U3D_SSUSB_U2_CTRL_0P) & ~SSUSB_U2_PORT_PDN);
                                       while(!(os_readl(U3D_SSUSB_IP_PW_STS2) & SSUSB_U2_MAC_SYS_RST_B_STS));
#endif
                                       os_writel(U3D_USB20_MISC_CONTROL, os_readl(U3D_USB20_MISC_CONTROL) | LPM_U3_ACK_EN);// s/w LPM only

                                       //wait a while before remote wakeup, so xHCI PLS status is not affected
                                       os_ms_delay(20);
                                       os_writel(U3D_POWER_MANAGEMENT, os_readl(U3D_POWER_MANAGEMENT) | RESUME);

                                       os_printk(K_NOTICE, "RESUME: %d\n", os_readl(U3D_POWER_MANAGEMENT) & RESUME);
                                   }
                               }

                               if(dwIntrUsbValue & LPM_RESUME_INTR){
		#ifdef SUPPORT_OTG
                                   g_otg_resume = 1;
		#endif
                                   if(!(os_readl(U3D_POWER_MANAGEMENT) & LPM_HRWE)){
#ifdef POWER_SAVING_MODE
                                       os_writel(U3D_SSUSB_IP_PW_CTRL2, os_readl(U3D_SSUSB_IP_PW_CTRL2) & ~SSUSB_IP_DEV_PDN);
                                       os_writel(U3D_SSUSB_U2_CTRL_0P, os_readl(U3D_SSUSB_U2_CTRL_0P) & ~SSUSB_U2_PORT_PDN);
                                       while(!(os_readl(U3D_SSUSB_IP_PW_STS2) & SSUSB_U2_MAC_SYS_RST_B_STS));
#endif
                                       os_writel(U3D_USB20_MISC_CONTROL, os_readl(U3D_USB20_MISC_CONTROL) | LPM_U3_ACK_EN);// s/w LPM only
                                   }
                               }

                               /* Check for resume from suspend mode */
                               if (dwIntrUsbValue & RESUME_INTR){
                                   os_printk(K_NOTICE,"Resume Interrupt\n");
#ifdef POWER_SAVING_MODE
                                   os_writel(U3D_SSUSB_IP_PW_CTRL2, os_readl(U3D_SSUSB_IP_PW_CTRL2) & ~SSUSB_IP_DEV_PDN);
                                   os_writel(U3D_SSUSB_U2_CTRL_0P, os_readl(U3D_SSUSB_U2_CTRL_0P) & ~SSUSB_U2_PORT_PDN);
                                   while(!(os_readl(U3D_SSUSB_IP_PW_STS2) & SSUSB_U2_MAC_SYS_RST_B_STS));
#endif
                                   g_usb_status.suspend = 0;
                               }



	#ifdef SUPPORT_OTG
                               if (dwOtgIntValue)
                               {
                                   os_printk(K_ERR, "OTG HW: %x\n", dwOtgIntValue);
                                   if (dwOtgIntValue & VBUS_CHG_INTR)
                                   {
                                       g_otg_vbus_chg = 1;
                                       //chiachun
#if 0
                                       if(!(dwOtgIntValue & SSUSB_VBUS_VALID)){
                                           g_otg_disconnect = 1;
                                       }
#endif
                                       //chiachun...
                                       os_printk(K_ERR, "OTG: VBUS_CHG_INTR\n");
                                       os_setmsk(U3D_SSUSB_OTG_STS_CLR, SSUSB_VBUS_INTR_CLR);
                                   }

                                   //this interrupt is issued when B device becomes device
                                   if (dwOtgIntValue & SSUSB_CHG_B_ROLE_B)
                                   {
                                       g_otg_chg_b_role_b = 1;
                                       os_printk(K_ERR, "OTG: CHG_B_ROLE_B\n");
                                       os_setmsk(U3D_SSUSB_OTG_STS_CLR, SSUSB_CHG_B_ROLE_B_CLR);
                                       os_setmsk(U3D_DMAIESR, EP0DMAIECR | TXDMAIECR | RXDMAIECR);

			#if 0
                                       os_writel(U3D_SSUSB_PRB_CTRL1, 0x00000000);
                                       os_writel(U3D_SSUSB_PRB_CTRL2, 0x00000000);
                                       os_writel(U3D_SSUSB_PRB_CTRL3, 0x0f0f0f0f);

                                       timer = 0;
                                       os_ms_delay(1);
                                       while ((os_readl(U3D_SSUSB_PRB_CTRL5) & (1<<5)))
                                       {
                                           timer++;
                                           os_ms_delay(10);
                                           if (!(timer % 5))
                                               os_printk(K_ERR, "DMA not ready(%X) %d\n", os_readl(U3D_SSUSB_PRB_CTRL5), timer);

                                           if (timer == 100)
                                               break;
                                       }
			#else
                                       timer = 0;
                                       os_ms_delay(1);

                                       while (os_readl(U3D_SSUSB_OTG_STS) & SSUSB_XHCI_MAS_DMA_REQ)
                                       {
                                           timer++;
                                           os_ms_delay(10);
                                           if (!(timer % 5))
                                               os_printk(K_ERR, "DMA not ready(%X) %d\n", os_readl(U3D_SSUSB_OTG_STS) & SSUSB_XHCI_MAS_DMA_REQ, timer);

                                           if (timer == 100)
                                               break;
                                       }
			#endif

                                       //switch DMA module to device
                                       os_printk(K_ERR, "Switch DMA to device\n");
                                       while (os_readl(U3D_SSUSB_OTG_STS) & SSUSB_XHCI_MAS_DMA_REQ)
                                       {
                                           timer++;
                                           os_ms_delay(10);
                                           if (!(timer % 5))
                                               os_printk(K_ERR, "DMA not ready(%X) %d\n", os_readl(U3D_SSUSB_OTG_STS) & SSUSB_XHCI_MAS_DMA_REQ, timer);

                                           if (timer == 100)
                                               break;
                                       }
                                       os_clrmsk(U3D_SSUSB_U2_CTRL_0P, SSUSB_U2_PORT_HOST_SEL);
                                   }

                                   //this interrupt is issued when B device becomes host
                                   if (dwOtgIntValue & SSUSB_CHG_A_ROLE_B)
                                   {
                                       g_otg_chg_a_role_b = 1;
                                       os_printk(K_ERR, "g_otg_chg_a_role_b = 1\n");
                                       os_printk(K_ERR, "OTG: CHG_A_ROLE_B\n");
                                       os_setmsk(U3D_SSUSB_OTG_STS_CLR, SSUSB_CHG_A_ROLE_B_CLR);
                                   }

                                   //this interrupt is issued when IDDIG reads B
                                   if (dwOtgIntValue & SSUSB_ATTACH_B_ROLE)
                                   {
                                       g_otg_attach_b_role = 1;
                                       os_printk(K_ERR, "OTG: CHG_ATTACH_B_ROLE\n");
                                       os_setmsk(U3D_SSUSB_OTG_STS_CLR, SSUSB_ATTACH_B_ROLE_CLR);

                                       //switch DMA module to device
                                       os_printk(K_ERR, "Switch DMA to device\n");
                                       os_clrmsk(U3D_SSUSB_U2_CTRL_0P, SSUSB_U2_PORT_HOST_SEL);
                                   }
                                   if (dwOtgIntValue & SSUSB_DEV_USBRST_INTR)
                                   {

                                       os_printk(K_ERR, "SSUSB_DEV_USBRST_INTR\n");
                                       os_setmsk(U3D_SSUSB_OTG_STS_CLR, SSUSB_DEV_USBRST_INTR);
                                   }

                               }
	#endif



                               if(dwDmaIntrValue){
                                   u3d_dma_handler(dwDmaIntrValue);
                               }

                               if((wIntrTxValue & 1) || (wIntrRxValue & 1)){
                                   if (wIntrRxValue & 1)
                                   {
                                       os_printk(K_ERR, "Service SETUPEND\n");
                                   }
                                   os_printk(K_INFO,"EP0CSR :%x\n",os_readl(U3D_EP0CSR));
                                   os_printk(K_INFO,"U3D_RXCOUNT0 :%x\n",os_readl(U3D_RXCOUNT0));

                                   u3d_ep0_handler();
                               }

                               if(wIntrTxValue){
                                   for(ep_num = 1; ep_num <= TX_FIFO_NUM; ep_num++){
                                       if(wIntrTxValue & (1<<ep_num)){

                                           if(USB_ReadCsr32(U3D_TX1CSR0, ep_num) & TX_SENTSTALL){
                                               USB_WriteCsr32(U3D_TX1CSR0, ep_num, USB_ReadCsr32(U3D_TX1CSR0, ep_num) | TX_SENTSTALL);
                                           }
                                           else{
                                               u3d_epx_handler(ep_num, USB_TX);
                                               g_tx_intr_cnt++;
                                           }
                                       }
                                   }
                               }
                               if(wIntrRxValue){
                                   for(ep_num = 1; ep_num <= RX_FIFO_NUM; ep_num++){
                                       if(wIntrRxValue & (1<<ep_num)){
                                           os_printk(K_ERR,"wIntrRxValue :%x\n",wIntrRxValue);

                                           if(USB_ReadCsr32(U3D_RX1CSR0, ep_num) & RX_SENTSTALL){
                                               USB_WriteCsr32(U3D_RX1CSR0, ep_num, USB_ReadCsr32(USB_RX, ep_num) | RX_SENTSTALL);
                                           }
                                           else{
                                               u3d_epx_handler(ep_num, USB_RX);
                                               g_rx_intr_cnt++;
                                           }
                                       }
                                   }
                               }

                               os_printk(K_INFO, "[END INTERRUPT]\n");

                               return IRQ_HANDLED;
                           }


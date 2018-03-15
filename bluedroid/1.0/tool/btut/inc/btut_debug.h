#ifndef __BTUT_DEBUG_H__
#define __BTUT_DEBUG_H__

#define BTUT_LOG_LV_VBS  (1 << 0)
#define BTUT_LOG_LV_INF  (1 << 1)
#define BTUT_LOG_LV_DBG  (1 << 2)
#define BTUT_LOG_LV_WRN  (1 << 3)
#define BTUT_LOG_LV_ERR  (1 << 4)
#define BTUT_LOG_MASK    (BTUT_LOG_LV_VBS|BTUT_LOG_LV_INF|BTUT_LOG_LV_DBG|BTUT_LOG_LV_WRN|BTUT_LOG_LV_ERR)

#define BTUT_LOG_FLAG_COLOR             (1 << 8)
#define BTUT_LOG_FLAG_TIMESTAMP         (1 << 9)

#define BTUT_Logv(fmt, ...) BTUT_Log(BTUT_LOG_LV_VBS, "<V> " fmt , ## __VA_ARGS__)
#define BTUT_Logi(fmt, ...) BTUT_Log(BTUT_LOG_LV_INF, "<I> " fmt , ## __VA_ARGS__)
#define BTUT_Logd(fmt, ...) BTUT_Log(BTUT_LOG_LV_DBG, "<D> " fmt , ## __VA_ARGS__)
#define BTUT_Logw(fmt, ...) BTUT_Log(BTUT_LOG_LV_WRN, "<W> %s(). " fmt , __func__, ## __VA_ARGS__)
#define BTUT_Loge(fmt, ...) BTUT_Log(BTUT_LOG_LV_ERR, "<E> %s()[%d] : " fmt , __func__, __LINE__, ## __VA_ARGS__)

void BTUT_Log(unsigned char lv, const char *fmt, ...);
void BTUT_Log_SetFlag(unsigned short flag);

#endif  //__BTUT_DEBUG_H__

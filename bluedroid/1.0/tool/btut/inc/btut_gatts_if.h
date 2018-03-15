#ifndef __BTUT_GATTS_IF_H__
#define __BTUT_GATTS_IF_H__

#include <hardware/bt_gatt.h>

#define CMD_KEY_GATTS        "GATTS"
#define BTUT_GATTS_SERVER_UUID  "49557E51-D815-11E4-8830-0800200C9A66"

int btut_gatts_init(const btgatt_server_interface_t *interface);

#endif /* __BTUT_GATTS_IF_H__ */

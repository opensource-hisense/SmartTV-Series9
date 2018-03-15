#ifndef __BTUT_GATTC_IF_H__
#define __BTUT_GATTC_IF_H__

#include <hardware/bt_gatt.h>

#define CMD_KEY_GATTC        "GATTC"
#define BTUT_GATTC_APP_UUID  "49557E50-D815-11E4-8830-0800200C9A66"

int btut_gattc_init(const btgatt_client_interface_t *interface);
int btut_gattc_deinit();

#endif /* __BTUT_GATTC_IF_H__ */

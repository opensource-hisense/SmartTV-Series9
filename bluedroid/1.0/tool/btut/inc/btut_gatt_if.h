#ifndef __BTUT_GATT_IF_H__
#define __BTUT_GATT_IF_H__

#include <hardware/bt_gatt.h>

#define CMD_KEY_GATT        "GATT"

typedef struct
{
     unsigned char type;
     unsigned char len;
     void *value;
} btut_gatt_adv_data_item_t;

typedef struct
{
    btut_gatt_adv_data_item_t data[10];
} btut_gatt_adv_data_t;

int btut_gatt_init();
int btut_gatt_deinit();
void btut_gatt_btaddr_stoh(char *btaddr_s, bt_bdaddr_t *bdaddr_h);
void btut_gatt_uuid_stoh(char *uuid_s,  bt_uuid_t *uuid);
bool btut_gatt_is_uuid_equal(bt_uuid_t *uuid1,  bt_uuid_t *uuid2);
void btut_gatt_print_uuid (bt_uuid_t* uuid, char* uuid_s);
void btut_gatt_decode_adv_data (uint8_t* adv_data, btut_gatt_adv_data_t *parse_data);

#endif /* __BTUT_GATT_IF_H__ */

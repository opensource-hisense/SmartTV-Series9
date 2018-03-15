#include <string.h>
#include "btut_avrcp_tg_if.h"

extern const void *btut_gap_get_profile_interface(const char *profile_id);

static void btut_rc_tg_remote_features_cb (bt_bdaddr_t *bd_addr, btrc_remote_features_t features);
static void btut_rc_tg_get_play_status_cb();
static void btut_rc_tg_list_player_app_attr_cb();
static void btut_rc_tg_list_player_app_values_cb (btrc_player_attr_t attr_id);
static void btut_rc_tg_get_player_app_value_cb (uint8_t num_attr, btrc_player_attr_t *p_attrs);
static void btut_rc_tg_get_player_app_attrs_text_cb (uint8_t num_attr, btrc_player_attr_t *p_attrs);
static void btut_rc_tg_get_player_app_values_text_cb (uint8_t attr_id, uint8_t num_val, uint8_t *p_vals);
static void btut_rc_tg_set_player_app_value_cb (btrc_player_settings_t *p_vals);
static void btut_rc_tg_get_element_attr_cb (uint8_t num_attr, btrc_media_attr_t *p_attrs);
static void btut_rc_tg_register_notification_cb (btrc_event_id_t event_id, uint32_t param);
static void btut_rc_tg_volume_change_cb (uint8_t volume, uint8_t ctype);
static void btut_rc_tg_passthrough_cmd_cb (int id, int key_state);

static btrc_interface_t *g_bt_rc_tg_interface = NULL;
static btrc_callbacks_t g_bt_rc_tg_callbacks =
{
    sizeof(btrc_callbacks_t),
    btut_rc_tg_remote_features_cb,
    btut_rc_tg_get_play_status_cb,
    btut_rc_tg_list_player_app_attr_cb,
    btut_rc_tg_list_player_app_values_cb,
    btut_rc_tg_get_player_app_value_cb,
    btut_rc_tg_get_player_app_attrs_text_cb,
    btut_rc_tg_get_player_app_values_text_cb,
    btut_rc_tg_set_player_app_value_cb,
    btut_rc_tg_get_element_attr_cb,
    btut_rc_tg_register_notification_cb,
    btut_rc_tg_volume_change_cb,
    btut_rc_tg_passthrough_cmd_cb,
};

static void btut_rc_tg_remote_features_cb(bt_bdaddr_t *bd_addr, btrc_remote_features_t features)
{
    BTUT_Logd("[AVRCP] %s() features = 0x%x CT version = 0x%x", __func__, features);
}

static void btut_rc_tg_get_play_status_cb()
{
    BTUT_Logd("[AVRCP] %s() ",__func__);
}

static void btut_rc_tg_list_player_app_attr_cb()
{
    BTUT_Logd("[AVRCP] %s() ", __func__);
}

static void btut_rc_tg_list_player_app_values_cb(btrc_player_attr_t attr_id)
{
    BTUT_Logd("[AVRCP] %s() attr_id = 0x%x", __func__, attr_id);
}

static void btut_rc_tg_get_player_app_value_cb(uint8_t num_attr, btrc_player_attr_t *p_attrs)
{
    BTUT_Logd("[AVRCP] %s() p_attrs = 0x%x", __func__, *p_attrs);
}

static void btut_rc_tg_get_player_app_attrs_text_cb(uint8_t num_attr, btrc_player_attr_t *p_attrs)
{
    BTUT_Logd("[AVRCP] %s() p_attrs = 0x%x", __func__, *p_attrs);
}

static void btut_rc_tg_get_player_app_values_text_cb(uint8_t attr_id, uint8_t num_val, uint8_t *p_vals)
{
    BTUT_Logd("[AVRCP] %s() attr_id = 0x%x, num_val = 0x%x, p_vals = 0x%x", __func__, attr_id, num_val,  p_vals);
}

static void btut_rc_tg_set_player_app_value_cb(btrc_player_settings_t *p_vals)
{
    BTUT_Logd("[AVRCP] %s() p_vals->num_attr = 0x%x, p_vals->attr_ids = 0x%x, p_vals->attr_values = 0x%x", __func__,
        p_vals->num_attr, p_vals->attr_ids, p_vals->attr_values);
}

static void btut_rc_tg_get_element_attr_cb(uint8_t num_attr, btrc_media_attr_t *p_attrs)
{
    BTUT_Logd("[AVRCP] %s() num_attr = 0x%x, p_attrs = 0x%x",__func__, num_attr, *p_attrs);
}

static void btut_rc_tg_register_notification_cb(btrc_event_id_t event_id, uint32_t param)
{
    btrc_register_notification_t p;

    BTUT_Logd("[AVRCP] %s() event_id = 0x%x, param = 0x%x", __func__, event_id, param);

    switch (event_id)
    {
    case BTRC_EVT_PLAY_STATUS_CHANGED:
        p.play_status = param;
        break;
    default:
        break;
    }
    g_bt_rc_tg_interface->register_notification_rsp(event_id, BTRC_NOTIFICATION_TYPE_INTERIM, &p);
}

static void btut_rc_tg_volume_change_cb(uint8_t volume, uint8_t ctype)
{
    BTUT_Logd("[AVRCP] %s() volume = 0x%x, ctype = 0x%x", __func__, volume, ctype);
}

static void btut_rc_tg_passthrough_cmd_cb(int id, int key_state)
{
    BTUT_Logd("[AVRCP] %s() id = 0x%x, key_state = 0x%x", __func__, id, key_state);
}

static BTUT_CLI bt_rc_tg_cli_commands[] =
{
    {NULL, NULL, NULL},
};

static int btut_rc_tg_cmd_handler(int argc, char **argv)
{
    BTUT_CLI *cmd, *match = NULL;
    int ret = 0;
    int count = 0;

    BTUT_Logd("[AVRCP] argc: %d, argv[0]: %s\n", argc, argv[0]);

    cmd = bt_rc_tg_cli_commands;
    while (cmd->cmd)
    {
        if (!strcmp(cmd->cmd, argv[0]))
        {
            match = cmd;
            count = 1;
            break;
        }
        cmd++;
    }

    if (count == 0)
    {
        BTUT_Logd("[AVRCP] Unknown command '%s'\n", argv[0]);
        btut_print_cmd_help(CMD_KEY_RC_TG, bt_rc_tg_cli_commands);
        ret = -1;
    }
    else
    {
        match->handler(argc - 1, &argv[1]);
    }

    return ret;
}

int btut_rc_tg_init()
{
    int ret = 0;
    BTUT_MOD rc_mod = {0};

    BTUT_Logd("[AVRCP] %s() \n", __func__);

    g_bt_rc_tg_interface = (btrc_interface_t *) btut_gap_get_profile_interface(BT_PROFILE_AV_RC_ID);
    if (g_bt_rc_tg_interface == NULL)
    {
        BTUT_Loge("[AVRCP] Failed to get AVRCP TG interface\n");
        return -1;
    }

    if (g_bt_rc_tg_interface->init(&g_bt_rc_tg_callbacks) != BT_STATUS_SUCCESS)
    {
        BTUT_Loge("[AVRCP] Failed to init AVRCP TG interface\n");
        return -1;
    }

    rc_mod.mod_id = BTUT_MOD_AVRCP_TG;
    strncpy(rc_mod.cmd_key, CMD_KEY_RC_TG, sizeof(rc_mod.cmd_key));
    rc_mod.cmd_handler = btut_rc_tg_cmd_handler;
    rc_mod.cmd_tbl = bt_rc_tg_cli_commands;

    ret = btut_register_mod(&rc_mod);
    BTUT_Logd("[AVRCP] btut_register_mod() for TG returns: %d\n", ret);

    return ret;
}

int btut_rc_tg_deinit()
{
    BTUT_Logd("[AVRCP] %s() \n", __func__);

    if (g_bt_rc_tg_interface == NULL)
    {
        BTUT_Loge("[AVRCP] Failed to get AVRCP TG interface\n");
        return -1;
    }

    g_bt_rc_tg_interface->cleanup();

    return 0;
}

#ifndef __BTUT_AVRCP_TG_IF_H__
#define __BTUT_AVRCP_TG_IF_H__

#include <hardware/bluetooth.h>
#include <hardware/bt_rc.h>

#include "btut_cli.h"
#include "btut_debug.h"

#define CMD_KEY_RC_TG     "AVRCP_TG"
#define AVRCP_CT_MAX_ATTR_NO 7

int btut_rc_tg_init(void);
int btut_rc_tg_deinit(void);

#define AVRCP_STATE_PRESSED 0
#define AVRCP_STATE_RELEASED 1

#define AVRCP_POP_POWER             0x40
#define AVRCP_POP_VOLUME_UP         0x41
#define AVRCP_POP_VOLUME_DOWN       0x42
#define AVRCP_POP_MUTE              0x43
#define AVRCP_POP_PLAY              0x44
#define AVRCP_POP_STOP              0x45
#define AVRCP_POP_PAUSE             0x46
#define AVRCP_POP_RECORD            0x47
#define AVRCP_POP_REWIND            0x48
#define AVRCP_POP_FAST_FORWARD      0x49
#define AVRCP_POP_EJECT             0x4A
#define AVRCP_POP_FORWARD           0x4B
#define AVRCP_POP_BACKWARD          0x4C

#define AVRCP_CHARSET_UTF8          0x006A

#endif /* __BTUT_AVRCP_TG_IF_H__ */

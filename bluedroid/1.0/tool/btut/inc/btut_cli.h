#ifndef __BTUT_CLI_H__
#define __BTUT_CLI_H__

#include <time.h>

#define BTUT_SUCCESS 0
#define BTUT_FAILED (-1)

#define BTUT_MAX_ARGS 25
#define BTUT_MAX_KEY_LEN    32

#define BTUT_MAX_PATH_LEN   256
#define BTUT_HISTORY_FILE   ".btut_history"

#define BTUT_MAX_MODULES    40

typedef struct btut_cli_t
{
    const char *cmd;
    int (*handler)(int argc, char *argv[]);
    const char *usage;
} BTUT_CLI;

typedef int (*BTUT_CMD_HANDLER)(int argc, char **argv);

enum
{
    BTUT_MOD_GAP = 0,
    BTUT_MOD_A2DP_SINK,
    BTUT_MOD_A2DP_SRC,
    BTUT_MOD_GATT_CLIENT,
    BTUT_MOD_GATT_SERVER,
    BTUT_MOD_HID,
    BTUT_MOD_AVRCP_CT,
    BTUT_MOD_AVRCP_TG,
    BTUT_MOD_CNF,
    BTUT_MOD_TOOLS,
    BTUT_MOD_NUM,
};

typedef struct btut_mod_t
{
    int mod_id;
    char cmd_key[BTUT_MAX_KEY_LEN];
    BTUT_CMD_HANDLER cmd_handler;
    BTUT_CLI *cmd_tbl;
} BTUT_MOD;

extern void btut_print_cmd_help(const char* prefix, BTUT_CLI *tbl);
extern int btut_register_mod(BTUT_MOD *mod);

#endif

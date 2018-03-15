#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <dlfcn.h>
#include <errno.h>

// CLI
#include "btut_cli.h"
#include "btut_debug.h"
#include "btut_tools_if.h"

static int driver_test(int argc, char *argv[]);

static BTUT_CLI bt_tools_cli_commands[] =
{
    { "driver_test", driver_test,
      "= driver enable disable write open test" },
    { NULL, NULL, NULL }
};

// For handling incoming commands from CLI.
int btut_tools_cmd_handler(int argc, char **argv)
{
    BTUT_CLI *cmd, *match = NULL;
    int ret = 0;
    int count;

    count = 0;
    cmd = bt_tools_cli_commands;

    BTUT_Logd("[Tools] argc: %d, argv[0]: %s\n", argc, argv[0]);

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
        BTUT_Logd("Unknown command '%s'\n", argv[0]);

        btut_print_cmd_help(CMD_KEY_TOOLS, bt_tools_cli_commands);
        ret = -1;
    }
    else
    {
        match->handler(argc - 1, &argv[1]);
    }

    return ret;
}

int btut_tools_init()
{
    int ret = 0;
    BTUT_MOD tools_mod = {0};

    // Register to btut_cli.
    tools_mod.mod_id = BTUT_MOD_TOOLS;
    strncpy(tools_mod.cmd_key, CMD_KEY_TOOLS, sizeof(tools_mod.cmd_key));
    tools_mod.cmd_handler = btut_tools_cmd_handler;
    tools_mod.cmd_tbl = bt_tools_cli_commands;

    ret = btut_register_mod(&tools_mod);
    BTUT_Logd("[Tools] btut_register_mod() returns: %d\n", ret);

    return ret;
}

/*
 * bluetooth driver test
 * enable disable write
 */
static int driver_test(int argc, char *argv[])
{
    typedef void (*rx_callback)();

    typedef int (*bt_enable)(int flag, rx_callback func_cb);
    bt_enable mtk_bt_enable = NULL;

    typedef int (*bt_write)(int bt_fd, unsigned char *buffer, unsigned short length);
    bt_write mtk_bt_write = NULL;

    typedef int (*bt_disable)(int bt_fd);
    bt_disable mtk_bt_disable = NULL;
    void *glib_handle = NULL;
    unsigned char buffer[51] = {0x03, 0x0, 0xe, 0x30 };
    int fd=0;

    glib_handle = dlopen("libbt-vendor.so", RTLD_LAZY);
    if (!glib_handle)
    {
        printf("%s\n", dlerror());
        goto error;
    }

    dlerror(); /* Clear any existing error */

    mtk_bt_enable = dlsym(glib_handle, "mtk_bt_enable");
    if (mtk_bt_enable)
    {
        fd = mtk_bt_enable(0, NULL);
    }

    mtk_bt_write= dlsym(glib_handle, "mtk_bt_write");
    if (mtk_bt_write)
    {
        mtk_bt_write(fd, buffer, sizeof (buffer));
    }

    mtk_bt_disable = dlsym(glib_handle, "mtk_bt_disable");
    if (mtk_bt_disable)
    {
        mtk_bt_disable(fd);
    }

error:
    if (glib_handle)
    {
        dlclose(glib_handle);
        glib_handle = NULL;
    }

    return 0;
}

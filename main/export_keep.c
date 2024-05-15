/**
 * @file export_keep.c
 * @author Letter (NevermindZZT@gmail.com)
 * @brief 
 * @version 1.0.0
 * @date 2024-05-10
 * @copyright (c) 2024 Letter All rights reserved.
 */
#include "shell.h"
#include "rtam.h"

static unsigned int keepShell = 0;

#define SHELL_CMD_USED(_name) \
        extern ShellCommand shellCommand##_name; \
        keepShell += (int) &shellCommand##_name

static unsigned int shellKeep(void)
{
    SHELL_CMD_USED(sysInfo);
    SHELL_CMD_USED(ps);
    SHELL_CMD_USED(hid_device);
    SHELL_CMD_USED(hid_send);
    // SHELL_CMD_USED(smartConfig);
    // SHELL_CMD_USED(wlanScan);
    // SHELL_CMD_USED(wlanConnect);

    SHELL_CMD_USED(lvglTest);
    SHELL_CMD_USED(lvglTest2);
    SHELL_CMD_USED(home);

    return keepShell;
}


static size_t keepRtam = 0;

#define RTAPP_USERD(_name) \
    extern RtApp rtApp##_name; \
    keepRtam += (size_t) &rtApp##_name

static unsigned int rtamKeep(void) {
    RTAPP_USERD(shell);
    RTAPP_USERD(gui);
    RTAPP_USERD(launcher);
    RTAPP_USERD(setting);
    RTAPP_USERD(test);

    return keepRtam;
}


void exportKeep(void)
{
    if (keepShell) {
        shellKeep();
    }
    if (keepRtam) {
        rtamKeep();
    }
}

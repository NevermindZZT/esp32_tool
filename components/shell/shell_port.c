/**
 * @file shell_port.c
 * @author Letter (NevermindZZT@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2019-02-22
 * 
 * @copyright (c) 2019 Letter
 * 
 */

#include "shell.h"
#include "freertos/FreeRTOS.h"
#include "driver/uart.h"


#define     SHELL_UART      UART_NUM_0

static void shellKeep(void);

unsigned int keep = 0;

Shell shell;
char shellBuffer[512];

/**
 * @brief 用户shell写
 * 
 * @param data 数据
 * @param len 数据长度
 * 
 * @return unsigned short 写入实际长度
 */
unsigned short userShellWrite(char *data, unsigned short len)
{
    return uart_write_bytes(SHELL_UART, (const char *)data, len);
}


/**
 * @brief 用户shell读
 * 
 * @param data 数据
 * @param len 数据长度
 * 
 * @return unsigned short 读取实际长度
 */
signed char userShellRead(char *data, unsigned short len)
{
    return uart_read_bytes(SHELL_UART, (uint8_t *)data, len, portMAX_DELAY);
}


/**
 * @brief 用户shell初始化
 * 
 */
void userShellInit(void)
{
    uart_config_t uartConfig = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(SHELL_UART, &uartConfig);
    uart_driver_install(SHELL_UART, 256 * 2, 0, 0, NULL, 0);
    shell.write = userShellWrite;
    shell.read = userShellRead;
    shellInit(&shell, shellBuffer, 512);

    xTaskCreate(shellTask, "shell", 8192, &shell, 10, NULL);

    if (keep) {
        shellKeep();
    }
}

static void test(void)
{
    shellPrint(shellGetCurrent(), "hello world\n");
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(1)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC),
test, test, test);

SHELL_EXPORT_USER(SHELL_CMD_PERMISSION(1), root, root, root);


#define SHELL_CMD_USED(_name) \
        extern ShellCommand shellCommand##_name; \
        keep += (int) &shellCommand##_name

static void shellKeep(void)
{
    SHELL_CMD_USED(sysInfo);
    SHELL_CMD_USED(ps);
    SHELL_CMD_USED(hid_device);
    SHELL_CMD_USED(hid_send);
    // SHELL_CMD_USED(smartConfig);
    // SHELL_CMD_USED(wlanScan);
    // SHELL_CMD_USED(wlanConnect);
}

#if SHELL_USING_FUNC_SIGNATURE == 1
void shellFuncSignatureTest(int a, char *b, char c)
{
    printf("a = %d, b = %s, c = %c\r\n", a, b, c);
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC),
funcSignatureTest, shellFuncSignatureTest, test function signature, .data.cmd.signature = "isc");
#endif


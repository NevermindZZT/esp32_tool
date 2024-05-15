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
#include "rtam.h"


#define     SHELL_UART      UART_NUM_0

Shell shell;
char shellBuffer[512];

/**
 * @brief 用户shell写
 * 
 * @param data 数据
 * @param len 数据长度
 * 
 * @return signed short 写入实际长度
 */
signed short userShellWrite(char *data, unsigned short len)
{
    return uart_write_bytes(SHELL_UART, (const char *)data, len);
}


/**
 * @brief 用户shell读
 * 
 * @param data 数据
 * @param len 数据长度
 * 
 * @return signed short 读取实际长度
 */
signed short userShellRead(char *data, unsigned short len)
{
    return uart_read_bytes(SHELL_UART, (uint8_t *)data, len, portMAX_DELAY);
}


/**
 * @brief 用户shell初始化
 * 
 */
int userShellInit(void)
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

    return 0;
}
RTAPP_EXPORT(shell, userShellInit, NULL, NULL, RTAPP_FLAGS_AUTO_START|RATPP_FLAGS_SERVICE, NULL, NULL);

static void test(void)
{
    shellPrint(shellGetCurrent(), "hello world\n");
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(1)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC),
test, test, test);

SHELL_EXPORT_USER(SHELL_CMD_PERMISSION(1), root, root, root);

#if SHELL_USING_FUNC_SIGNATURE == 1
void shellFuncSignatureTest(int a, char *b, char c)
{
    printf("a = %d, b = %s, c = %c\r\n", a, b, c);
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC),
funcSignatureTest, shellFuncSignatureTest, test function signature, .data.cmd.signature = "isc");
#endif


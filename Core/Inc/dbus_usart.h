#ifndef DBUS_DBUS_USART_H
#define DBUS_DBUS_USART_H

#include "main.h"
#include <stdint.h>

#define DBUS_HUART         huart3
#define DBUS_BUFLEN        18U       // 标准DBUS帧长
#define DBUS_BUF_SIZE      90U       // 循环缓冲区总大小（可存 5 帧，约 70ms）
#define rc_Init {0, 0, 0, 0, 0, 0}


typedef struct
{
    int16_t ch0;    // 右摇杆水平
    int16_t ch1;    // 右摇杆垂直
    int16_t ch2;    // 左摇杆水平
    int16_t ch3;    // 左摇杆垂直
    uint8_t sw1;    // 右拨杆
    uint8_t sw2;    // 左拨杆
} rc_info_t;


extern rc_info_t rc;
extern volatile uint8_t dbus_rx_flag;  // 一帧接收完成标志

void dbus_uart_init(void);
void uart_receive_handler(UART_HandleTypeDef *huart);

#endif //DBUS_DBUS_USART_H

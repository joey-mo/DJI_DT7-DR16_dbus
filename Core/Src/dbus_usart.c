#include "dbus_usart.h"

#include <stdlib.h>
#include <string.h>

#include "usart.h"

// 循环接收缓冲区，DMA全程循环写入
uint8_t dbus_rx_buf[DBUS_BUF_SIZE] = {0};
rc_info_t rc = rc_Init;
volatile uint8_t dbus_rx_flag = 0U;


void dbus_uart_init(void)
{
    // 清除空闲标志，避免上电误触发
    __HAL_UART_CLEAR_IDLEFLAG(&DBUS_HUART);
    // 开启空闲中断
    __HAL_UART_ENABLE_IT(&DBUS_HUART, UART_IT_IDLE);
    HAL_NVIC_SetPriority(USART3_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(USART3_IRQn);
    // 用HAL标准函数启动DMA循环接收，DMA全程不停
    if (HAL_UART_Receive_DMA(&DBUS_HUART, dbus_rx_buf, DBUS_BUF_SIZE) != HAL_OK)
    {
        Error_Handler();
    }

    // 关掉 PEIE/EIE：HAL_UART_Receive_DMA() 会把这两个错误中断一起打开，而
    // HAL_UART_IRQHandler() 一旦看到 PE/FE/NE/ORE 就按"阻塞性错误"处理
    // （UART_EndRxTransfer() + 中止 DMA），接收从此永久失效——本工程只在初始化时
    // 启动一次接收，没有重启逻辑。关掉后错误分支的进入条件不成立，DMA 持续有效；
    // 错误标志仍由空闲中断里 __HAL_UART_CLEAR_IDLEFLAG 读 SR+DR 时一并清掉。
    __HAL_UART_DISABLE_IT(&DBUS_HUART, UART_IT_PE);    // CR1.PEIE = 0
    __HAL_UART_DISABLE_IT(&DBUS_HUART, UART_IT_ERR);   // CR3.EIE  = 0w w
}

static void rc_callback_handler(rc_info_t *rc, uint8_t *buff)
{
    // 摇杆通道解析，11位有效位，中点1024
    rc->ch0 = (int16_t)(((uint16_t)buff[0] | ((uint16_t)buff[1] << 8)) & 0x07FFU) - 1024;
    rc->ch1 = (int16_t)(((uint16_t)(buff[1] >> 3) | ((uint16_t)buff[2] << 5)) & 0x07FFU) - 1024;
    rc->ch2 = (int16_t)(((uint16_t)(buff[2] >> 6) | ((uint16_t)buff[3] << 2) | ((uint16_t)buff[4] << 10)) & 0x07FFU) - 1024;
    rc->ch3 = (int16_t)(((uint16_t)(buff[4] >> 1) | ((uint16_t)buff[5] << 7)) & 0x07FFU) - 1024;

    rc->sw1 = (buff[5] >> 6) & 0x03U;  // bit6~bit7
    rc->sw2 = (buff[5] >> 4) & 0x03U;  // bit4~bit5

    // 异常校验：DR16满杆约±660，超出则判定为数据错乱
    if ((abs(rc->ch0) > 660) || (abs(rc->ch1) > 660) ||
        (abs(rc->ch2) > 660) || (abs(rc->ch3) > 660))
    {
        memset(rc, 0, sizeof(rc_info_t));//清零
    }
}

static void uart_rx_idle_callback(UART_HandleTypeDef* huart)
{
    __HAL_UART_CLEAR_IDLEFLAG(huart);//第一件事先清除串口空闲标志

    if (huart != &DBUS_HUART)
    {
        return;
    }

    // 计算当前写指针位置
    // NDTR = 剩余可写字节数，总大小 - 剩余 = 已写入位置
    uint16_t write_pos = DBUS_BUF_SIZE - __HAL_DMA_GET_COUNTER(huart->hdmarx);
    uint8_t frame_buf[DBUS_BUFLEN];

    //从循环缓冲区快照出完整一帧
    if (write_pos >= DBUS_BUFLEN)
    {
        // 正常情况：帧未跨缓冲区首尾，直接倒推18字节拷贝
        memcpy(frame_buf, &dbus_rx_buf[write_pos - DBUS_BUFLEN], DBUS_BUFLEN);
    }
    else
    {
        // 回卷情况：帧跨了缓冲区结尾和开头，分两段拼接
        uint8_t tail_len = write_pos;
        uint8_t head_len = DBUS_BUFLEN - tail_len;
        // 先拷缓冲区尾部的部分
        memcpy(frame_buf, &dbus_rx_buf[DBUS_BUF_SIZE - head_len], head_len);
        // 再拷缓冲区开头的部分
        memcpy(frame_buf + head_len, dbus_rx_buf, tail_len);
    }

    // 空闲中断触发 = 一帧结束，取缓冲区里最后 18 字节解析
    // （DBUS 帧没有帧头、没有校验字节，按协议直接拆位即可）
    rc_callback_handler(&rc, frame_buf);
    dbus_rx_flag = 1U;
}

void uart_receive_handler(UART_HandleTypeDef *huart)
{
    // 同时检查标志位和中断使能，确保是有效空闲中断
    if (__HAL_UART_GET_FLAG(huart, UART_FLAG_IDLE) &&
        __HAL_UART_GET_IT_SOURCE(huart, UART_IT_IDLE))
    {
        uart_rx_idle_callback(huart);
    }
}
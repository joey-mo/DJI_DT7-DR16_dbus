//
// RC 遥控数据串口输出（VOFA+ FireWater 协议，阻塞发送）—— 实现
//
// 一帧只输出 6 个通道值（逗号分隔，换行结束）：
//   ch0,ch1,ch2,ch3,sw1,sw2
//     ch0 右摇杆水平 / ch1 右摇杆垂直 / ch2 左摇杆水平 / ch3 左摇杆垂直
//       范围 ±660，中位 0
//     sw1 右拨杆 / sw2 左拨杆，取值 1 / 2 / 3
//
// 用法：main() 的 while(1) 里调 RC_Monitor_Task()，函数内部限频。
// 上位机：VOFA+ 选 FireWater 协议，115200-8-N-1。
//

#include "rc_monitor.h"

#include <stdio.h>   /* snprintf：只用到整数格式，nano.specs 下无需 _printf_float */

#include "dbus_usart.h"   /* rc */
#include "usart.h"        /* huart6 */

/* 上一帧打印时刻，用于限频 */
static uint32_t monitor_last_tick = 0U;

void RC_Monitor_Task(void)
{
    char     buf[48];
    int      len;
    uint32_t now = HAL_GetTick();

    /* 限频：调用方每圈都调用也不会超频 */
    if ((now - monitor_last_tick) < RC_MONITOR_PERIOD_MS)
    {
        return;
    }
    monitor_last_tick = now;

    /* VOFA+ FireWater：逗号分隔，换行结束 */
    len = snprintf(buf, sizeof(buf), "%d,%d,%d,%d,%d,%d\n",
                   (int)rc.ch0, (int)rc.ch1, (int)rc.ch2, (int)rc.ch3,
                   (int)rc.sw1, (int)rc.sw2);
    if (len <= 0)
    {
        return;
    }
    if (len >= (int)sizeof(buf))
    {
        len = (int)sizeof(buf) - 1;   /* snprintf 被截断 */
    }

    (void)HAL_UART_Transmit(&huart6, (uint8_t *)buf, (uint16_t)len, RC_MONITOR_TX_TIMEOUT_MS);
}

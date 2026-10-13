//
// RC 遥控数据串口输出（VOFA+ FireWater 协议，阻塞发送）
// 发送口：USART6 —— PG14 = TX（接 USB-TTL 的 RX 脚），PG9 不用接
//
// 输出格式（一行一帧，逗号分隔，换行结束）：
//   ch0,ch1,ch2,ch3,sw1,sw2
//     ch0 右摇杆水平 / ch1 右摇杆垂直 / ch2 左摇杆水平 / ch3 左摇杆垂直
//       范围 ±660，中位 0
//     sw1 右拨杆 / sw2 左拨杆，取值 1 / 2 / 3
//
// 上位机：VOFA+ 选 FireWater 协议，115200-8-N-1，就是 6 条曲线。
// 注意：本工程用 --specs=nano.specs 链接，newlib-nano 不带浮点打印，
//       所以这里全部用整数输出（通道值本来就是整数，无需 %f）。
//

#ifndef DBUS_RC_MONITOR_H
#define DBUS_RC_MONITOR_H

#include <stdint.h>

// 打印周期（ms）：20ms 一帧 = 50Hz
#define RC_MONITOR_PERIOD_MS        20U

// 单帧阻塞发送超时（ms）
#define RC_MONITOR_TX_TIMEOUT_MS    10U

// 放在 main() 的 while(1) 里每圈调用一次；函数内部按 RC_MONITOR_PERIOD_MS 限频，
// 所以即使空循环每圈都调用也只会按周期发帧。
void RC_Monitor_Task(void);

#endif //DBUS_RC_MONITOR_H

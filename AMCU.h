#pragma once


#include "BambuBus.h"
#ifdef __cplusplus
extern "C"
{
#endif
//送料机存料检测IO
#define IO_PIN_1 18
#define IO_PIN_2 19
#define IO_PIN_3 20
#define IO_PIN_4 21
//#define IO_PIN_KEY 24

// 舵机引脚定义
#define SERVO_PIN_1 13
#define SERVO_PIN_2 12
#define SERVO_PIN_3 11
#define SERVO_PIN_4 10

#define SERVO_COUNT 4

#define max_filament_num 16
#define WHEEL_D 11 // 送料轮直径
#define SERVO_DELAY 6000 // 舵机延迟时间
#define PULL_BACK_METER 200 // 回抽距离



// #define AMCU_uart uart1
// #define AMCU_uart_IRQ UART1_IRQ
// #define AMCU_pin_tx 4
// #define AMCU_pin_rx 5

#define AMCU_AS5600_SDA 2
#define AMCU_AS5600_SCL 3

void set_color(u_int8_t r, u_int8_t g, u_int8_t b, u_int8_t a);
void rgb_set_breath(int16_t time,uint16_t count);
void selectOnePos(char num);
void releaseAllPos();
void set_LED_state(char led, char state);
void debug_info(bool force);

extern void AMCU_init();
extern void AMCU_run();
extern void AMCU_motion();


float get_last_pullback_meters(int num);


#ifdef __cplusplus
}
#endif

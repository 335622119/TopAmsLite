#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include <HardwareSerial.h>

#ifdef __cplusplus
extern "C" {
#endif

// 使用ESP32-C3的UART1
#define BambuBus_uart Serial1
#define UART_TX_PIN 5       // 根据您的接线修改
#define UART_RX_PIN 4       // 根据您的接线修改
#define BambuBus_pin_de 6   // MAX485的DE/RE控制引脚


int get_BambuBus_have_data();
uint8_t* get_BambuBus_data_buf();

enum _filament_status {
  offline,
  online,
  NFC_waiting
};

enum _filament_motion_state_set {
  need_pull_back,    // 需要回抽耗材
  need_send_out,     // 需要送出耗材
  waiting,           // 等待状态
  act_send_mm,       // 正在按毫米单位送出耗材
  act_pull_mm,       // 正在按毫米单位回抽耗材
  select_pos,        // 选择位置状态
  release_all,       // 释放所有部件
  cancel             // 取消/停止状态
};

enum package_type {
  BambuBus_package_filament_motion_short,    // 短包耗材运动信息
  BambuBus_package_filament_motion_long,     // 长包耗材运动信息
  BambuBus_package_online_detect,            // 在线检测包
  BambuBus_package_REQx6,                    // REQx6请求包
  BambuBus_package_NFC_detect,               // NFC检测包
  BambuBus_package_set_filament,             // 设置耗材信息包
  BambuBus_long_package_MC_online,           // MCU在线状态长包
  BambuBus_longe_package_filament,           // 耗材信息读取长包
  BambuBus_long_package_version,             // 版本信息长包
  BambuBus_package_heartbeat,                // 心跳包
  BambuBus_package_ETC,                      // 其他未分类包
  BambuBus_package_ERROR,                    // 错误包
  __BambuBus_package_packge_type_size        // 包类型总数（用于边界检查）
};


extern package_type get_packge_type(unsigned char *buf, int length);

extern void BambuBus_init();
extern int BambuBus_run();

extern bool Bambubus_read();
extern void Bambubus_set_need_to_save();
extern int get_now_filament_num();
extern String get_now_filament_name(int num);
extern void reset_filament_meters(int num);
extern void add_filament_meters(int num, float meters);
extern float get_filament_meters(int num);
extern void set_filament_online(int num, bool if_online);
extern bool is_filament_online(int num);
extern uint8_t get_filament_color_A(int num);
extern uint8_t get_filament_color_R(int num);
extern uint8_t get_filament_color_G(int num);
extern uint8_t get_filament_color_B(int num);
extern int get_cmd_type();
extern unsigned char get_now_op_num();
extern void set_filament_motion(int num, _filament_motion_state_set motion);
extern void send_uart(const unsigned char *data, uint16_t length);

_filament_motion_state_set get_filament_motion(int num);

#ifdef __cplusplus
}
#endif

#include "AMCU.h"
#include "Motor.h"
#include <limits> // 用于获取最小值



// 电机对象
Motor mc(7, 8); // 构建电机对象
Motor mc_ext(IO_PIN_3, IO_PIN_4); // 构建扩展电机对象(用于外接主电机驱动)

// 全局变量
char last_num = -1; // 上一次的 current_num
unsigned long last_num_change_time = 0; // 记录上次 current_num 变化的时间
bool execute_motion = true; // 控制是否执行动作
int last_filament_num = 255;
float last_meters[max_filament_num];
float last_pullback_meters[max_filament_num];
_filament_motion_state_set act;
bool ams_enable = true;
int now_debug_num = 0;
float target = 0;
float last_action[max_filament_num];

float get_last_pullback_meters(int num) {
  return last_pullback_meters[num];
}

// 初始化IO引脚
void init_io_pins() {
  pinMode(IO_PIN_1, OUTPUT);      // 设置引脚为输出模式
  digitalWrite(IO_PIN_1, HIGH);   // 输出高电平（3.3V）
}
// 初始化AMCU
void AMCU_init() {
  init_io_pins();
  //  init_selector();
  // 初始化BambuBus
  BambuBus_init();
  mc.stop(); // 停止电机
  mc_ext.stop(); // 停止电机，外部
  releaseAllPos(); // 释放所有料槽
  delay(500);
  selectOnePos(get_now_filament_num()); // 选中当前料槽

  for (int i = 0; i < max_filament_num; i++) {
    last_meters[i] = std::numeric_limits<float>::lowest();
  }
}
/**
   控制一路执行机
*/
void setSelectPos(char num, boolean state) {
  // 控制舵机选择料槽
#define pin 11
  //    switch(num){
  //        case 0:
  //            pin=SERVO_PIN_1;
  //            break;
  //        case 1:
  //            pin=SERVO_PIN_2;
  //            break;
  //        case 2:
  //            pin=SERVO_PIN_3;
  //            break;
  //        case 3:
  //            pin=SERVO_PIN_4;
  //            break;
  //        default:
  //            return;
  //    }
  //  if (state) {
  //    digitalWrite(pin, HIGH);
  //  } else {
  //    digitalWrite(pin, LOW);
  //  }
  //  Serial.println("控制一路执行机 :");
  //  Serial.println(num);
}

/**
   选择唯一一路执行机
*/
void selectOnePos(char num) {
  for (int i = 0; i < SERVO_COUNT; i++) {
    if (i == num) {
      setSelectPos(i, true);
    } else {
      setSelectPos(i, false);
    }
  }
  //  Serial.println("选择唯一一路执行机 :");
  //  Serial.println(num);
}

/**
   释放所有执行机
*/
void releaseAllPos() {
  for (int i = 0; i < SERVO_COUNT; i++) {
    setSelectPos(i, false);
  }
}
// AMCU主运行逻辑
void AMCU_run() {
  bool if_count_meters = true;
  static int now_filament_num = 255;
  int x = get_now_filament_num();

  // 检查当前料槽号是否变化
  if (now_filament_num != x) {
    now_filament_num = x;
    if_count_meters = false;
    Bambubus_set_need_to_save();
  }

  // 调试模式下使用手动指定的料槽号
  if (!ams_enable) {
    now_filament_num = now_debug_num;
  }
  // 根据当前动作状态处理里程计数
  _filament_motion_state_set current_state = get_filament_motion(now_filament_num);
  String current_state_text = "";
  switch (current_state) {
    case need_pull_back:

      Serial.println("111111111111 ");
      if (last_action[now_filament_num] != need_pull_back) {
        last_action[now_filament_num] = need_pull_back;
        last_pullback_meters[now_filament_num] = get_filament_meters(now_filament_num);
        Serial.println("22222222 ");
      }
      current_state_text = "反转-回抽操作";
      break;
    case need_send_out:
      last_action[now_filament_num] = need_send_out;
      if_count_meters = false;
      current_state_text = "正转-进料操作";
      break;
    case waiting:
      last_action[now_filament_num] = waiting;
      current_state_text = "等待指令";
      break;
    case act_send_mm:
      current_state_text = "正在送入耗材(mm)";
      break;
    case act_pull_mm:
      current_state_text = "正在回抽耗材(mm)";
      break;
    case select_pos:
      current_state_text = "选择位置中";
      break;
    case release_all:
      current_state_text = "释放所有部件";
      break;
    case cancel:
      current_state_text = "已取消/停止";
      break;
    default:
      current_state_text = "未知状态";
      break;
  }
  if (current_state_text != "等待指令") {
    Serial.println(current_state_text);
    Serial.println(now_filament_num);
  }
  //  switch (current_state) {
  //    case need_pull_back:// 回抽指令
  //      Serial.println("回抽指令");
  //      if (last_action[now_filament_num] != need_pull_back) {
  //        last_action[now_filament_num] = need_pull_back;
  //        last_pullback_meters[now_filament_num] = get_filament_meters(now_filament_num);
  //      }
  //      break;
  //    case need_send_out: // 送料指令,送出时不计入里程
  //      Serial.println("送料指令");
  //      last_action[now_filament_num] = need_send_out;
  //      if_count_meters = false;
  //      break;
  //    case waiting:
  //      last_action[now_filament_num] = waiting;
  //      break;
  //    default:
  //      break;
  //  }
  if (if_count_meters || ams_enable == false) {
    float distance_E = 0.01;
    add_filament_meters(now_filament_num, distance_E);
  }
  // 运行BambuBus协议
  int stu = -1;
  if (ams_enable) {
    stu = BambuBus_run();
  }
  // 执行运动控制
  AMCU_motion();

  // 检查IO引脚状态
  if (digitalRead(IO_PIN_1) == LOW && now_filament_num == 0) {
    set_filament_online(0, offline);
  } else {
    set_filament_online(0, online);
  }
  if (digitalRead(IO_PIN_1) == LOW && now_filament_num == 1) {
    set_filament_online(1, offline);
  } else {
    set_filament_online(1, online);
  }
  if (digitalRead(IO_PIN_1) == LOW && now_filament_num == 2) {
    set_filament_online(2, offline);
  } else {
    set_filament_online(2, online);
  }
  if (digitalRead(IO_PIN_1) == LOW && now_filament_num == 3) {
    set_filament_online(3, offline);
  } else {
    set_filament_online(3, online);
  }

  //  Serial.println("IO_PIN_1");
  //  Serial.println(digitalRead(IO_PIN_1));
}

// 运动控制逻辑
void AMCU_motion() {
  static int pull_end_cnt = 0; // 回抽结束后循环次数
  char current_num = get_now_filament_num();

  // 调试模式使用手动指定的料槽号
  if (!ams_enable) {
    current_num = now_debug_num;
  }

  // AMS模式下自动选择料槽
  if (ams_enable) {
    selectOnePos(current_num);
  }

  // 检查料槽号是否变化
  if (current_num != last_num) {
    last_num = current_num;
    last_num_change_time = millis(); // 更新时间戳
    execute_motion = false; // 重置动作执行标志位
  }

  // 检查等待时间是否结束
  if (!execute_motion && millis() - last_num_change_time >= SERVO_DELAY) {
    execute_motion = true; // 允许执行动作
  }

  float meter = get_filament_meters(current_num);

  // AMS模式下获取当前动作状态
  if (ams_enable) {
    act = get_filament_motion(current_num);
  }

  // 根据动作状态执行相应操作
  switch (act) {
    case need_pull_back: // 回抽动作
      //      Serial.println("回抽动作 开始");
      if (meter - last_pullback_meters[current_num] > PULL_BACK_METER) {
        //        Serial.println("回抽完成，停止电机");
        // 回抽完成，停止电机
        mc.stop();
        mc_ext.stop();

        if (pull_end_cnt > 0) {
          pull_end_cnt--;
        }
        if (pull_end_cnt == 0) {
          pull_end_cnt = -1;
        }
      } else if (execute_motion && is_filament_online(current_num)) {
        //        Serial.println("执行回抽动作");
        // 执行回抽动作
        pull_end_cnt = 20;
        mc.backforward(); // 退料控制DC电机反转
        mc_ext.backforward(); // 退料控制DC电机反转
      } else {
        // 条件不满足，停止电机
        mc.stop();
        mc_ext.stop();
      }
      break;

    case need_send_out: // 送料动作
      //      Serial.println("送料动作 开始");
      if (execute_motion && is_filament_online(current_num)) {
        //        Serial.println("执行送料动作");
        // 执行送料动作
        mc.forward(); // 送料控制DC电机正转
        mc_ext.forward(); // 送料控制DC电机正转
      } else {
        // 条件不满足，停止电机
        mc.stop();
        mc_ext.stop();
      }
      break;

    case waiting: // 等待状态
      //      Serial.println("电机等待状态");
      mc.stop();
      mc_ext.stop();
      break;

    case act_send_mm: // 手动送料指定距离
      //      Serial.println("手动送料指定距离 开始");
      if (last_pullback_meters[current_num] - meter > target) {
        //        Serial.println("手动送料指定距离，达到目标距离，停止电机");
        // 达到目标距离，停止电机
        mc.stop();
        mc_ext.stop();
        act = waiting;
      } else if (execute_motion && is_filament_online(current_num)) {
        //        Serial.println("执行送料动作，执行回抽动作");
        // 执行送料动作
        mc.forward();
        mc_ext.forward();
      } else {
        //        Serial.println("手动送料指定距离，条件不满足，停止电机");
        // 条件不满足，停止电机
        mc.stop();
        mc_ext.stop();
      }
      break;

    case select_pos: // 选择料槽位置
      Serial.println("选择料槽位置 开始 :");
      Serial.println(target);
      selectOnePos(target);
      break;

    case act_pull_mm: // 手动回抽指定距离
      Serial.println("手动回抽指定距离 开始");
      if (meter - last_pullback_meters[current_num] > target) {
        Serial.println("手动回抽指定距离，达到目标距离，停止电机");
        // 达到目标距离，停止电机
        mc.stop();
        mc_ext.stop();
        act = waiting;
      } else if (execute_motion && is_filament_online(current_num)) {
        Serial.println("手动回抽指定距离，执行回抽动作");
        // 执行回抽动作
        mc.backforward();
        mc_ext.backforward();
      } else {
        Serial.println("手动回抽指定距离，条件不满足，停止电机");
        // 条件不满足，停止电机
        mc.stop();
        mc_ext.stop();
      }
      break;

    case cancel: // 取消动作
      mc.stop();
      mc_ext.stop();
      act = waiting;
      break;

    case release_all: // 释放所有料槽
      mc.stop();
      mc_ext.stop();
      releaseAllPos();
      act = waiting;
      break;

    default:
      break;
  }
}

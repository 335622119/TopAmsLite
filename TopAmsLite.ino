#include "BambuBus.h"
#include "AMCU.h"


void setup() {
  Serial.begin(115200);

  Serial.println("BambuBus Init");
  AMCU_init();
  Serial.println("AMCU Init");

  // 设置默认线材在线状态
  //  for (int i = 0; i < 4; i++) {
  //    set_filament_online(i, true);
  //  }
}
bool isLog = true;
void loop() {

  AMCU_run();

  //   每10秒打印一次状态
  //  static unsigned long last_print = 0;
  //  if (millis() - last_print > 500) {
  //    if (isLog) {
  //      printDebugInfo(true);
  //    }
  //    last_print = millis();
  //  }

  // 处理线材动作请求
//  for (int i = 0; i < 4; i++) {
//    _filament_motion_state_set motion = get_filament_motion(i);
//    if (motion != waiting) {
//      Serial.printf("Processing motion for filament %d: %d\n", i, motion);
//      // 这里实现实际的线材动作控制
//      printDebugInfo(true);
//      // 完成后重置为等待状态
//      set_filament_motion(i, waiting);
//    }
//  }

  if (Serial.available() > 0) {  // 检查是否有串口数据
    char receivedChar = Serial.read();  // 读取一个字符
    if (receivedChar == '1') {          // 如果收到 '1'
      printDebugInfo(true);
      isLog = true;
    }
    else if (receivedChar == '0') {     // 如果收到 '0'
      isLog = false;
    }
  }
  delay(10);
}


void printDebugInfo(bool execute_motion) {
  Serial.println("");
  Serial.println("");
  Serial.println("\n===== 耗材槽状态监控 =====");

  int current_num = get_now_filament_num();

  // 遍历所有耗材槽（假设4个槽）
  for (int i = 0; i < 4; i++) {
    // 获取当前槽数据
    float meter = get_filament_meters(i);
    bool is_online = is_filament_online(i);

    // 根据槽号选择对应的引脚（槽0→PIN_1，槽1→PIN_2...）
    int io_pin = 0;
    int servo_pin = 0;
    switch (i) {
      case 0:
        io_pin = IO_PIN_1;
        servo_pin = SERVO_PIN_1;
        break;
      case 1:
        io_pin = IO_PIN_2;
        servo_pin = SERVO_PIN_2;
        break;
      case 2:
        io_pin = IO_PIN_3;
        servo_pin = SERVO_PIN_3;
        break;
      case 3:
        io_pin = IO_PIN_4;
        servo_pin = SERVO_PIN_4;
        break;
    }
    //获取指定耗材槽编号的运动状态
    String current_state_text = "";
    _filament_motion_state_set current_state = get_filament_motion(i);
    switch (current_state) {
      case need_pull_back:
        current_state_text = "反转-回抽操作";
        break;
      case need_send_out:
        current_state_text = "正转-进料操作";
        break;
      case waiting:
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

    // 读取颜色信息
    uint8_t r = get_filament_color_R(i);
    uint8_t g = get_filament_color_G(i);
    uint8_t b = get_filament_color_B(i);
    uint8_t a = get_filament_color_A(i);

    // 读取类型信息
    String type = get_now_filament_name(i);
    // 构建状态行
    char status_line[200];
    snprintf(status_line, sizeof(status_line),
             "[槽%d] %s%s  耗材:%-6s  RGBA:(%d,%d,%d,%d)\n"
             "  状态:%s  %s  长度:%-5.3f米  回抽:%-5.3f米\n"
             "  IO:%d  伺服:%d",
             i + 1,
             is_online ? "在线" : "离线",
             i == current_num ? " (当前)" : "",
             type.c_str(),
             r, g, b, a,
             current_state_text.c_str(),
             execute_motion && (i == current_num) ? "执行中" : "待机",
             meter,
             get_last_pullback_meters(i),
             digitalRead(io_pin),  // 动态读取对应IO引脚
             digitalRead(servo_pin) // 动态读取对应伺服引脚
            );

    Serial.println(status_line);
    Serial.println("----------------------------------");
  }
  // 全局信息
  Serial.printf("[全局] 操作号:%3d\n",
                get_now_op_num());
  Serial.println("======================");

  String cmd_text = "";
  bool is_log_show = true;
  // 完整帧接收成功
  int BambuBus_have_data_length = get_BambuBus_have_data();

  if (BambuBus_have_data_length > 0) {
    // 获取数据缓冲区指针（安全校验）
    uint8_t* source_buf = get_BambuBus_data_buf();
    uint8_t BambuBus_data_buf[1000];
    // 执行内存拷贝（带长度保护）
    size_t copy_length = min(BambuBus_have_data_length, 1000);
    memcpy(BambuBus_data_buf, source_buf, copy_length);
    int data_length = BambuBus_have_data_length;
    int cmd = get_packge_type(BambuBus_data_buf, data_length);
    switch (cmd) {
      case BambuBus_package_filament_motion_short:
        cmd_text = "短包耗材运动信息";
        break;
      case BambuBus_package_filament_motion_long:
        cmd_text = "长包耗材运动信息";
        break;
      case BambuBus_package_online_detect:
        cmd_text = "在线检测包";
        break;
      case BambuBus_package_REQx6:
        cmd_text = "REQx6请求包";
        break;
      case BambuBus_package_NFC_detect:
        cmd_text = "NFC检测包";
        break;
      case BambuBus_package_set_filament:
        cmd_text = "设置耗材信息包";
        break;
      case BambuBus_long_package_MC_online:
        cmd_text = "MCU在线状态长包";
        break;
      case BambuBus_longe_package_filament:
        cmd_text = "耗材信息读取长包";
        break;
      case BambuBus_long_package_version:
        cmd_text = "版本信息长包";
        break;
      case BambuBus_package_heartbeat:
        cmd_text = "收到心跳包，连接正常";  // 符合要求
        break;
      case BambuBus_package_ETC:
        cmd_text = "其他未分类包";
        break;
      case BambuBus_package_ERROR:
        cmd_text = "错误包";
        break;
      default:
        cmd_text = "未知数据包";  // 处理未定义的包类型
        break;
    }
    if (is_log_show) {
      Serial.printf("[协议] 命令: %s\n", cmd_text.c_str());
      Serial.println("---------------- HEX DUMP ----------------");
      for (int i = 0; i < copy_length; i++) {
        Serial.printf("%02X ", BambuBus_data_buf[i]);
        if ((i + 1) % 16 == 0) Serial.println();
      }
      Serial.println("\n------------------------------------------");
    }
  }
}

/**
 * @file Train_HAS3_altar.ino
 * @author YuBin Kim
 * @brief
 * @version 0.1
 * @date 2022-11-24 ~ 2022-11-26
 *
 * @copyright Copyright (c) 2022
 */

#define FIRMWARE_VER 14
#define PARTITION_VER 1
#include "Train_HAS3_altar.h"

//************************************************ Core1 ********************************************************************
/**
 * @brief Temple Intialize
 */
void TempleInit()
{
  // has2wifi.Setup("KT_GiGA_6C64", "ed46zx1198");                     j// 와이파이 세팅
  has2wifi.Setup("badland_shoot", "Code3824@");
  TelnetInit();
  ota.setLogStream(Serial);
  ota.setOnSuccess([]() {
    has2wifi.Send((String)(const char *)my["device_name"], "device_state", "setting");
  });
  ota.setOnSkip([]() {
    has2wifi.Send((String)(const char *)my["device_name"], "device_state", "setting");
  });
    ota.setPartitionUpdate(
        "https://raw.githubusercontent.com/Fuzzyline-HAS2/Train_HAS3_altar/main/partitions.bin",
        "https://raw.githubusercontent.com/Fuzzyline-HAS2/Train_HAS3_altar/main/partitions.sig",
        "https://raw.githubusercontent.com/Fuzzyline-HAS2/Train_HAS3_altar/main/partition_version.txt",
        PARTITION_VER
    );
  nexInit();                                                         // 디스플레이 세팅
  MySerial2.begin(9600, SERIAL_8N1, SERIAL2_RX_PIN, SERIAL2_TX_PIN); // 디스플레이 세팅
  SensorInit();                                                      // IoT Glove 사용 센서, 모듈 세팅
  TimerInit();                                                       // 타이머 세팅
  
}

/**
 * @brief 아두이노 기본 문법 (전원이 켜지면 한번만 실행)
 */
void setup()
{
  delay(1000);
  Serial.begin(115200);
  TempleInit();
  DataChange();
  NextionInit();
}

/**
 * @brief 아두이노 기본 문법 (전원이 켜져있는동안 Core1에서 계속 실행)
 */
void loop()
{
  TelnetRun();
  TimerRun();
  NeoFunc();
  if (activate_bool)
  {
    ActivateFunc();
  }
}

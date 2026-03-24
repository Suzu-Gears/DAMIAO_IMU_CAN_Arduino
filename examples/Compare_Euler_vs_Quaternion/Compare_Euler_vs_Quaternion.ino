#if defined(ARDUINO_ARCH_RENESAS)
#include <Arduino_CAN.h>  // For Arduino UNO R4, etc.

#elif defined(ARDUINO_ARCH_ESP32)
#include <ESP32_TWAI.h>  // For ESP32 series
const gpio_num_t CAN_TX_PIN = 22;
const gpio_num_t CAN_RX_PIN = 21;

#elif defined(ARDUINO_ARCH_RP2040)
#include <RP2040PIO_CAN.h>  // For RP2040, RP2350, etc.
const uint32_t CAN_TX_PIN = 0;
const uint32_t CAN_RX_PIN = 1;

#else
#warning "This board is not officially supported. Please include your CAN library and define CAN pins if necessary before including DAMIAO_IMU.h"
#include <Arduino_CAN.h>  // Default to Arduino_CAN
#endif

#include <DAMIAO_IMU_CAN.h>

const uint8_t IMU_MASTER_ID = 0x02;
const uint8_t IMU_SLAVE_ID = 0x01;

DamiaoImuCan imu(IMU_MASTER_ID, IMU_SLAVE_ID, &CAN);

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000);  // シリアルが開くまで最大3秒待機

  bool can_ok = false;
#if defined(ARDUINO_ARCH_ESP32)
  can_ok = CAN.begin(CanBitRate::BR_1000k, CAN_TX_PIN, CAN_RX_PIN);
#elif defined(ARDUINO_ARCH_RP2040)
  CAN.setTX(CAN_TX_PIN);
  CAN.setRX(CAN_RX_PIN);
  can_ok = CAN.begin(CanBitRate::BR_1000k);
#else  // This covers ARDUINO_ARCH_RENESAS and the default case
  can_ok = CAN.begin(CanBitRate::BR_1000k);
#endif

  if (!can_ok) {
    Serial.println("CAN bus initialization failed!");
    while (1);
  }
  Serial.println("CAN bus initialized.");

  Serial.println("--- 03_ActiveMode Example ---");
  Serial.println("Setting IMU to Active Mode and continuously updating data.");

  // アクティブモードに設定
  if (imu.changeToActive()) {
    Serial.println("IMU successfully changed to Active Mode.");
  } else {
    Serial.println("Failed to change IMU to Active Mode!");
  }
}

void loop() {
  imu.update();

  // ---------------------------------------------------------
  // 1. 既存のオイラー角 (IMU内部計算)
  // ---------------------------------------------------------
  float euler_roll = imu.getRoll();  // 単位: 度 (deg)

  // ---------------------------------------------------------
  // 2. クォータニオンから算出した「重力方向の傾き」
  // ---------------------------------------------------------
  float qw = imu.getQuatW();
  float qx = imu.getQuatX();
  float qy = imu.getQuatY();
  float qz = imu.getQuatZ();

  // 重力ベクトル（世界座標のZ軸）をロボット座標系（センサ座標系）へ変換
  // ※センサの搭載向きによって軸の定義が変わりますが、
  //   一般的に「X軸が前方」「Z軸が天頂」の場合の計算式です。

  // 重力のX成分（前後方向の成分）
  float gravity_x = 2.0f * (qx * qz - qw * qy);

  // 重力のY成分（左右方向の成分・今回は使いませんが参考まで）
  float gravity_y = 2.0f * (qy * qz + qw * qx);

  // 重力のZ成分（垂直方向の成分）
  float gravity_z = qw * qw - qx * qx - qy * qy + qz * qz;

  // 角度算出 (ラジアン -> 度)
  // atan2(y, z) で「垂直軸からの倒れ具合」を計算します
  float quat_roll_rad = atan2(gravity_y, gravity_z);
  float quat_roll_deg = quat_roll_rad * 180.0f / PI;

  // ---------------------------------------------------------
  // 3. 比較用のシリアル出力 (Arduino Serial Plotter対応形式)
  // ---------------------------------------------------------

  // 青線: IMUそのままのRoll
  Serial.print("Euler_Roll:");
  Serial.print(euler_roll);

  // 赤線: クォータニオン計算のRoll
  Serial.print(" ,Quat_Calc_Roll:");
  Serial.print(quat_roll_deg);

  // 参考: ヨー角（これを回したときに違いが出ます）
  Serial.print(" ,Yaw_Ref:");
  Serial.println(imu.getYaw());

  // 制御周期に合わせてウェイト（実際はもっと短くてOK）
  delay(50);
}

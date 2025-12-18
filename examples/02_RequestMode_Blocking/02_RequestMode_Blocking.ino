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

  Serial.println("--- 02_RequestMode_Blocking Example ---");
  Serial.println("Using readXXXX() functions to block and wait for sensor data.");

  // リクエストモードに設定
  if (imu.changeToRequest()) {
    Serial.println("IMU successfully changed to Request Mode.");
  } else {
    Serial.println("Failed to change IMU to Request Mode!");
  }
}

void loop() {
  // Read Accelerometer data (blocking call)
  if (imu.readAccel()) {
    Serial.print("Accel: ");
    Serial.print(imu.getAccelX(), 2);
    Serial.print(", ");
    Serial.print(imu.getAccelY(), 2);
    Serial.print(", ");
    Serial.print(imu.getAccelZ(), 2);
    Serial.print(" | Temp: ");  // 温度はAccelのデータに含まれる
    Serial.print(imu.getTemp());
    Serial.print("℃");
    Serial.println();
  } else {
    Serial.println("Failed to read accelerometer data.");
  }
  delay(500);

  // Read Gyro data (blocking call)
  if (imu.readGyro()) {
    Serial.print("Gyro: ");
    Serial.print(imu.getGyroX(), 2);
    Serial.print(", ");
    Serial.print(imu.getGyroY(), 2);
    Serial.print(", ");
    Serial.print(imu.getGyroZ(), 2);
    Serial.println();
  } else {
    Serial.println("Failed to read gyro data.");
  }
  delay(500);

  // Read Euler data (blocking call)
  if (imu.readEuler()) {
    Serial.print("Euler: ");
    Serial.print(imu.getPitch(), 2);
    Serial.print(", ");
    Serial.print(imu.getYaw(), 2);
    Serial.print(", ");
    Serial.print(imu.getRoll(), 2);
    Serial.println();
  } else {
    Serial.println("Failed to read Euler data.");
  }
  delay(500);

  // Read Quaternion data (blocking call)
  if (imu.readQuat()) {
    Serial.print("Quat: ");
    Serial.print(imu.getQuatW(), 2);
    Serial.print(", ");
    Serial.print(imu.getQuatX(), 2);
    Serial.print(", ");
    Serial.print(imu.getQuatY(), 2);
    Serial.print(", ");
    Serial.print(imu.getQuatZ(), 2);
    Serial.println();
  } else {
    Serial.println("Failed to read Quaternion data.");
  }
  delay(500);
}

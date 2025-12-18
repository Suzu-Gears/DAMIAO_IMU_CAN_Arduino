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
#include "imu_string_converter.h"

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

  Serial.println("--- 04_ReadWrite_Parameters Example ---");
  delay(1000);

  Serial.println("\n--- Reading Current Parameters ---");

  // CAN Slave ID
  auto slaveid_opt = imu.getCANSlaveId();
  if (slaveid_opt.has_value()) {
    Serial.print("Current CAN Slave ID: 0x");
    Serial.println(String(slaveid_opt.value(), HEX));
  } else {
    Serial.println("Failed to read CAN Slave ID.");
  }

  // CAN Master ID
  auto masterid_opt = imu.getCANMasterId();
  if (masterid_opt.has_value()) {
    Serial.print("Current CAN Master ID: 0x");
    Serial.println(String(masterid_opt.value(), HEX));
  } else {
    Serial.println("Failed to read CAN Master ID.");
  }

  // Baudrate
  auto baudrate_opt = imu.getBaudrate();
  if (baudrate_opt.has_value()) {
    Serial.print("Current CAN Baudrate: ");
    Serial.println(canBaudrateToString(baudrate_opt.value()));
  } else {
    Serial.println("Failed to read CAN Baudrate.");
  }
  delay(500);

  // --- Write new parameters ---
  Serial.println("\n--- Writing New Parameters ---");

  // Change CAN Slave ID to 0x05 for example, then change it back
  uint8_t new_id = 0x05;
  Serial.print("Changing Slave ID to 0x");
  Serial.println(String(new_id, HEX));
  if (imu.setCANSlaveId(new_id)) {
    Serial.println("Successfully set new Slave ID.");
    delay(100);
    slaveid_opt = imu.getCANSlaveId();
    if (slaveid_opt.has_value() && slaveid_opt.value() == new_id) {
      Serial.println("Verification successful.");
    } else {
      Serial.println("Verification FAILED.");
    }
  } else {
    Serial.println("Failed to set new Slave ID.");
  }
  delay(500);

  // Change back to original ID
  Serial.print("Changing Slave ID back to 0x");
  Serial.println(String(IMU_SLAVE_ID, HEX));
  if (imu.setCANSlaveId(IMU_SLAVE_ID)) {
    Serial.println("Successfully reverted Slave ID.");
  } else {
    Serial.println("Failed to revert Slave ID.");
  }
  delay(500);

  // --- Save parameters to flash ---
  Serial.println("\n--- Saving Parameters to Flash ---");
  Serial.println("NOTE: This will permanently change the IMU's startup configuration.");
  // Uncomment the following lines to save the current settings to the IMU's flash memory.
  /*
  if (imu.saveParameters()) {
    Serial.println("Parameters successfully saved to flash.");
  } else {
    Serial.println("Failed to save parameters.");
  }
  */
  delay(1000);

  Serial.println("\n--- Finished ---");
}

void loop() {
}

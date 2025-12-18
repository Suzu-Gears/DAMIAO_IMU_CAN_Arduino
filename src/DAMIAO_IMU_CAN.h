#pragma once

#include <Arduino.h>
#include <cstdint>
#include <cstring>
#include <optional>

#if !__has_include(<api/HardwareCAN.h>)
#error "DAMIAO_IMU_CAN.h: include a HardwareCAN provider (e.g. <Arduino_CAN.h>, <ESP32_TWAI.h>, <RP2040PIO_CAN.h>) before this file."
#endif

#include <api/HardwareCAN.h>

class DamiaoImuCan {
public:
  enum class CmdMode : uint8_t {
    READ = 0,
    WRITE = 1
  };

  enum class ComPort : uint8_t {
    COM_USB = 0,
    COM_RS485,
    COM_CAN,
    COM_VOFA
  };

  enum class CAN_Baudrate : uint8_t {
    CAN_BAUD_1M = 0,
    CAN_BAUD_500K,
    CAN_BAUD_400K,
    CAN_BAUD_250K,
    CAN_BAUD_200K,
    CAN_BAUD_100K,
    CAN_BAUD_50K,
    CAN_BAUD_25K
  };

  enum class AskId : uint8_t {
    SUCCESS = 0,         // Success response
    REGISTER_NOT_EXIST,  // Register does not exist
    INVALID_DATA,        // Invalid data
    OPERATION_FAILED,    // Operation failed
    TIMEOUT = 255        // Library defined timeout
  };

  enum class RegId : uint8_t {
    REBOOT_IMU = 0,         // 00 W
    ACCEL_DATA,             // 01 R
    GYRO_DATA,              // 02 R
    EULER_DATA,             // 03 R
    QUAT_DATA,              // 04 R
    SET_ZERO,               // 05 W
    ACCEL_CALI,             // 06 W
    GYRO_CALI,              // 07 W
    MAG_CALI,               // 08 W
    CHANGE_COM,             // 09 RW
    SET_DELAY,              // 0A RW
    CHANGE_ACTIVE,          // 0B RW
    SET_BAUD,               // 0C RW
    SET_CAN_ID,             // 0D RW
    SET_MST_ID,             // 0E RW
    DATA_OUTPUT_SELECTION,  // 0F RW Output Data Select: Modification not supported by current FW 1.0.2.4
    SAVE_PARAM = 254,       // FE W
    RESTORE_SETTING = 255   // FF W
  };

  DamiaoImuCan(uint32_t master_id, uint32_t slave_id, arduino::HardwareCAN* can = nullptr)
    : masterId_(master_id), slaveId_(slave_id), can_(can) {}

  void setCANBus(arduino::HardwareCAN* can) {
    can_ = can;
  }

  bool update() {
    bool result = false;
    while (can_->available() > 0) {
      const CanMsg rxMsg = can_->read();
      if (rxMsg.getStandardId() != masterId_) { continue; }
      if (rxMsg.data_length < 8) { continue; }
      switch (static_cast<RegId>(rxMsg.data[0])) {
        case RegId::ACCEL_DATA:
          updateAccel(rxMsg.data);
          result = true;
          break;
        case RegId::GYRO_DATA:
          updateGyro(rxMsg.data);
          result = true;
          break;
        case RegId::EULER_DATA:
          updateEuler(rxMsg.data);
          result = true;
          break;
        case RegId::QUAT_DATA:
          updateQuaternion(rxMsg.data);
          result = true;
          break;
      }
    }
    return result;
  }

  float getAccelX() const {
    return data_.accel[0];
  }
  float getAccelY() const {
    return data_.accel[1];
  }
  float getAccelZ() const {
    return data_.accel[2];
  }
  float getGyroX() const {
    return data_.gyro[0];
  }
  float getGyroY() const {
    return data_.gyro[1];
  }
  float getGyroZ() const {
    return data_.gyro[2];
  }
  float getQuatW() const {
    return data_.quat[0];
  }
  float getQuatX() const {
    return data_.quat[1];
  }
  float getQuatY() const {
    return data_.quat[2];
  }
  float getQuatZ() const {
    return data_.quat[3];
  }
  float getRoll() const {
    return data_.roll;
  }
  float getPitch() const {
    return data_.pitch;
  }
  float getYaw() const {
    return data_.yaw;
  }
  uint8_t getTemp() const {
    return data_.temp;
  }

  bool requestAccel() {
    return sendCmd(RegId::ACCEL_DATA, CmdMode::READ, 0);
  }
  bool requestGyro() {
    return sendCmd(RegId::GYRO_DATA, CmdMode::READ, 0);
  }
  bool requestEuler() {
    return sendCmd(RegId::EULER_DATA, CmdMode::READ, 0);
  }
  bool requestQuat() {
    return sendCmd(RegId::QUAT_DATA, CmdMode::READ, 0);
  }
  bool readAccel(std::optional<uint32_t> timeout_ms_opt = std::nullopt) {
    if (!requestAccel()) return false;
    return receiveFeedback(RegId::ACCEL_DATA, timeout_ms_opt);
  }
  bool readGyro(std::optional<uint32_t> timeout_ms_opt = std::nullopt) {
    if (!requestGyro()) return false;
    return receiveFeedback(RegId::GYRO_DATA, timeout_ms_opt);
  }
  bool readEuler(std::optional<uint32_t> timeout_ms_opt = std::nullopt) {
    if (!requestEuler()) return false;
    return receiveFeedback(RegId::EULER_DATA, timeout_ms_opt);
  }
  bool readQuat(std::optional<uint32_t> timeout_ms_opt = std::nullopt) {
    if (!requestQuat()) return false;
    return receiveFeedback(RegId::QUAT_DATA, timeout_ms_opt);
  }

  bool reboot() {  // 00 W
    return writeReg(RegId::REBOOT_IMU, 0);
  }
  bool setZero() {  // 05 W
    return writeReg(RegId::SET_ZERO, 0);
  }
  bool accelCalibration() {  // 06 W
    return writeReg(RegId::ACCEL_CALI, 0);
  }
  bool gyroCalibration() {  // 07 W
    return writeReg(RegId::GYRO_CALI, 0);
  }
  bool magCalibration() {  // 08 W
    return writeReg(RegId::MAG_CALI, 0);
  }
  std::optional<ComPort> getComPort() {  // 09 RW
    auto response = readReg(RegId::CHANGE_COM);
    if (response.has_value()) {
      return static_cast<ComPort>(response.value());
    }
    return std::nullopt;
  }
  bool setComPort(ComPort port) {  // 09 RW
    return writeReg(RegId::CHANGE_COM, static_cast<uint8_t>(port));
  }
  std::optional<uint32_t> getActiveModeDelay() {  // 0A RW
    return readReg(RegId::SET_DELAY);
  }
  // Not working. 1 ~ 1000 [ms]
  bool setActiveModeDelay(uint32_t delay) {  // 0A RW
    return writeReg(RegId::SET_DELAY, delay);
  }
  std::optional<bool> isActiveMode() {  // 0B RW
    auto response = readReg(RegId::CHANGE_ACTIVE);
    if (response.has_value()) {
      return response.value() == 1;
    }
    return std::nullopt;
  }
  bool changeToActive() {  // 0B RW
    return writeReg(RegId::CHANGE_ACTIVE, 1);
  }
  bool changeToRequest() {  // 0B RW
    return writeReg(RegId::CHANGE_ACTIVE, 0);
  }
  std::optional<CAN_Baudrate> getBaudrate() {  // 0C RW
    auto response = readReg(RegId::SET_BAUD);
    if (response.has_value()) {
      return static_cast<CAN_Baudrate>(response.value());
    }
    return std::nullopt;
  }
  bool setBaudrate(CAN_Baudrate baud) {  // 0C RW
    return writeReg(RegId::SET_BAUD, static_cast<uint8_t>(baud));
  }
  std::optional<uint32_t> getCANSlaveId() {  // 0D RW
    return readReg(RegId::SET_CAN_ID);
  }
  bool setCANSlaveId(uint32_t slaveId) {  // 0D RW
    if (writeReg(RegId::SET_CAN_ID, slaveId)) {
      slaveId_ = slaveId;
      return true;
    }
    return false;
  }
  std::optional<uint32_t> getCANMasterId() {  // 0E RW
    return readReg(RegId::SET_MST_ID);
  }
  bool setCANMasterId(uint32_t masterId) {  // 0E RW
    if (writeReg(RegId::SET_MST_ID, masterId)) {
      masterId_ = masterId;
      return true;
    }
    return false;
  }
  std::optional<uint32_t> getDataOutputSelection() {  // 0F RW
    return readReg(RegId::DATA_OUTPUT_SELECTION);
  }

  // Modification not supported by current FW 1.0.2.4.
  // bool setDataOutputSelection(uint32_t selection) {  // 0F RW
  //   return writeReg(RegId::DATA_OUTPUT_SELECTION, selection);
  // }

  // Save succeeds, but always returns false.
  bool saveParameters() {  // FE W
    return writeReg(RegId::SAVE_PARAM, 0);
  }

  // Not working.
  // bool restoreSettings() {  // FF W
  //   return writeReg(RegId::RESTORE_SETTING, 0);
  // }

private:
  struct ImuData {
    uint8_t temp = 0;
    float accel[3] = { 0, 0, 0 };  // X, Y, Z
    float gyro[3] = { 0, 0, 0 };   // X, Y, Z
    float pitch = 0;
    float yaw = 0;
    float roll = 0;
    float quat[4] = { 0, 0, 0, 0 };  // W, X, Y, Z
  };

  static constexpr uint32_t DEFAULT_PARAM_TIMEOUT_MS = 30;

  static constexpr float ACCEL_CAN_MAX = 235.2f;
  static constexpr float ACCEL_CAN_MIN = -235.2f;
  static constexpr float GYRO_CAN_MAX = 34.88f;
  static constexpr float GYRO_CAN_MIN = -34.88f;
  static constexpr float PITCH_CAN_MAX = 90.0f;
  static constexpr float PITCH_CAN_MIN = -90.0f;
  static constexpr float ROLL_CAN_MAX = 180.0f;
  static constexpr float ROLL_CAN_MIN = -180.0f;
  static constexpr float YAW_CAN_MAX = 180.0f;
  static constexpr float YAW_CAN_MIN = -180.0f;
  static constexpr float QUATERNION_MAX = 1.0f;
  static constexpr float QUATERNION_MIN = -1.0f;

  bool sendCmd(RegId reg_id, CmdMode rw, uint32_t data) {
    CanMsg txMsg;
    txMsg.id = CanStandardId(slaveId_);
    txMsg.data_length = 8;
    txMsg.data[0] = 0xCC;
    txMsg.data[1] = static_cast<uint8_t>(reg_id);
    txMsg.data[2] = static_cast<uint8_t>(rw);  // 0 for Read, 1 for Write
    txMsg.data[3] = 0xDD;
    std::memcpy(txMsg.data + 4, &data, 4);
    return can_->write(txMsg) >= 0;
  }

  std::optional<uint32_t> readReg(RegId reg_id, std::optional<uint32_t> timeout_ms_opt = std::nullopt) {
    if (!sendCmd(reg_id, CmdMode::READ, 0)) { return std::nullopt; }
    uint32_t rxData = 0;
    if (receiveRegResponse(reg_id, rxData, timeout_ms_opt) != AskId::SUCCESS) { return std::nullopt; }
    return rxData;
  }

  bool writeReg(RegId reg_id, uint32_t data, std::optional<uint32_t> timeout_ms_opt = std::nullopt) {
    if (!sendCmd(reg_id, CmdMode::WRITE, data)) { return false; }
    uint32_t rxData;
    return receiveRegResponse(reg_id, rxData, timeout_ms_opt) == AskId::SUCCESS;
  }

  AskId receiveRegResponse(RegId expected_reg_id, uint32_t& rxData, std::optional<uint32_t> timeout_ms_opt = std::nullopt) {
    uint32_t timeout_ms = timeout_ms_opt.value_or(DEFAULT_PARAM_TIMEOUT_MS);
    uint32_t startTime = millis();
    while (millis() - startTime < timeout_ms) {
      if (can_->available()) {
        const CanMsg rxMsg = can_->read();
        if (rxMsg.getStandardId() != CanStandardId(masterId_)) { continue; }
        if (rxMsg.data_length < 8) { continue; }
        if (rxMsg.data[0] != 0xCC) { continue; }
        if (rxMsg.data[1] != static_cast<uint8_t>(expected_reg_id)) { continue; }
        if (rxMsg.data[2] != 0xDD) { continue; }
        const AskId ask_id = static_cast<AskId>(rxMsg.data[3]);
        if (ask_id == AskId::SUCCESS) {
          std::memcpy(&rxData, rxMsg.data + 4, 4);
        }
        return ask_id;
      }
    }
    return AskId::TIMEOUT;
  }

  bool receiveFeedback(RegId expected_reg_id, std::optional<uint32_t> timeout_ms_opt = std::nullopt) {
    uint32_t timeout_ms = timeout_ms_opt.value_or(DEFAULT_PARAM_TIMEOUT_MS);
    uint32_t startTime = millis();
    while (millis() - startTime < timeout_ms) {
      if (can_->available()) {
        const CanMsg rxMsg = can_->read();
        if (rxMsg.getStandardId() != CanStandardId(masterId_)) { continue; }
        if (rxMsg.data_length < 8) { continue; }
        const RegId sensorType = static_cast<RegId>(rxMsg.data[0]);
        if (sensorType != expected_reg_id) { continue; }
        switch (sensorType) {
          case RegId::ACCEL_DATA:
            updateAccel(rxMsg.data);
            return true;
          case RegId::GYRO_DATA:
            updateGyro(rxMsg.data);
            return true;
          case RegId::EULER_DATA:
            updateEuler(rxMsg.data);
            return true;
          case RegId::QUAT_DATA:
            updateQuaternion(rxMsg.data);
            return true;
          default:
            break;
        }
      }
    }
    return false;
  }

  //  int float_to_uint(float x_float, float x_min, float x_max, int bits) {
  //    float span = x_max - x_min;
  //    float offset = x_min;
  //    return (int)((x_float - offset) * ((float)((1 << bits) - 1)) / span);
  //  }

  float uint_to_float(int x_int, float x_min, float x_max, int bits) {
    float span = x_max - x_min;
    float offset = x_min;
    return ((float)x_int) * span / ((float)((1 << bits) - 1)) + offset;
  }

  void updateAccel(const uint8_t* pData) {
    auto update_Temperature = [this](const uint8_t* pData) {
      data_.temp = pData[1];
    };

    update_Temperature(pData);
    uint16_t accel_raw[3];
    accel_raw[0] = pData[3] << 8 | pData[2];
    accel_raw[1] = pData[5] << 8 | pData[4];
    accel_raw[2] = pData[7] << 8 | pData[6];
    data_.accel[0] = uint_to_float(accel_raw[0], ACCEL_CAN_MIN, ACCEL_CAN_MAX, 16);
    data_.accel[1] = uint_to_float(accel_raw[1], ACCEL_CAN_MIN, ACCEL_CAN_MAX, 16);
    data_.accel[2] = uint_to_float(accel_raw[2], ACCEL_CAN_MIN, ACCEL_CAN_MAX, 16);
  }

  void updateGyro(const uint8_t* pData) {
    uint16_t gyro_raw[3];
    gyro_raw[0] = pData[3] << 8 | pData[2];
    gyro_raw[1] = pData[5] << 8 | pData[4];
    gyro_raw[2] = pData[7] << 8 | pData[6];
    data_.gyro[0] = uint_to_float(gyro_raw[0], GYRO_CAN_MIN, GYRO_CAN_MAX, 16);
    data_.gyro[1] = uint_to_float(gyro_raw[1], GYRO_CAN_MIN, GYRO_CAN_MAX, 16);
    data_.gyro[2] = uint_to_float(gyro_raw[2], GYRO_CAN_MIN, GYRO_CAN_MAX, 16);
  }

  void updateEuler(const uint8_t* pData) {
    int16_t euler_raw[3];
    euler_raw[0] = pData[3] << 8 | pData[2];
    euler_raw[1] = pData[5] << 8 | pData[4];
    euler_raw[2] = pData[7] << 8 | pData[6];
    data_.pitch = uint_to_float(euler_raw[0], PITCH_CAN_MIN, PITCH_CAN_MAX, 16);
    data_.yaw = uint_to_float(euler_raw[1], YAW_CAN_MIN, YAW_CAN_MAX, 16);
    data_.roll = uint_to_float(euler_raw[2], ROLL_CAN_MIN, ROLL_CAN_MAX, 16);
  }

  void updateQuaternion(const uint8_t* pData) {
    int w = pData[1] << 6 | ((pData[2] & 0xF8) >> 2);
    int x = (pData[2] & 0x03) << 12 | (pData[3] << 4) | ((pData[4] & 0xF0) >> 4);
    int y = (pData[4] & 0x0F) << 10 | (pData[5] << 2) | (pData[6] & 0xC0) >> 6;
    int z = (pData[6] & 0x3F) << 8 | pData[7];
    data_.quat[0] = uint_to_float(w, QUATERNION_MIN, QUATERNION_MAX, 14);
    data_.quat[1] = uint_to_float(x, QUATERNION_MIN, QUATERNION_MAX, 14);
    data_.quat[2] = uint_to_float(y, QUATERNION_MIN, QUATERNION_MAX, 14);
    data_.quat[3] = uint_to_float(z, QUATERNION_MIN, QUATERNION_MAX, 14);
  }

  arduino::HardwareCAN* can_;
  uint32_t masterId_;
  uint32_t slaveId_;
  ImuData data_;
};

#pragma once

#include <DAMIAO_IMU_CAN.h>

// Helper function to convert ComPort enum to a string
const char* comPortToString(DamiaoImuCan::ComPort port) {
  switch (port) {
    case DamiaoImuCan::ComPort::COM_USB:
      return "USB";
    case DamiaoImuCan::ComPort::COM_RS485:
      return "RS485";
    case DamiaoImuCan::ComPort::COM_CAN:
      return "CAN";
    case DamiaoImuCan::ComPort::COM_VOFA:
      return "VOFA";
    default:
      return "Unknown";
  }
}

// Helper function to convert CAN_Baudrate enum to a string
const char* canBaudrateToString(DamiaoImuCan::CAN_Baudrate baud) {
  switch (baud) {
    case DamiaoImuCan::CAN_Baudrate::CAN_BAUD_1M:
      return "1Mbps";
    case DamiaoImuCan::CAN_Baudrate::CAN_BAUD_500K:
      return "500kbps";
    case DamiaoImuCan::CAN_Baudrate::CAN_BAUD_400K:
      return "400kbps";
    case DamiaoImuCan::CAN_Baudrate::CAN_BAUD_250K:
      return "250kbps";
    case DamiaoImuCan::CAN_Baudrate::CAN_BAUD_200K:
      return "200kbps";
    case DamiaoImuCan::CAN_Baudrate::CAN_BAUD_100K:
      return "100kbps";
    case DamiaoImuCan::CAN_Baudrate::CAN_BAUD_50K:
      return "50kbps";
    case DamiaoImuCan::CAN_Baudrate::CAN_BAUD_25K:
      return "25kbps";
    default:
      return "Unknown";
  }
}

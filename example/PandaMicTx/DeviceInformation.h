#ifndef _DEVICE_INFORMATION_H
#define _DEVICE_INFORMATION_H

#include <string>
#include <esp_gap_bt_api.h>

#define STR_FORMAT "%02x:%02x:%02x:%02x:%02x:%02x"

class BluetoothAddress
{
public:
  esp_bd_addr_t value;

  BluetoothAddress() {}

  BluetoothAddress(const esp_bd_addr_t &a)
  {
    memcpy(&value, &a, ESP_BD_ADDR_LEN);
  }

  BluetoothAddress(const string &s)
  {
    sscanf(s.c_str(), STR_FORMAT, value[0], value[1], value[2], value[3], value[4], value[5]);
  }

  string toString() const
  {
    char str[18];
    sprintf(str, STR_FORMAT, value[0], value[1], value[2], value[3], value[4], value[5]);
    return std::string(str);
  }
};

struct DeviceInformation
{
  BluetoothAddress address;
  std::string name;
  int rssi;
};

#endif

#ifndef _DEVICE_H
#define _DEVICE_H

#include <string>
#include "DeviceInformation.h"

struct Device
{
  BluetoothAddress address;
  char name[50];
};

bool operator==(const Device &lhs, const Device &rhs)
{
  return lhs.address.toString() == rhs.address.toString();
}

bool operator!=(const Device &lhs, const Device &rhs)
{
  return !(lhs == rhs);
}

#endif

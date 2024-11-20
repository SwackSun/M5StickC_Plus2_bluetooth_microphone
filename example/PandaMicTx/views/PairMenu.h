#ifndef _PAIR_MENU_H
#define _PAIR_MENU_H

#include "Navigation/Menu.h"
#include "storage/Storage.h"
#include "PairCommand.h"

BluetoothAddress testDevice(esp_bd_addr_t{11, 38, 117, 3, 197, 152});
DeviceInformation device = {
  .address=testDevice,
  .name="123",
  .rssi=1,
};

class PairMenu : public Menu
{

public:
  PairMenu(CLite_GFX *gfx) : Menu(gfx, "Scan for new device", "Pairing")
  {
    this->custom([&](CLite_GFX *gfx) {
        return new PairCommand(device.name + " (" + device.address.toString() + ")", device);
      });
  }

  void onEnter() override
  {

  }

  void onLeave() override
  {
    clear();
  }
};

#endif

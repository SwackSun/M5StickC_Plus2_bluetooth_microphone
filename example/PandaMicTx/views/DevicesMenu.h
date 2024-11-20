#ifndef _DEVICES_MENU_H
#define _DEVICES_MENU_H

#include "Navigation/Menu.h"
#include "storage/Storage.h"

class DevicesMenu : public Menu
{
public:
  DevicesMenu(CLite_GFX *gfx) : Menu(gfx, "Configure devices", "Devices")
  {
  }

  void onEnter() override
  {
    auto devices = storage.getDevices();
    for (size_t i(0); i < devices.size(); ++i)
    {
      Device device(devices[i]);
      auto menu = subMenu(device.name, device.name);

      menu->command(
          "Set as active", [=]() { storage.setActiveDevice(device); }, CallbackAction::Back);

      if (device != storage.getActiveDevice())
      {
        menu->command(
            "Forget", [=]() { storage.removeDevice(i); }, CallbackAction::Back);
      }
    }
  }

  void onLeave() override
  {
    clear();
  }
};

#endif

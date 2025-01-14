#ifndef _HOME_VIEW_H
#define _HOME_VIEW_H

#include <A2DPSession.h>
#include "Navigation/Drawable.h"
#include "GlobalTicker.h"
#include "utils.h"
#include "enum_strings.h"
#include "storage/Storage.h"

#define CHAR_HEIGHT 5
#define CHAR_WIDTH 5

#define SIDEBAR_WIDTH CHAR_WIDTH + 5

class HomeView : public Drawable
{
  bool screenActive = true;
  bool hasChanged = true;
  Drawable *mainMenu;
  Drawable *visualizer;
  A2DPSession *aSession;

  GFXcanvas1 sidebar;
  GFXcanvas1 canvas;

  int batteryPercentage;
  GlobalTicker batteryTicker;
  GlobalTicker refreshTicker;
  GlobalTicker screenTicker;

public:
  HomeView(CLite_GFX *gfx, Drawable *mainMenu, Drawable *visualizer, A2DPSession *aSession)
      : Drawable(gfx),
        mainMenu(mainMenu),
        visualizer(visualizer),
        aSession(aSession),
        sidebar(gfx), canvas(gfx),
        batteryTicker(1000, [&]() { refreshBatteryPercentage(); }),
        refreshTicker(250, [&]() { hasChanged = true; }),
        screenTicker(
            30000, [&]() { screenOff(); }, 1)
  {
    sidebar.createSprite(SIDEBAR_WIDTH, gfx->height());
    sidebar.setFont(&TomThumb);
    sidebar.setTextSize(1);
    canvas.createSprite(gfx->width() - SIDEBAR_WIDTH, gfx->height());
    canvas.setFont(&TomThumb);
    canvas.setTextSize(1);
    refreshBatteryPercentage();
  }

  virtual bool needsRedraw()
  {
    return hasChanged;
  }

  virtual void draw()
  {
    if (!screenActive)
      return gfx->fillScreen(0);

    sidebar.fillScreen(0);
    canvas.fillScreen(0);

    drawCanvas();
    drawSidebar();

    gfx->startWrite();
    canvas.pushSprite(SIDEBAR_WIDTH, 0);
    sidebar.pushSprite(0, 0);
    gfx->endWrite();
    hasChanged = false;
  }

  virtual NavigationCommand *input(int key)
  {
    if (!screenActive && key != KEY_A)
    {
      screenOn();
      return new NopCommand();
    }

    screenTicker.start();
    switch (key)
    {
    case KEY_A:
      if (aSession->connectionState == A2DPSession::ConnectionState::CONNECTED)
      {
        if (aSession->mediaState == A2DPSession::MediaState::INACTIVE)
        {
          aSession->resume();
          digitalWrite(LED_BUILTIN, HIGH);
        }
        else
        {
          aSession->pause();
          digitalWrite(LED_BUILTIN, LOW);
        }
      }
      return new NopCommand();
    case KEY_B:
      return new NavigateToCommand(mainMenu);
    case KEY_C:
      return new NavigateToCommand(visualizer);
    }

    return new NopCommand();
  }

  void onEnter() override
  {
    batteryTicker.start();
    refreshTicker.start();
    screenTicker.start();
  }

  void onLeave() override
  {
    batteryTicker.stop();
    refreshTicker.stop();
    screenTicker.stop();
  }

private:
  void drawCanvas()
  {
    char bV[50];

    // Header
    canvas.setCursor(0, 0);
    canvas.print(":: PANDA MICROPHONE");
    canvas.setCursor(canvas.width() - CHAR_WIDTH * 5, 0);
    sprintf(bV, "%d%%", batteryPercentage);
    canvas.print(bV);
    canvas.drawFastHLine(0, CHAR_HEIGHT + 2, canvas.width(), TFT_WHITE);

    canvas.setCursor(0, CHAR_HEIGHT * 2 + 3);
    canvas.println(storage.getActiveDevice().name);

    canvas.print("BT: ");
    canvas.println(enumToString(aSession->connectionState).c_str());

    canvas.print("TX: ");
    canvas.println(enumToString(aSession->mediaState).c_str());
  }

  void drawSidebar()
  {
    if (aSession->connectionState == A2DPSession::ConnectionState::CONNECTED)
    {
      sidebar.setCursor(0, CHAR_HEIGHT);
      sidebar.print("M");
    }

    sidebar.setCursor(1, (sidebar.height() + CHAR_HEIGHT) / 2);
    sidebar.print("=");

    sidebar.setCursor(1, sidebar.height());
    sidebar.print("V");

    sidebar.drawFastVLine(CHAR_WIDTH + 2, 0, sidebar.height(), TFT_WHITE);
  }

  void refreshBatteryPercentage()
  {
    batteryPercentage = getBatteryPercentage() * 100;
    notifyChanged();
  }

  void notifyChanged()
  {
    if (screenActive)
      hasChanged = true;
  }

  void screenOn()
  {
    screenTicker.start();
    screenActive = true;
    hasChanged = true;
  }

  void screenOff()
  {
    screenActive = false;
    hasChanged = true;
  }
};

#endif

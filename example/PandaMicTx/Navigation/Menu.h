#ifndef _MENU_H
#define _MENU_H

#include <iostream>
#include "Navigation/TomThumb2.h"
#include "Menu.h"
#include "MenuItem.h"
#include "MenuCommand.h"
#include "MenuInfo.h"
#include "NavigationCommand.h"
#include "Drawable.h"

#define CHAR_HEIGHT 12
#define CHAR_WIDTH 8
#define LINE_WIDTH 1
#define SPACING 1

#define HEADER_WIDTH (CHAR_HEIGHT + LINE_WIDTH + SPACING)

using namespace std;

class Menu : public MenuItem, public Drawable
{
  GFXcanvas1 headerCanvas;
  GFXcanvas1 mainCanvas;

  int scrollPosition = 0;
  int selectedItem = 0;
  vector<MenuItem *> menuItems;

  bool hasChanged = true;

public:
  Menu(CLite_GFX *gfx, string label) : Menu(gfx, label, ""){};

  Menu(CLite_GFX *gfx, string label, string name) : Drawable(gfx), headerCanvas(HEADER_WIDTH, gfx->height()), mainCanvas(gfx->width() - HEADER_WIDTH, gfx->height()), MenuItem(label, name)
  {
    headerCanvas.setFont(&TomThumb2);
    headerCanvas.setTextSize(2);
    mainCanvas.setFont(&TomThumb2);
    mainCanvas.setTextSize(2);
    info("Back");
  }

  bool needsRedraw() override
  {
    return hasChanged;
  }

  void draw() override
  {
    if (!name.empty())
    {
      drawHeader();
      drawCanvasToGfx(&headerCanvas, 0, 0);
    }

    drawMenu();
    drawCanvasToGfx(&mainCanvas, name.empty() ? 0 : headerCanvas.width() + CHAR_WIDTH, 0);

    hasChanged = false;
  }

  NavigationCommand *input(int key) override
  {
    notifyChanged();

    if (key == KEY_UP)
    {
      --selectedItem;
      if (selectedItem < 0)
        selectedItem = menuItems.size() - 1;
    }
    else if (key == KEY_DOWN)
    {
      selectedItem = (selectedItem + 1) % menuItems.size();
    }
    else if (key == KEY_SELECT)
    {
      if (selectedItem == 0)
      {
        return new BackCommand();
      }

      return menuItems[selectedItem]->activate();
    }

    if (!isItemVisible(selectedItem))
      scrollToItem(selectedItem);

    return new NopCommand();
  }

  Menu *subMenu(string name, string label)
  {
    Menu *menu = new Menu(gfx, label, name);
    menuItems.push_back(menu);
    notifyChanged();
    return menu;
  }

  void command(string label, callback_t callback, CallbackAction action = CallbackAction::Nop)
  {
    MenuCommand *command = new MenuCommand(label, callback, action);
    menuItems.push_back(command);
    notifyChanged();
  }

  MenuInfo *info(string label)
  {
    MenuInfo *info = new MenuInfo(label);
    menuItems.push_back(info);
    notifyChanged();
    return info;
  }

  void custom(std::function<MenuItem *(CLite_GFX *)> factory)
  {
    MenuItem *menu = factory(gfx);
    menuItems.push_back(menu);
    notifyChanged();
  }

  NavigationCommand *activate()
  {
    return new NavigateToCommand(this);
  }

  void clear()
  {
    menuItems.clear();
    info("Back");
    int scrollPosition = 0;
    int selectedItem = 0;
  }

  virtual void onEnter() override {}
  virtual void onLeave() override {}

protected:
  void notifyChanged()
  {
    hasChanged = true;
  }

private:
  void drawHeader()
  {
    headerCanvas.fillScreen(0);
    headerCanvas.setRotation(3);
    headerCanvas.setCursor(0, CHAR_HEIGHT);
    headerCanvas.print(name.c_str());
    headerCanvas.drawLine(0, CHAR_HEIGHT + SPACING, headerCanvas.width(), CHAR_HEIGHT + SPACING, 1);
    headerCanvas.setRotation(0);
  }

  void drawMenu()
  {
    GFXcanvas1 canvas(mainCanvas.width(), getItemPosition(menuItems.size()));
    canvas.setFont(&TomThumb2);
    canvas.setTextSize(2);
    canvas.setCursor(0, CHAR_HEIGHT);

    for (int i(0); i < menuItems.size(); ++i)
    {
      MenuItem *item = menuItems[i];

      if (i == selectedItem)
      {
        canvas.print("> ");
      }
      else
      {
        canvas.setCursor(canvas.getCursorX() + CHAR_WIDTH * 2, canvas.getCursorY());
      }

      canvas.println(item->label.c_str());
    }

    mainCanvas.fillScreen(0);
    mainCanvas.setCursor(0, CHAR_HEIGHT);
    mainCanvas.drawBitmap(0, -scrollPosition, canvas.getBuffer(), canvas.width(), canvas.height(), TFT_WHITE, 0);
  }

  void exit()
  {
  }

  void scrollToItem(int item)
  {
    int itemPosition = getItemPosition(item);

    if (itemPosition + CHAR_HEIGHT > scrollPosition + mainCanvas.height())
    {
      scrollPosition = itemPosition - mainCanvas.height() + CHAR_HEIGHT + 2;
    }
    else
    {
      scrollPosition = itemPosition;
    }
  }

  bool isItemVisible(int item)
  {
    int itemPosition = getItemPosition(item + 1);
    // ESP_LOGD("", "itemPosition: %d, scrollPosition: %d, mainCanvasHeight: %d", itemPosition, scrollPosition, mainCanvas.height());
    return itemPosition - CHAR_HEIGHT > scrollPosition && itemPosition < scrollPosition + mainCanvas.height();
  }

  void drawCanvasToGfx(GFXcanvas1 *canvas, int x, int y)
  {
    gfx->drawBitmap(x, y, canvas->getBuffer(), canvas->width(), canvas->height(), TFT_WHITE, 0);
  }

  static int getItemPosition(int item)
  {
    return (CHAR_HEIGHT + 1) * item;
  }
};

#endif

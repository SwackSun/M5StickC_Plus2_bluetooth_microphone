#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>
#include <Arduino.h>
#include <EasyButton.h>
#include <Wire.h>
#include "CLite_GFX.h"
#include "Navigation/Navigation.h"
#include "Navigation/Menu.h"
#include <esp_pm.h>
#include "constants.h"
#include "logo.h"
#include "views/HomeView.h"
#include "views/PairMenu.h"
#include "views/DevicesMenu.h"
#include "views/VolumeVisualizer.h"
#include "storage/Storage.h"
#include "esp32-hsp-hf/hfp_hf_main.h"

#define CONFIG_EXAMPLE_PEER_DEVICE_NAME "PandaMicTx"

#define LOG_TAG "main"

extern int hfp_hf_main(void);

const float ADC_MODIFIER = 3.3 / 4095;
const float BATTERY_MODIFIER = 2 * (3.45 / 4095);

CLite_GFX lcd;
Navigation navigation;
// BluetoothClient bt;

EasyButton buttonA(BUTTON_A);
EasyButton buttonB(BUTTON_B);
EasyButton buttonC(BUTTON_C);

HomeView *homeView;
Menu *mainMenu;
MenuInfo *analogInputInfo;
MenuInfo *batteryInfo;
MenuInfo *cpuInfo;
VolumeVisalizer visualizer(&lcd);
BlueStatus bStatus = BlueStatus();

void redraw();
void start();
void stop();
void activateBluetooth();
void turnOff();

GlobalTicker powerTicker(5000, []() {
  if (getBatteryPercentage() > 0.15)
  {
    if (bStatus.connectionState == BlueStatus::ConnectionState::CONNECTED && bStatus.mediaState == BlueStatus::MediaState::ACTIVE)
    {
      digitalWrite(LED_BUILTIN, HIGH);
    }
    else
    {
      digitalWrite(LED_BUILTIN, HIGH);
      vTaskDelay(pdMS_TO_TICKS(20));
      digitalWrite(LED_BUILTIN, LOW);
    }
  }
  else if (getBatteryPercentage() < 0.15)
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
});

GlobalTicker refreshInfo(5000, []() {
  char bV[50];
  sprintf(bV, "Battery: %.2fv", analogRead(BATTERY_PIN) * BATTERY_MODIFIER);
  batteryInfo->label = string(bV);

  char cI[50];
  sprintf(cI, "CPU: %dMHz", getCpuFrequencyMhz());
  cpuInfo->label = string(cI);
});

void setup()
{
  /* Hold pwr pin */
  gpio_reset_pin((gpio_num_t)POWER_HOLD_PIN);
  pinMode(POWER_HOLD_PIN, OUTPUT);
  digitalWrite(POWER_HOLD_PIN, HIGH);
  Wire1.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  
  lcd.begin();
  lcd.setRotation(1);
  printf("Demo...\r\n");
  vTaskDelay(pdMS_TO_TICKS(2002));

  // NVS & Storage
//   esp_err_t ret = nvs_flash_init();
//   if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
//   {
//     printf("======>nvs_flash_erase...\n");
//     ESP_ERROR_CHECK(nvs_flash_erase());
//     ret = nvs_flash_init();
//   }
//   ESP_ERROR_CHECK(ret);
//   storage.init();

  // Display
  lcd.setBrightness(255);
  lcd.fillScreen(TFT_BLACK);
  lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  lcd.setTextSize(1);
  lcd.setTextColor(TFT_WHITE);
  lcd.clearDisplay();
  lcd.drawBitmap(0, 0, LOGO, 128, 32, TFT_WHITE);
  lcd.display();
  
  // Views
  mainMenu = new Menu(&lcd, "Main Menu", "Main");
  homeView = new HomeView(&lcd, mainMenu, &visualizer, &bStatus);
  navigation.navigateTo(homeView);

  // Menus
  mainMenu->custom([](CLite_GFX *gfx) { return new DevicesMenu(gfx); });
  mainMenu->custom([](CLite_GFX *gfx) { return new PairMenu(gfx); });
  mainMenu->command("Disconnect", stop, CallbackAction::Back);
  // mainMenu->command("Connect", stop);
  // mainMenu->command(
  //     "Toggle LED", []() { digitalWrite(BUILTIN_LED, !digitalRead(BUILTIN_LED)); });
  Menu *infoMenu = mainMenu->subMenu("Dev Inf", "Device information");
  batteryInfo = infoMenu->info("Battery: ?");
  cpuInfo = infoMenu->info("CPU: ?");
  infoMenu->command("Draw logo", []() {
    lcd.clearDisplay();
    lcd.drawBitmap(0, 0, LOGO, 128, 32, TFT_WHITE);
    lcd.display();
    vTaskDelay(pdMS_TO_TICKS(2000));
  });
  mainMenu->command("Turn off", []() { turnOff(); });

  // Buttons
  buttonA.begin();
  buttonB.begin();
  buttonC.begin();
  buttonA.onPressed([]() { navigation.input(KEY_A); });
  buttonB.onPressed([]() { navigation.input(KEY_B); });
  buttonC.onPressed([]() { navigation.input(KEY_C); });

  // Timers
  powerTicker.start();
  refreshInfo.start();

  // Initializations
  // activateBluetooth();
  // audio_init();

  // bStatus.start(storage.getActiveDevice().address);

  hfp_hf_main();

  //Init finished, clear lcd
  vTaskDelay(pdMS_TO_TICKS(2000));
  lcd.clearDisplay();
  lcd.display();
}

void redraw()
{
  // static portMUX_TYPE drawMutex = portMUX_INITIALIZER_UNLOCKED;
  lcd.clearDisplay();
  navigation.draw();
  lcd.display();
  // portENTER_CRITICAL(&drawMutex);
  // portEXIT_CRITICAL(&drawMutex);
}

void loop()
{
  GlobalTicker::updateAll();

  buttonA.read();
  buttonB.read();
  buttonC.read();

  if (navigation.needsRedraw())
    redraw();

  vTaskDelay(10);
  yield();
}

void activateBluetooth()
{

}

void start()
{
  if (bStatus.start())
  {
  }
}

void stop()
{
  bStatus.stop();
  vTaskDelay(pdMS_TO_TICKS(150));

}

void turnOff()
{
  stop();
  esp_deep_sleep_start();
}

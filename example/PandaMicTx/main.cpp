#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include <Arduino.h>
#include <EasyButton.h>
#include <Wire.h>
#include "CLite_GFX.h"
#include "Navigation/Navigation.h"
#include "Navigation/Menu.h"
#include <esp_pm.h>
#include "constants.h"
#include "audio_in.h"
#include "logo.h"
#include "views/HomeView.h"
#include "views/PairMenu.h"
#include "views/DevicesMenu.h"
#include "views/VolumeVisualizer.h"
#include "storage/Storage.h"
#include "esp_bt.h"
#include "hfp_hf/bt_app_core.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_bt_api.h"
#include "esp_hf_client_api.h"
#include "hfp_hf/bt_app_hf.h"
#include "hfp_hf/gpio_pcm_config.h"
#include "esp_console.h"
#include "hfp_hf/app_hf_msg_set.h"
//#include "sdkconfig.h"
// #undef CORE_DEBUG_LEVEL
// #define CORE_DEBUG_LEVEL 4
// #undef CONFIG_LOG_DEFAULT_LEVEL_DEBUG
// #define CONFIG_LOG_DEFAULT_LEVEL_DEBUG y
// #undef CONFIG_LOG_DEFAULT_LEVEL
// #define CONFIG_LOG_DEFAULT_LEVEL 4
// #undef CONFIG_LOG_MAXIMUM_EQUALS_DEFAULT
// #define CONFIG_LOG_MAXIMUM_EQUALS_DEFAULT y
// #undef CONFIG_LOG_MAXIMUM_LEVEL
// #define CONFIG_LOG_MAXIMUM_LEVEL 4
// #undef CONFIG_BT_ENABLED
// #define CONFIG_BT_ENABLED y
// #undef CONFIG_BT_BLE_ENABLED
// #define CONFIG_BT_BLE_ENABLED n
// #undef CONFIG_BTDM_CTRL_MODE_BR_EDR_ONLY
// #define CONFIG_BTDM_CTRL_MODE_BR_EDR_ONLY y
// #undef CONFIG_BTDM_CTRL_BR_EDR_MAX_SYNC_CONN
// #define CONFIG_BTDM_CTRL_BR_EDR_MAX_SYNC_CONN 1
// #undef CONFIG_BT_BLUEDROID_ENABLED
// #define CONFIG_BT_BLUEDROID_ENABLED y
// #undef CONFIG_BT_CLASSIC_ENABLED
// #define CONFIG_BT_CLASSIC_ENABLED y
// #undef CONFIG_BT_HFP_ENABLE
// #define CONFIG_BT_HFP_ENABLE y
// #undef CONFIG_BT_HFP_CLIENT_ENABLE
// #define CONFIG_BT_HFP_CLIENT_ENABLE y
// #undef CONFIG_BT_HFP_AUDIO_DATA_PATH_HCI
// #define CONFIG_BT_HFP_AUDIO_DATA_PATH_HCI y

#include "MicDriver.h"

#define CONFIG_EXAMPLE_PEER_DEVICE_NAME "PandaMicTx"

#define LOG_TAG "main"

const float ADC_MODIFIER = 3.3 / 4095;
const float BATTERY_MODIFIER = 2 * (3.45 / 4095);

MicDriver micDriver = MicDriver();
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

void redraw();
void start();
void stop();
void activateBluetooth();
void turnOff();
int32_t dataCallback(uint8_t *data, int32_t len);

A2DPSession aSession(dataCallback);

esp_bd_addr_t peer_addr = {0};
static char peer_bdname[ESP_BT_GAP_MAX_BDNAME_LEN + 1];
static uint8_t peer_bdname_len;
static const char remote_device_name[] = CONFIG_EXAMPLE_PEER_DEVICE_NAME;

static char *bda2str(esp_bd_addr_t bda, char *str, size_t size)
{
    if (bda == NULL || str == NULL || size < 18) {
        return NULL;
    }

    uint8_t *p = bda;
    sprintf(str, "%02x:%02x:%02x:%02x:%02x:%02x",
            p[0], p[1], p[2], p[3], p[4], p[5]);
    return str;
}

static bool get_name_from_eir(uint8_t *eir, char *bdname, uint8_t *bdname_len)
{
    uint8_t *rmt_bdname = NULL;
    uint8_t rmt_bdname_len = 0;

    if (!eir) {
        return false;
    }

    rmt_bdname = esp_bt_gap_resolve_eir_data(eir, ESP_BT_EIR_TYPE_CMPL_LOCAL_NAME, &rmt_bdname_len);
    if (!rmt_bdname) {
        rmt_bdname = esp_bt_gap_resolve_eir_data(eir, ESP_BT_EIR_TYPE_SHORT_LOCAL_NAME, &rmt_bdname_len);
    }

    if (rmt_bdname) {
        if (rmt_bdname_len > ESP_BT_GAP_MAX_BDNAME_LEN) {
            rmt_bdname_len = ESP_BT_GAP_MAX_BDNAME_LEN;
        }

        if (bdname) {
            memcpy(bdname, rmt_bdname, rmt_bdname_len);
            bdname[rmt_bdname_len] = '\0';
        }
        if (bdname_len) {
            *bdname_len = rmt_bdname_len;
        }
        return true;
    }

    return false;
}

void esp_bt_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
    switch (event) {
    case ESP_BT_GAP_DISC_RES_EVT: {
        for (int i = 0; i < param->disc_res.num_prop; i++){
            if (param->disc_res.prop[i].type == ESP_BT_GAP_DEV_PROP_EIR
                && get_name_from_eir((uint8_t*)(param->disc_res.prop[i].val), peer_bdname, &peer_bdname_len)){
                if (strcmp(peer_bdname, remote_device_name) == 0) {
                    memcpy(peer_addr, param->disc_res.bda, ESP_BD_ADDR_LEN);
                    ESP_LOGI(BT_HF_TAG, "Found a target device address:");
                    ESP_LOG_BUFFER_HEX(BT_HF_TAG, peer_addr, ESP_BD_ADDR_LEN);
                    ESP_LOGI(BT_HF_TAG, "Found a target device name: %s", peer_bdname);
                    printf("Connect.\n");
                    esp_hf_client_connect(peer_addr);
                    esp_bt_gap_cancel_discovery();
                }
            }
        }
        break;
    }
    case ESP_BT_GAP_DISC_STATE_CHANGED_EVT:
        ESP_LOGI(BT_HF_TAG, "ESP_BT_GAP_DISC_STATE_CHANGED_EVT");
    case ESP_BT_GAP_RMT_SRVCS_EVT:
    case ESP_BT_GAP_RMT_SRVC_REC_EVT:
        break;
    case ESP_BT_GAP_AUTH_CMPL_EVT: {
        if (param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS) {
            ESP_LOGI(BT_HF_TAG, "authentication success: %s", param->auth_cmpl.device_name);
            ESP_LOG_BUFFER_HEX(BT_HF_TAG, param->auth_cmpl.bda, ESP_BD_ADDR_LEN);
        } else {
            ESP_LOGE(BT_HF_TAG, "authentication failed, status:%d", param->auth_cmpl.stat);
        }
        break;
    }
    case ESP_BT_GAP_PIN_REQ_EVT: {
        ESP_LOGI(BT_HF_TAG, "ESP_BT_GAP_PIN_REQ_EVT min_16_digit:%d", param->pin_req.min_16_digit);
        if (param->pin_req.min_16_digit) {
            ESP_LOGI(BT_HF_TAG, "Input pin code: 0000 0000 0000 0000");
            esp_bt_pin_code_t pin_code = {0};
            esp_bt_gap_pin_reply(param->pin_req.bda, true, 16, pin_code);
        } else {
            ESP_LOGI(BT_HF_TAG, "Input pin code: 1234");
            esp_bt_pin_code_t pin_code;
            pin_code[0] = '1';
            pin_code[1] = '2';
            pin_code[2] = '3';
            pin_code[3] = '4';
            esp_bt_gap_pin_reply(param->pin_req.bda, true, 4, pin_code);
        }
        break;
    }

#if (CONFIG_EXAMPLE_SSP_ENABLED == true)
    case ESP_BT_GAP_CFM_REQ_EVT:
        ESP_LOGI(BT_HF_TAG, "ESP_BT_GAP_CFM_REQ_EVT Please compare the numeric value: %"PRIu32, param->cfm_req.num_val);
        esp_bt_gap_ssp_confirm_reply(param->cfm_req.bda, true);
        break;
    case ESP_BT_GAP_KEY_NOTIF_EVT:
        ESP_LOGI(BT_HF_TAG, "ESP_BT_GAP_KEY_NOTIF_EVT passkey:%"PRIu32, param->key_notif.passkey);
        break;
    case ESP_BT_GAP_KEY_REQ_EVT:
        ESP_LOGI(BT_HF_TAG, "ESP_BT_GAP_KEY_REQ_EVT Please enter passkey!");
        break;
#endif

    case ESP_BT_GAP_MODE_CHG_EVT:
        ESP_LOGI(BT_HF_TAG, "ESP_BT_GAP_MODE_CHG_EVT mode:%d", param->mode_chg.mode);
        break;

    default: {
        ESP_LOGI(BT_HF_TAG, "event: %d", event);
        break;
    }
    }
    return;
}

/* event for handler "bt_av_hdl_stack_up */
enum {
    BT_APP_EVT_STACK_UP = 0,
};

/* handler for bluetooth stack enabled events */
static void bt_hf_client_hdl_stack_evt(uint16_t event, void *p_param);

void ble_test(void)
{
    char bda_str[18] = {0};
    /* Initialize NVS — it is used to store PHY calibration data */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BLE));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    if ((ret = esp_bt_controller_init(&bt_cfg)) != ESP_OK) {
        ESP_LOGE(BT_HF_TAG, "%s initialize controller failed: %s", __func__, esp_err_to_name(ret));
        return;
    }

    if ((ret = esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT)) != ESP_OK) {
        ESP_LOGE(BT_HF_TAG, "%s enable controller failed: %s", __func__, esp_err_to_name(ret));
        return;
    }

    if ((ret = esp_bluedroid_init()) != ESP_OK) {
        ESP_LOGE(BT_HF_TAG, "%s initialize bluedroid failed: %s", __func__, esp_err_to_name(ret));
        return;
    }

    if ((ret = esp_bluedroid_enable()) != ESP_OK) {
        ESP_LOGE(BT_HF_TAG, "%s enable bluedroid failed: %s", __func__, esp_err_to_name(ret));
        return;
    }

    ESP_LOGI(BT_HF_TAG, "Own address:[%s]", bda2str((uint8_t *)esp_bt_dev_get_address(), bda_str, sizeof(bda_str)));
    /* create application task */
    bt_app_task_start_up();

    /* Bluetooth device name, connection mode and profile set up */
    bt_app_work_dispatch(bt_hf_client_hdl_stack_evt, BT_APP_EVT_STACK_UP, NULL, 0, NULL);

#if CONFIG_BT_HFP_AUDIO_DATA_PATH_PCM
    /* configure the PCM interface and PINs used */
    app_gpio_pcm_io_cfg();
#endif

    /* configure external chip for acoustic echo cancellation */
#if ACOUSTIC_ECHO_CANCELLATION_ENABLE
    app_gpio_aec_io_cfg();
#endif /* ACOUSTIC_ECHO_CANCELLATION_ENABLE */
    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    esp_console_dev_uart_config_t uart_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    repl_config.prompt = "hfp_hf>";

    // init console REPL environment
    ESP_ERROR_CHECK(esp_console_new_repl_uart(&uart_config, &repl_config, &repl));

    /* Register commands */
    register_hfp_hf();
    printf("\n ==================================================\n");
    printf(" |       Steps to test hfp_hf                     |\n");
    printf(" |                                                |\n");
    printf(" |  1. Print 'help' to gain overview of commands  |\n");
    printf(" |  2. Setup a service level connection           |\n");
    printf(" |  3. Run hfp_hf to test                         |\n");
    printf(" |                                                |\n");
    printf(" =================================================\n\n");

    // start console REPL
    ESP_ERROR_CHECK(esp_console_start_repl(repl));
}

static void bt_hf_client_hdl_stack_evt(uint16_t event, void *p_param)
{
    ESP_LOGD(BT_HF_TAG, "%s evt %d", __func__, event);
    switch (event) {
    case BT_APP_EVT_STACK_UP: {
        /* set up device name */
        // char *dev_name = "ESP_HFP_HF";
        // esp_bt_gap_set_device_name(dev_name);
        ESP_LOGI(BT_HF_TAG, "============== BT_APP_EVT_STACK_UP");
        /* register GAP callback function */
        esp_bt_gap_register_callback(esp_bt_gap_cb);
        ESP_LOGI(BT_HF_TAG, "============== esp_bt_gap_cb is set");
        esp_hf_client_register_callback(bt_app_hf_client_cb);
        ESP_LOGI(BT_HF_TAG, "============== esp_hf_client_register_callback is set");
        esp_hf_client_init();

#if (CONFIG_EXAMPLE_SSP_ENABLED == true)
    /* Set default parameters for Secure Simple Pairing */
    esp_bt_sp_param_t param_type = ESP_BT_SP_IOCAP_MODE;
    esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_IO;
    esp_bt_gap_set_security_param(param_type, &iocap, sizeof(uint8_t));
#endif

        esp_bt_pin_type_t pin_type = ESP_BT_PIN_TYPE_FIXED;
        esp_bt_pin_code_t pin_code;
        pin_code[0] = '0';
        pin_code[1] = '0';
        pin_code[2] = '0';
        pin_code[3] = '0';
        esp_bt_gap_set_pin(pin_type, 4, pin_code);

        /* set discoverable and connectable mode, wait to be connected */
        esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);

        /* start device discovery */
        ESP_LOGI(BT_HF_TAG, "Starting device discovery...");
        esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY, 10, 0);
        break;
    }
    default:
        ESP_LOGE(BT_HF_TAG, "%s unhandled evt %d", __func__, event);
        break;
    }
}

void bt_app_hf_client_audio_open(void)
{
    ESP_LOGI(BT_HF_TAG, "%d", __func__);
    micDriver.mic_init();
}

void bt_app_hf_client_audio_close(void)
{

}

uint32_t bt_app_hf_client_outgoing_cb(uint8_t *p_buf, uint32_t sz)
{
  micDriver.mic_get_raw(p_buf,sz);
}

void bt_app_hf_client_incoming_cb(const uint8_t *buf, uint32_t sz)
{

}

GlobalTicker powerTicker(5000, []() {
  if (getBatteryPercentage() > 0.15)
  {
    if (aSession.connectionState == A2DPSession::ConnectionState::CONNECTED && aSession.mediaState == A2DPSession::MediaState::ACTIVE)
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
  homeView = new HomeView(&lcd, mainMenu, &visualizer, &aSession);
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

  // aSession.start(storage.getActiveDevice().address);

  ble_test();

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
  ESP_LOGD(LOG_TAG, "Activating bluetooth...");
  if (!bluetooth.isEnabled())
  {
    bool status;
    ESP_LOGD(LOG_TAG, "bluetooth.enable...");
    status = bluetooth.enable();

    if (status)
    {
      ESP_LOGD(LOG_TAG, "Successfully turned on bluetooth.");
    }
    else
    {
      ESP_LOGW(LOG_TAG, "Failed to turn on bluetooth.");
      return;
    }
  }
}

void start()
{
  // if (aSession.start(testDevice))
  // {
  //   audio_start();
  //   // print = true;
  // }
}

void stop()
{
  aSession.stop();
  vTaskDelay(pdMS_TO_TICKS(150));
  // audio_stop();
  // print = false;
}

void turnOff()
{
  stop();
  audio_stop();
  audio_deinit();
  bluetooth.disable();
  esp_deep_sleep_start();
}

uint8_t buffer[2048];
int32_t dataCallback(uint8_t *data, int32_t len)
{
  if (len <= 0 || data == nullptr)
    return 0;

  size_t read;
  ESP_ERROR_CHECK(i2s_read(I2S_PORT, buffer, len * 2, &read, portMAX_DELAY));

  for (int i = 0; i < read / 8; ++i)
  {
    data[4 * i + 0] = buffer[8 * i + 6];
    data[4 * i + 1] = buffer[8 * i + 7];
    data[4 * i + 2] = buffer[8 * i + 6];
    data[4 * i + 3] = buffer[8 * i + 7];
  }

  if (visualizer.isActive)
  {
    memcpy(visualizer.buffer, data, read / 2);
    visualizer.samples = read / 2;
  }

  return read / 2;
}

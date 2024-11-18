#ifndef _BLUETOOTH_CLIENT_H
#define _BLUETOOTH_CLIENT_H

#include <esp_log.h>
#include <esp_bt.h>
#include <esp_bt_main.h>
#include <esp_bt_device.h>
#include <esp_gap_bt_api.h>
#include <nvs.h>
#include <nvs_flash.h>

#define BT_DEVICE_NAME "PandaMic"

class BluetoothClient
{
public:
  bool isEnabled()
  {
    return esp_bluedroid_get_status() == ESP_BLUEDROID_STATUS_ENABLED;
  }

  bool enable()
  {
    if (esp_bluedroid_get_status() != ESP_BLUEDROID_STATUS_UNINITIALIZED)
    {
      ESP_LOGW(BT_HF_TAG, "Bluetooth already enabled (Status: %d)", esp_bluedroid_get_status());
      return false;
    }

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
        return false;
    }

    if ((ret = esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT)) != ESP_OK) {
        ESP_LOGE(BT_HF_TAG, "%s enable controller failed: %s", __func__, esp_err_to_name(ret));
        return false;
    }

    if ((ret = esp_bluedroid_init()) != ESP_OK) {
        ESP_LOGE(BT_HF_TAG, "%s initialize bluedroid failed: %s", __func__, esp_err_to_name(ret));
        return false;
    }

    if ((ret = esp_bluedroid_enable()) != ESP_OK) {
        ESP_LOGE(BT_HF_TAG, "%s enable bluedroid failed: %s", __func__, esp_err_to_name(ret));
        return false;
    }

    ESP_LOGI(BT_HF_TAG, "Own address:[%s]", bda2str((uint8_t *)esp_bt_dev_get_address(), bda_str, sizeof(bda_str)));

    esp_bt_dev_set_device_name(BT_DEVICE_NAME);

    esp_bredr_tx_power_set(ESP_PWR_LVL_N3, ESP_PWR_LVL_P6);

    return true;
  }

  bool disable()
  {
    if (esp_bluedroid_get_status() != ESP_BLUEDROID_STATUS_ENABLED)
    {
      ESP_LOGW(BT_HF_TAG, "Bluetooth already disabled (Status: %d)", esp_bluedroid_get_status());
      return false;
    }

    if (esp_bluedroid_disable() != ESP_OK)
    {
      ESP_LOGE(BT_HF_TAG, "%s disable bluedroid failed\n", __func__);
      return false;
    }
    ESP_LOGD(BT_HF_TAG, "esp_bluedroid_get_status() after esp_bluedroid_disable() said: %d \n", esp_bluedroid_get_status());

    if (esp_bluedroid_deinit() != ESP_OK)
    {
      ESP_LOGE(BT_HF_TAG, "%s initialize bluedroid failed\n", __func__);
      return false;
    }
    ESP_LOGD(BT_HF_TAG, "esp_bluedroid_get_status() after esp_bluedroid_init() said: %d \n", esp_bluedroid_get_status());

    return true;
  }
private:

  char *bda2str(esp_bd_addr_t bda, char *str, size_t size)
  {
      if (bda == NULL || str == NULL || size < 18) {
          return NULL;
      }

      uint8_t *p = bda;
      sprintf(str, "%02x:%02x:%02x:%02x:%02x:%02x",
              p[0], p[1], p[2], p[3], p[4], p[5]);
      return str;
  }

} bluetooth;

extern BluetoothClient bluetooth;

#endif
#ifndef _DISCOVERY_SESSION_H
#define _DISCOVERY_SESSION_H

#include <string>
#include <map>
#include <esp_log.h>
#include <esp_bt.h>
#include <esp_bt_main.h>
#include <esp_bt_device.h>
#include <esp_gap_bt_api.h>
#include "BluetoothClient.h"
#include "DeviceInformation.h"

#define DS_TAG "DS_TAG"
#define GAP_TAG "GAP_TAG"

#define SCAN_MODE ESP_BT_SCAN_MODE_CONNECTABLE_DISCOVERABLE
// #define SCAN_MODE ESP_BT_SCAN_MODE_NONE

class DiscoverySession
{
  static DiscoverySession *ActiveSession;
  bool keepActive;

public:
  std::function<void(DeviceInformation)> callback;
  std::map<string, DeviceInformation> devices;

  DiscoverySession()
  {
  }

  ~DiscoverySession()
  {
    if (this == DiscoverySession::ActiveSession)
    {
      this->stop();
    }
  }

  void onNewDevice(std::function<void(DeviceInformation)> callback)
  {
    this->callback = callback;
  }

  bool start()
  {
    if (DiscoverySession::ActiveSession != nullptr)
    {
      ESP_LOGE(DS_TAG, "Only one active discovery session is allowed.");
      return false;
    }

    DiscoverySession::ActiveSession = this;
    keepActive = true;

    // register GAP callback function
    esp_err_t err;
    err = esp_bt_gap_register_callback([](esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param) { DiscoverySession::ActiveSession->bt_api_callback(event, param); });
    ESP_ERROR_CHECK(err);

    // Set default parameters for Secure Simple Pairing
    esp_bt_sp_param_t param_type = ESP_BT_SP_IOCAP_MODE;
    esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_IO;
    // esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_NONE;
    ESP_ERROR_CHECK(esp_bt_gap_set_security_param(param_type, &iocap, sizeof(uint8_t)));

    // Legacy PIN
    ESP_ERROR_CHECK(esp_bt_gap_set_pin(ESP_BT_PIN_TYPE_FIXED, 0, 0));

    // set discoverable and connectable mode
    // err = esp_bt_gap_set_scan_mode(ESP_BT_SCAN_MODE);
    ESP_ERROR_CHECK(esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE));
    ESP_ERROR_CHECK(esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY, 0x30, 0));

    // This function starts Inquiry and Name Discovery
    startInquiry();

    ESP_LOGD(DS_TAG, "Started discovery sesssion.");
    return true;
  }

  bool stop()
  {
    if (DiscoverySession::ActiveSession != this)
    {
      ESP_LOGE(DS_TAG, "Cannot stop discovery session that is not active.");
      return false;
    }

    keepActive = false;
    esp_err_t err = esp_bt_gap_cancel_discovery();
    ESP_ERROR_CHECK(err);
    ESP_ERROR_CHECK(esp_bt_gap_set_scan_mode(ESP_BT_NON_CONNECTABLE, ESP_BT_NON_DISCOVERABLE));

    ESP_LOGD(DS_TAG, "Stopped discovery sesssion.");
    return true;
  }

  void clear()
  {
    devices.clear();
  }

private:
  void startInquiry()
  {
    // This function starts Inquiry and Name Discovery
    esp_err_t err = esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY, 20, 0);
    ESP_ERROR_CHECK(err);
  }

  // This is how the BT Stack communicates information to us.
  void bt_api_callback(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
  {
    // ESP_LOGD(DS_TAG, "Received event %d", event);

    if (event == ESP_BT_GAP_DISC_RES_EVT)
    {
      char bda_str[18];
      // esp_bd_addr_t address;
      BluetoothAddress address;
      uint32_t cod = 0;
      int rssi = -129; /* invalid value */
      uint8_t bdname_len;
      char bdname[ESP_BT_GAP_MAX_BDNAME_LEN];
      uint8_t eir[ESP_BT_GAP_EIR_DATA_LEN];
      uint8_t eir_len;
      esp_bt_gap_dev_prop_t *p;

      address = param->disc_res.bda;

      // ESP_LOGI(GAP_TAG, "Device found: %s", bda2str(param->disc_res.bda, bda_str, 18));

      if (devices.count(address.toString()) == 1)
      {
        // ESP_LOGI(GAP_TAG, "Found an old device again.");
        return;
      }

      ESP_LOGI(GAP_TAG, "Device found: %s", address.toString().c_str());
      // for (int i = 0; i < param->disc_res.num_prop; i++)
      // {
      //   p = param->disc_res.prop + i;
      //   switch (p->type)
      //   {
      //   case ESP_BT_GAP_DEV_PROP_COD:
      //     cod = *(uint32_t *)(p->val);
      //     ESP_LOGI(GAP_TAG, "--Class of Device: 0x%x", cod);
      //     break;
      //   case ESP_BT_GAP_DEV_PROP_RSSI:
      //     rssi = *(int8_t *)(p->val);
      //     ESP_LOGI(GAP_TAG, "--RSSI: %d", rssi);
      //     break;
      //   case ESP_BT_GAP_DEV_PROP_BDNAME:
      //   default:
      //     break;
      //   }
      // }

      /* search for device with MAJOR service class as "rendering" in COD */
      // app_gap_cb_t *p_dev = &m_dev_info;
      // if (p_dev->dev_found && 0 != memcmp(param->disc_res.bda, p_dev->bda, ESP_BD_ADDR_LEN))
      // {
      //   return;
      // }

      // if (!esp_bt_gap_is_valid_cod(cod) ||
      //     !(esp_bt_gap_get_cod_major_dev(cod) == ESP_BT_COD_MAJOR_DEV_PHONE))
      // {
      //   return;
      // }

      // memcpy(p_dev->bda, param->disc_res.bda, ESP_BD_ADDR_LEN);
      // p_dev->dev_found = true;
      for (int i = 0; i < param->disc_res.num_prop; i++)
      {
        p = param->disc_res.prop + i;
        switch (p->type)
        {
        case ESP_BT_GAP_DEV_PROP_COD:
          cod = *(uint32_t *)(p->val);
          ESP_LOGI(GAP_TAG, "--Class of Device: 0x%x", cod);
          break;
        case ESP_BT_GAP_DEV_PROP_RSSI:
          rssi = *(int8_t *)(p->val);
          ESP_LOGI(GAP_TAG, "--RSSI: %d", rssi);
          break;
        case ESP_BT_GAP_DEV_PROP_BDNAME:
        {
          bdname_len = (p->len > ESP_BT_GAP_MAX_BDNAME_LEN) ? ESP_BT_GAP_MAX_BDNAME_LEN : (uint8_t)p->len;
          memcpy(&bdname, (uint8_t *)(p->val), bdname_len);
          bdname[bdname_len] = '\0';
          ESP_LOGD(GAP_TAG, "--? name: %s", bdname);
          break;
        }
        case ESP_BT_GAP_DEV_PROP_EIR:
        {
          eir_len = p->len;
          memcpy(&eir, (uint8_t *)(p->val), eir_len);
          break;
        }
        default:
          break;
        }
      }

      bool name_res = get_name_from_eir(eir, bdname, &bdname_len);
      if (!name_res)
        return;

      ESP_LOGD(GAP_TAG, "--name: %s", bdname);

      uint8_t *raw_addr = address.value;
      ESP_LOGD(GAP_TAG, "--raw_addr: %d %d %d %d %d %d", raw_addr[0], raw_addr[1], raw_addr[2], raw_addr[3], raw_addr[4], raw_addr[5]);

      DeviceInformation deviceInfo = {
          .address = address,
          .name = bdname,
          .rssi = rssi};
      devices[deviceInfo.address.toString()] = deviceInfo;
      this->callback(deviceInfo);
    }
    else if (event == ESP_BT_GAP_DISC_STATE_CHANGED_EVT)
    {
      if (param->disc_st_chg.state == ESP_BT_GAP_DISCOVERY_STOPPED)
      {
        if (keepActive)
        {
          ESP_LOGD(GAP_TAG, "Restarting device discovery.");
          startInquiry();
        }
        else
        {
          DiscoverySession::ActiveSession = nullptr;
          ESP_LOGD(GAP_TAG, "Device discovery stopped.");
        }
      }
    }
    else if (event == ESP_BT_GAP_AUTH_CMPL_EVT)
    {
      if (param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS)
      {
        ESP_LOGI("", "authentication success: %s", param->auth_cmpl.device_name);
        esp_log_buffer_hex("", param->auth_cmpl.bda, ESP_BD_ADDR_LEN);
      }
      else
      {
        ESP_LOGE("", "authentication failed, status:%d", param->auth_cmpl.stat);
      }
    }
    else if (event == ESP_BT_GAP_KEY_REQ_EVT)
    {
      ESP_LOGI("", "ESP_BT_GAP_KEY_REQ_EVT");
      // esp_bt_pin_code_t pin_code = {0};
      // esp_bt_gap_ssp_passkey_reply(param->cfm_req.bda, true, 0);
    }
    else if (event == ESP_BT_GAP_CFM_REQ_EVT)
    {
      ESP_LOGI("", "ESP_BT_GAP_CFM_REQ_EVT Please compare the numeric value: %d", param->cfm_req.num_val);
      esp_bt_gap_ssp_confirm_reply(param->cfm_req.bda, true);
    }
  }

  static char *bda2str(esp_bd_addr_t bda, char *str, size_t size)
  {
    if (bda == NULL || str == NULL || size < 18)
    {
      return NULL;
    }

    uint8_t *p = bda;
    sprintf(str, "%02x:%02x:%02x:%02x:%02x:%02x",
            p[0], p[1], p[2], p[3], p[4], p[5]);
    return str;
  }

  // Try to extract device name from the Bluetooth Extended Inquiry Response.
  static bool get_name_from_eir(uint8_t *eir, char *bdname, uint8_t *bdname_len)
  {

    uint8_t *rmt_bdname = NULL;
    uint8_t rmt_bdname_len = 0;

    if (!eir)
    {
      return false;
    }

    // Try to get long name
    rmt_bdname = esp_bt_gap_resolve_eir_data(eir, ESP_BT_EIR_TYPE_CMPL_LOCAL_NAME, &rmt_bdname_len);
    if (!rmt_bdname)
    {
      // Try to get short name
      rmt_bdname = esp_bt_gap_resolve_eir_data(eir, ESP_BT_EIR_TYPE_SHORT_LOCAL_NAME, &rmt_bdname_len);
    }

    if (rmt_bdname)
    {
      // If the name is larger that the max (not sure why this length)
      if (rmt_bdname_len > ESP_BT_GAP_MAX_BDNAME_LEN)
      {
        rmt_bdname_len = ESP_BT_GAP_MAX_BDNAME_LEN;
      }

      // If we were provided with an output variable
      if (bdname)
      {
        // Copy the name from the EIR to the variable, making sure to null-terminate it.
        memcpy(bdname, rmt_bdname, rmt_bdname_len);
        bdname[rmt_bdname_len] = '\0';
      }
      if (bdname_len)
      {
        // Also output the length
        *bdname_len = rmt_bdname_len;
      }
      return true;
    }

    return false;
  }
};

DiscoverySession *DiscoverySession::ActiveSession;

#endif

/*
 * SPDX-FileCopyrightText: 2021 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#ifndef __BT_APP_HF_H__
#define __BT_APP_HF_H__

#include <stdint.h>
#include "esp_hf_client_api.h"


#define BT_HF_TAG "BT_HF"

#ifdef __cplusplus
extern "C" {
#endif

void bt_app_hf_client_audio_open(void);
void bt_app_hf_client_audio_close(void);
uint32_t bt_app_hf_client_outgoing_cb(uint8_t *p_buf, uint32_t sz);
void bt_app_hf_client_incoming_cb(const uint8_t *buf, uint32_t sz);
/**
 * @brief     callback function for HF client
 */
void bt_app_hf_client_cb(esp_hf_client_cb_event_t event, esp_hf_client_cb_param_t *param);

#ifdef __cplusplus
}
#endif

#endif /* __BT_APP_HF_H__*/

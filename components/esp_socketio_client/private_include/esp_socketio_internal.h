/*
 * SPDX-FileCopyrightText: 2020-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ESP_SOCKETIO_INTERNAL_H_
#define _ESP_SOCKETIO_INTERNAL_H_

// #include "sys/time.h"
// #include "esp_transport.h"
// #include "sys/socket.h"
// #include "sys/queue.h"
#include "esp_log.h"
// #include "esp_tls.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief      Utility macro to be used for NULL ptr check after malloc
 *
 */
#define ESP_SOCKETIO_MEM_CHECK(TAG, a, action) if (!(a)) {                                         \
        ESP_LOGE(TAG,"%s(%d): %s", __FUNCTION__, __LINE__, "Memory exhausted");                     \
        action;                                                                                     \
        }

/**
 * @brief      Utility macro for checking the error code of esp_err_t
 */
#define ESP_SOCKETIO_ERR_OK_CHECK(TAG, err, action) \
        {                                                     \
            esp_err_t _esp_socketio_err_to_check = err;      \
            if (_esp_socketio_err_to_check != ESP_OK) {      \
                ESP_LOGE(TAG,"%s(%d): Expected ESP_OK; reported: %d", __FUNCTION__, __LINE__, _esp_socketio_err_to_check); \
                action;                                       \
            }                                                 \
        }

#ifdef __cplusplus
}
#endif

#endif //_ESP_SOCKETIO_INTERNAL_H_

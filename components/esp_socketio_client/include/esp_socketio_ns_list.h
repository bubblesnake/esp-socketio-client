/*
 * SPDX-FileCopyrightText: 2015-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ESP_SOCKETIO_NS_LIST_H_
#define _ESP_SOCKETIO_NS_LIST_H_


#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct esp_socketio_ns_list *esp_socketio_ns_list_handle_t;

/**
 * @brief Create a new empty namespace list by allocating memory for the structure and initializing the head pointer to NULL.
 *             This function must be the first function to call.
 *             It returns a esp_socketio_ns_list_handle_t that you must use as input to other functions in the interface.
 *             This call MUST have a corresponding call to esp_socketio_ns_list_destroy when the operation on the namespace list is complete.
 *
 * @return
 *     - `esp_socketio_ns_list_handle_t`
 *     - NULL if any errors
 */
esp_socketio_ns_list_handle_t esp_socketio_ns_list_create();

esp_err_t esp_socketio_ns_list_add_ns(esp_socketio_ns_list_handle_t ns_list, char *nsp, char *sid);

char *esp_socketio_ns_list_search_sid(esp_socketio_ns_list_handle_t ns_list, char *nsp);

bool esp_socketio_ns_list_is_nsp_exist(esp_socketio_ns_list_handle_t ns_list, const char *nsp);

int esp_socketio_ns_list_get_num(esp_socketio_ns_list_handle_t ns_list);

esp_err_t esp_socketio_ns_list_delete_ns(esp_socketio_ns_list_handle_t ns_list, char *nsp);

void esp_socketio_ns_list_destroy(esp_socketio_ns_list_handle_t ns_list);

#ifdef __cplusplus
}
#endif

#endif

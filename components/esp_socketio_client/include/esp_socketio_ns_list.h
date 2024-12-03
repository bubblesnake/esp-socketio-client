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
 * @brief Create a new empty namespace list by allocating memory for the structure and
 *             initializing the head pointer to NULL.
 *             This function must be the first function to call.
 *             It returns a esp_socketio_ns_list_handle_t that you must use as input to
 *             other functions in the interface.
 *             This call MUST have a corresponding call to esp_socketio_ns_list_destroy
 *             when the operation on the namespace list is complete.
 *
 * @return
 *     - `esp_socketio_ns_list_handle_t`
 *     - NULL if any errors
 */
esp_socketio_ns_list_handle_t esp_socketio_ns_list_create();

/**
 * @brief Add a namespace to the namespace list.
 *
 * @param ns_list           The namespace list handle returned by esp_socketio_ns_list_create
 * @param nsp               The namespace name
 * @param sid               Session ID of the namespace
 * @return esp_err_t
 */
esp_err_t esp_socketio_ns_list_add_ns(esp_socketio_ns_list_handle_t ns_list, char *nsp, char *sid);

/**
 * @brief Search the sid of a namespace in the list.
 *
 * @param ns_list           The namespace list handle returned by esp_socketio_ns_list_create
 * @param nsp               The namespace name
 * @return char *           The sid string
 */
char *esp_socketio_ns_list_search_sid(esp_socketio_ns_list_handle_t ns_list, char *nsp);

/**
 * @brief Check if a namespace exist in the namespace list.
 *
 * @param ns_list           The namespace list handle returned by esp_socketio_ns_list_create
 * @param nsp               The namespace name
 * @return bool
 */
bool esp_socketio_ns_list_is_nsp_exist(esp_socketio_ns_list_handle_t ns_list, const char *nsp);

/**
 * @brief Get number of namespaces in the namespace list.
 *
 * @param ns_list           The namespace list handle returned by esp_socketio_ns_list_create
 * @return int
 */
int esp_socketio_ns_list_get_num(esp_socketio_ns_list_handle_t ns_list);

/**
 * @brief Delete a namespace from the namespace list.
 *
 * @param ns_list           The namespace list handle returned by esp_socketio_ns_list_create
 * @param nsp               The namespace name
 * @return esp_err_t
 */
esp_err_t esp_socketio_ns_list_delete_ns(esp_socketio_ns_list_handle_t ns_list, char *nsp);

/**
 * @brief Destroy the namespace list.
 *             This function must be the last function to call. It destroy all resources
 *             associated with the namespace list handler and the handle itself.
 *
 * @param ns_list           The namespace list handle returned by esp_socketio_ns_list_create
 * @return void
 */
void esp_socketio_ns_list_destroy(esp_socketio_ns_list_handle_t ns_list);

#ifdef __cplusplus
}
#endif

#endif

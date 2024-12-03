/*
 * SPDX-FileCopyrightText: 2015-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ESP_SOCKETIO_CLIENT_H_
#define _ESP_SOCKETIO_CLIENT_H_


#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "esp_err.h"
#include "esp_event.h"
#include <sys/socket.h>
#include "esp_websocket_client.h"
#include "esp_socketio_packet.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct esp_socketio_client *esp_socketio_client_handle_t;

ESP_EVENT_DECLARE_BASE(SOCKETIO_EVENTS);         // declaration of the task events family

/**
 * @brief Socket.IO Client events id
 */
typedef enum {
    SOCKETIO_EVENT_ANY = -1,
    SOCKETIO_EVENT_ERROR = 0,               /*!< This event occurs when there are any errors during execution */
    SOCKETIO_EVENT_OPENED,                  /*!< Socket.IO server has sent open packet */
    SOCKETIO_EVENT_NS_CONNECTED,            /*!< A Socket.IO namespace has been connected. */
    SOCKETIO_EVENT_DATA,                    /*!< When receiving data from the server, possibly multiple portions of the packet */
    SOCKETIO_EVENT_MAX
} esp_socketio_event_id_t;

typedef struct {
    esp_websocket_event_id_t        websocket_event_id;
    esp_websocket_event_data_t      *websocket_event;
    esp_socketio_packet_handle_t    socketio_packet;
    esp_socketio_client_handle_t    client;
} esp_socketio_event_data_t;

typedef struct {
    esp_websocket_client_config_t websocket_config;
} esp_socketio_client_config_t;

ESP_EVENT_DECLARE_BASE(SOCKETIO_EVENTS);         // declaration of the task events family

esp_socketio_client_handle_t esp_socketio_client_init(const esp_socketio_client_config_t *config);

/**
 * @brief      Open the Socketio.IO connection
 *
 * @param[in]  client  The client handle
 *
 * @return     esp_err_t
 */
esp_err_t esp_socketio_client_start(esp_socketio_client_handle_t client);

/**
 * @brief Connect to a Socket.IO namespace
 *
 * @param client            The client handle
 * @param nsp               The namespace name
 * @param data              Additional payload to be appended to the CONNECT packet
 * @return esp_err_t
 */
esp_err_t esp_socketio_client_connect_nsp(esp_socketio_client_handle_t client, const char *nsp, const cJSON *data);

/**
 * @brief Send a Socket.IO packet
 *
 * @param client            The client handle
 * @param packet            The handle of the packet to be sent
 * @return esp_err_t
 */
esp_err_t esp_socketio_client_send_data(esp_socketio_client_handle_t client, esp_socketio_packet_handle_t packet);

/**
 * @brief      Send Engine.IO close packet and close the WebSocket connection in a clean way
 *
 * Sequence of clean close initiated by client:
 *
 *  Notes:
 *  - Cannot be called from the websocket event handler
 *
 * @param[in]  client  The client
 * @param[in]  timeout Timeout in RTOS ticks for waiting
 *
 * @return     esp_err_t
 */
esp_err_t esp_socketio_client_close(esp_socketio_client_handle_t client, TickType_t timeout);

/**
 * @brief      Call esp_websocket_client_destroy and free all Socket.IO client resources.
 *             This function must be the last function to call for an session.
 *             It is the opposite of the esp_socketio_client_init function and must be called
 *             with the same handle as returned by esp_websocket_client_init.
 *             This may close all connections this handle has used.
 *
 *  Notes:
 *  - Cannot be called from the websocket event handler
 *
 * @param[in]  client  The client
 *
 * @return     esp_err_t
 */
esp_err_t esp_socketio_client_destroy(esp_socketio_client_handle_t client);

/**
 * @brief      Get the handle to a packet pre-allocated for transmission.
 *
 * @param[in]  client  The client
 *
 * @return     esp_socketio_packet_handle_t handle to the esp_socketio_packet struct.
 */
esp_socketio_packet_handle_t esp_socketio_client_get_tx_packet(esp_socketio_client_handle_t client);

/**
 * @brief      Get the maxPayload value returned in the server's OPEN packet.
 *             This function must be called after OPEN.
 *
 * @param[in]  client  The client
 *
 * @return     maxPayload value, or -1 if error.
 */
int esp_socketio_client_get_max_payload(esp_socketio_client_handle_t client);

/**
 * @brief Register the Socket.IO Events
 *
 * @param client            The client handle
 * @param event             The event id
 * @param event_handler     The callback function
 * @param event_handler_arg User context
 * @return esp_err_t
 */
esp_err_t esp_socketio_register_events(esp_socketio_client_handle_t client,
                                        esp_socketio_event_id_t event,
                                        esp_event_handler_t event_handler,
                                        void *event_handler_arg);


#ifdef __cplusplus
}
#endif

#endif

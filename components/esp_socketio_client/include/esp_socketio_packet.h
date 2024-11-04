/*
 * SPDX-FileCopyrightText: 2015-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ESP_SOCKETIO_PACKET_H_
#define _ESP_SOCKETIO_PACKET_H_


#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <cJSON.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ESP_SOCKETIO_CLIENT_SID_LEN     (20)

typedef struct esp_socketio_packet *esp_socketio_packet_handle_t;

typedef enum {
    EIO_PACKET_TYPE_UNKNOWN = 0,
    EIO_PACKET_TYPE_OPEN    = '0',
    EIO_PACKET_TYPE_CLOSE   = '1',
    EIO_PACKET_TYPE_PING    = '2',
    EIO_PACKET_TYPE_PONG    = '3',
    EIO_PACKET_TYPE_MESSAGE = '4',
    EIO_PACKET_TYPE_UPGRADE = '5',
    EIO_PACKET_TYPE_NOOP    = '6',
} esp_engineio_packet_type_t;

typedef enum {
    SIO_PACKET_TYPE_UNKNOWN         = 0,
    SIO_PACKET_TYPE_CONNECT         = '0',
    SIO_PACKET_TYPE_DISCONNECT      = '1',
    SIO_PACKET_TYPE_EVENT           = '2',
    SIO_PACKET_TYPE_ACK             = '3',
    SIO_PACKET_TYPE_CONNECT_ERROR   = '4',
    SIO_PACKET_TYPE_BINARY_EVENT    = '5',
    SIO_PACKET_TYPE_BINARY_ACK      = '6',
} esp_socketio_packet_type_t;

/**
 * @brief Initialize memory space for a Socket.IO packet
 *             This function must be the first function to call.
 *             It returns a esp_socketio_packet_handle_t that you must use as input to other functions in the interface.
 *             The function esp_socketio_packet_reset can be used to clear the members in the packet and reuse the handle.
 *             This call MUST have a corresponding call to esp_socketio_packet_destroy when the operation on the packet is complete.
 *
 * @param[in] eio_type      The Engine.IO packet type.
 * @param[in] nsp           The Socket.IO packet type.
 * @param[in] nsp           The namespace string. NULL means use default namespace "/".
 * @param[in] event_id      The Socket.IO event ID. Negative value means no ID present.
 *
 * @return
 *     - `esp_socketio_packet_handle_t`
 *     - NULL if any errors
 */
esp_socketio_packet_handle_t esp_socketio_packet_init();

/**
 * @brief Set header of a Socket.IO packet
 *
 * @param[in] packet        The packet.
 * @param[in] eio_type      The Engine.IO packet type.
 * @param[in] sio_type      The Socket.IO packet type.
 * @param[in] nsp           The namespace string. NULL means use default namespace "/".
 * @param[in] event_id      The Socket.IO event ID. Negative value means no ID present.
 *
 * @return
 *     - `esp_socketio_packet_handle_t`
 *     - NULL if any errors
 */
esp_err_t esp_socketio_packet_set_header(esp_socketio_packet_handle_t packet, esp_engineio_packet_type_t eio_type, esp_socketio_packet_type_t sio_type, char *nsp, int event_id);

/**
 * @brief Destroy the Socket.IO packet and free all resources.
 *             This function must be the last function to call for a packet. It is the opposite of the `esp_socketio_packet_init`
 *             or `esp_socketio_packet_parse` and must be called with the same handle as input returned by either of the two functions.
 *
 * @param[in] packet            The packet
 *
 * @return    esp_err_t
 */
esp_err_t esp_socketio_packet_reset(esp_socketio_packet_handle_t packet);

void esp_socketio_packet_destroy(esp_socketio_packet_handle_t packet);

/**
 * @brief Return the Engine.IO type of the Socket.IO packet. The packet must be parsed before this call. Otherwise EIO_PACKET_TYPE_UNKNOWN is returned.
 *
 * @param[in] packet            The packet
 *
 * @return    esp_engineio_packet_type_t
 *
 */
esp_engineio_packet_type_t esp_socketio_packet_get_eio_type(esp_socketio_packet_handle_t packet);

/**
 * @brief Return the Socket.IO type of the Socket.IO packet. The packet must be parsed before this call. Otherwise SIO_PACKET_TYPE_UNKNOWN is returned.
 *
 * @param[in] packet            The packet
 *
 * @return    esp_socketio_packet_type_t
 *
 */
esp_socketio_packet_type_t esp_socketio_packet_get_sio_type(esp_socketio_packet_handle_t packet);

char *esp_socketio_packet_get_nsp(esp_socketio_packet_handle_t packet);

cJSON *esp_socketio_packet_get_json(esp_socketio_packet_handle_t packet);

char *esp_socketio_packet_get_raw_data(esp_socketio_packet_handle_t packet, int *data_len_ptr);

int esp_socketio_packet_get_event_id(esp_socketio_packet_handle_t packet);

esp_err_t esp_socketio_packet_set_json(esp_socketio_packet_handle_t packet, const cJSON *json);

int esp_socketio_packet_add_binary_data(esp_socketio_packet_handle_t packet, const unsigned char *data, size_t data_size, bool increment);

void esp_socketio_packet_destroy_binary_data(esp_socketio_packet_handle_t packet);

int esp_socketio_packet_count_binary_data(esp_socketio_packet_handle_t packet);

int esp_socketio_packet_get_last_binary_index(esp_socketio_packet_handle_t packet);

esp_err_t esp_socketio_packet_get_current_binary_data(esp_socketio_packet_handle_t packet, unsigned char **data_ptr, size_t *data_size_ptr, int *index_ptr);

esp_err_t esp_socketio_parse_open_packet(const char *buf, int len, char *sid_ptr, int *ping_interval_ptr, int *ping_timeout_ptr, int *max_payload_ptr);

esp_err_t esp_socketio_packet_parse_message(esp_socketio_packet_handle_t packet, const char *buf, int len);

esp_err_t esp_socketio_packet_encode_message(esp_socketio_packet_handle_t packet);

#ifdef __cplusplus
}
#endif

#endif

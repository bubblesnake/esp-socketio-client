/*
 * SPDX-FileCopyrightText: 2015-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#include "esp_socketio_client.h"
#include "esp_transport.h"
#include "esp_transport_tcp.h"
#include "esp_transport_ssl.h"
/* using uri parser */
#include "http_parser.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_tls_crypto.h"
#include "esp_system.h"
#include <errno.h>
#include <arpa/inet.h>
#include "esp_socketio_ns_list.h"
#include "esp_socketio_internal.h"

static const char *TAG = "socketio_client";

#define SOCKETIO_EVENT_QUEUE_SIZE       (1)

ESP_EVENT_DEFINE_BASE(SOCKETIO_EVENTS);

// Socket.IO states
typedef enum {
    SOCKETIO_STATE_INIT = 0,
    SOCKETIO_STATE_HANDSHAKE,
    SOCKETIO_STATE_OPENED,
    SOCKETIO_STATE_CONNECTED,
    SOCKETIO_STATE_WAIT_FOR_BINARY,
    SOCKETIO_STATE_DISCONNECTED,
    SOCKETIO_STATE_CLOSED,
} socketio_client_state_t;

struct esp_socketio_client {
    esp_websocket_client_handle_t   ws_client;
    esp_event_loop_handle_t         event_handle;
    esp_timer_handle_t              sio_ping_timer;
    esp_socketio_ns_list_handle_t   ns_list;
    esp_socketio_packet_handle_t    rx_packet;
    esp_socketio_packet_handle_t    tx_packet;
    socketio_client_state_t         socketio_state;
    char                            sid[ESP_SOCKETIO_CLIENT_SID_LEN+1];
    int                             ping_interval;
    int                             ping_timeout;
    int                             max_payload;
};

static esp_err_t esp_sio_client_dispatch_event(esp_socketio_client_handle_t client,
        esp_socketio_event_id_t event,
        const void *data,
        int data_len)
{
    esp_err_t err;

    if ((err = esp_event_post_to(client->event_handle,
                                 SOCKETIO_EVENTS, event,
                                 data,
                                 sizeof(esp_socketio_event_data_t),
                                 portMAX_DELAY)) != ESP_OK) {
        return err;
    }
    return esp_event_loop_run(client->event_handle, 0);
}

static void sio_ping_interval_callback(void* arg)
{
    ESP_LOGE(TAG, "Ping timer expired!");
    esp_socketio_client_handle_t client = (esp_socketio_client_handle_t)arg;
    esp_socketio_event_data_t socketio_event_data;
    socketio_event_data.websocket_event_id = WEBSOCKET_EVENT_ANY;
    socketio_event_data.websocket_event = NULL;
    socketio_event_data.socketio_packet = NULL;
    socketio_event_data.client = client;

    esp_sio_client_dispatch_event(client, SOCKETIO_EVENT_ERROR, &socketio_event_data, sizeof(esp_socketio_event_data_t));
}

static void websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;
    esp_socketio_client_handle_t client = (esp_socketio_client_handle_t)handler_args;
    esp_socketio_event_data_t socketio_event_data;
    socketio_event_data.websocket_event_id = event_id;
    socketio_event_data.websocket_event = data;
    socketio_event_data.socketio_packet = NULL;
    socketio_event_data.client = client;

    switch (event_id) {
    case WEBSOCKET_EVENT_DATA:
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_DATA");
        ESP_LOGI(TAG, "Received opcode=%d", data->op_code);
        if (data->op_code == WS_TRANSPORT_OPCODES_CLOSE && data->data_len == 2) {
            ESP_LOGW(TAG, "Received closed message with code=%d", 256 * data->data_ptr[0] + data->data_ptr[1]);
        } else {
            ESP_LOGW(TAG, "Received=%.*s", data->data_len, (char *)data->data_ptr);
        }

        if (data->op_code == WS_TRANSPORT_OPCODES_PONG) {
            ESP_LOGW(TAG, "Received WS PONG");
        }

        ESP_LOGI(TAG, "Total payload length=%d, data_len=%d, current payload offset=%d\r\n", data->payload_len, data->data_len, data->payload_offset);

        if (data->payload_len == data->data_len && data->payload_len > 0) {
            if (data->op_code == WS_TRANSPORT_OPCODES_TEXT) {
                if (SOCKETIO_STATE_HANDSHAKE == client->socketio_state) {
                    if (EIO_PACKET_TYPE_OPEN == data->data_ptr[0]) {
                        if (esp_socketio_parse_open_packet(
                                &data->data_ptr[1],
                                data->data_len - 1,
                                client->sid,
                                &client->ping_interval,
                                &client->ping_timeout,
                                &client->max_payload
                            ) == ESP_OK) {
                            ESP_LOGI(TAG, "Start Socket.IO ping timer: %d ms", (client->ping_interval + client->ping_timeout));
                            ESP_ERROR_CHECK(esp_timer_start_once(client->sio_ping_timer, (client->ping_interval + client->ping_timeout) * 1000));
                            // Send event OPEN
                            client->socketio_state = SOCKETIO_STATE_OPENED;
                            esp_sio_client_dispatch_event(client, SOCKETIO_EVENT_OPENED, &socketio_event_data, sizeof(esp_socketio_event_data_t));
                        }
                    }
                }

                if ((SOCKETIO_STATE_OPENED == client->socketio_state || SOCKETIO_STATE_CONNECTED == client->socketio_state)) {
                    if (EIO_PACKET_TYPE_MESSAGE == data->data_ptr[0]) {
                        if (esp_socketio_packet_parse_message(client->rx_packet, data->data_ptr, data->data_len) != ESP_OK) {
                            ESP_LOGE(TAG, "Error parsing message.");
                            break;
                        }
                        char *nsp = esp_socketio_packet_get_nsp(client->rx_packet);
                        esp_socketio_packet_type_t sio_type = esp_socketio_packet_get_sio_type(client->rx_packet);
                        switch (sio_type)
                        {
                        case SIO_PACKET_TYPE_CONNECT:
                            client->socketio_state = SOCKETIO_STATE_CONNECTED;
                            cJSON *json_sid = cJSON_GetObjectItem(esp_socketio_packet_get_json(client->rx_packet), "sid");
                            if (!(cJSON_IsString(json_sid) && json_sid->valuestring != NULL)) {
                                ESP_LOGE(TAG, "Error! No Socket.IO data found in CONNECT message.");
                                break;
                            }
                            ESP_LOGI(TAG, "Add namespace: %s, sid: %s", (nsp == NULL)? "/" : nsp, json_sid->valuestring);
                            esp_socketio_ns_list_add_ns(client->ns_list, nsp, json_sid->valuestring);
                            socketio_event_data.socketio_packet = client->rx_packet;
                            esp_sio_client_dispatch_event(client, SOCKETIO_EVENT_NS_CONNECTED, &socketio_event_data, sizeof(esp_socketio_event_data_t));
                            break;

                        case SIO_PACKET_TYPE_DISCONNECT:
                            if (esp_socketio_ns_list_delete_ns(client->ns_list, nsp) == ESP_ERR_NOT_FOUND) {
                                ESP_LOGE(TAG, "Namespace not found");
                            }
                            if (esp_socketio_ns_list_get_num(client->ns_list) <= 0) {
                                client->socketio_state = SOCKETIO_STATE_DISCONNECTED;
                            }
                            break;

                        case SIO_PACKET_TYPE_EVENT:
                        case SIO_PACKET_TYPE_ACK:
                            socketio_event_data.socketio_packet = client->rx_packet;
                            esp_sio_client_dispatch_event(client, SOCKETIO_EVENT_DATA, &socketio_event_data, sizeof(esp_socketio_event_data_t));
                            break;

                        case SIO_PACKET_TYPE_BINARY_EVENT:
                        case SIO_PACKET_TYPE_BINARY_ACK:
                            client->socketio_state = SOCKETIO_STATE_WAIT_FOR_BINARY;
                            break;

                        case SIO_PACKET_TYPE_CONNECT_ERROR:
                            ESP_LOGE(TAG, "Received CONNECT_ERROR");
                            break;

                        default:
                            break;
                        }
                    }
                }

                if (EIO_PACKET_TYPE_PING == data->data_ptr[0] && data->data_len == 1) {
                    ESP_LOGD(TAG, "Receive Engine.IO PING, sending PONG");
                    esp_timer_stop(client->sio_ping_timer);
                    char pong = EIO_PACKET_TYPE_PONG;
                    esp_websocket_client_send_text(client->ws_client, &pong, 1, portMAX_DELAY);
                    esp_timer_start_once(client->sio_ping_timer, (client->ping_interval + client->ping_timeout) * 1000);
                }
            }

            if (client->socketio_state == SOCKETIO_STATE_WAIT_FOR_BINARY && data->op_code == WS_TRANSPORT_OPCODES_BINARY) {
                ESP_LOGI(TAG, "Received binary");
                esp_socketio_packet_add_binary_data(client->rx_packet, (const unsigned char *)data->data_ptr, data->data_len, false);
                if (esp_socketio_packet_count_binary_data(client->rx_packet) == esp_socketio_packet_get_last_binary_index(client->rx_packet) + 1) {
                    client->socketio_state = SOCKETIO_STATE_CONNECTED;
                    socketio_event_data.socketio_packet = client->rx_packet;
                    esp_sio_client_dispatch_event(client, SOCKETIO_EVENT_DATA, &socketio_event_data, sizeof(esp_socketio_event_data_t));
                }
            }
        }

        break;
    }
    return;
}

static void esp_sio_client_destroy_and_free_client(esp_socketio_client_handle_t client)
{
    if (client == NULL) {
        return;
    }

    if (client->event_handle) {
        esp_event_loop_delete(client->event_handle);
    }

    esp_socketio_ns_list_destroy(client->ns_list);

    esp_socketio_packet_destroy(client->rx_packet);
    esp_socketio_packet_destroy(client->tx_packet);

    free(client);
    return;
}

esp_socketio_client_handle_t esp_socketio_client_init(const esp_socketio_client_config_t *config)
{
    esp_socketio_client_handle_t sio_client = calloc(1, sizeof(struct esp_socketio_client));
    if (!(sio_client)) {
        ESP_LOGE(TAG, "Error allocating socketio_client memory.");
        return NULL;
    }
    sio_client->ws_client = esp_websocket_client_init(&config->websocket_config);
    ESP_SOCKETIO_MEM_CHECK(TAG, sio_client->ws_client, {
        esp_sio_client_destroy_and_free_client(sio_client);
        return NULL;
    });

    sio_client->ns_list = esp_socketio_ns_list_create();
    ESP_SOCKETIO_MEM_CHECK(TAG, sio_client->ns_list, {
        esp_sio_client_destroy_and_free_client(sio_client);
        return NULL;
    });

    sio_client->rx_packet = esp_socketio_packet_init();
    sio_client->tx_packet = esp_socketio_packet_init();

    esp_event_loop_args_t event_args = {
        .queue_size = SOCKETIO_EVENT_QUEUE_SIZE,
        .task_name = NULL // no task will be created
    };

    if (esp_event_loop_create(&event_args, &sio_client->event_handle) != ESP_OK) {
        ESP_LOGE(TAG, "Error create event handler for websocket client");
        esp_sio_client_destroy_and_free_client(sio_client);
        return NULL;
    }

    esp_websocket_register_events(sio_client->ws_client, WEBSOCKET_EVENT_ANY, websocket_event_handler, (void *)sio_client);

    const esp_timer_create_args_t oneshot_timer_args = {
            .callback = &sio_ping_interval_callback,
            .arg = (void*) sio_client,
            .name = "sio_ping"
    };

    ESP_ERROR_CHECK(esp_timer_create(&oneshot_timer_args, &sio_client->sio_ping_timer));

    return sio_client;
}

esp_err_t esp_socketio_client_start(esp_socketio_client_handle_t client)
{
    client->socketio_state = SOCKETIO_STATE_HANDSHAKE;
    return esp_websocket_client_start(client->ws_client);
}

esp_err_t esp_socketio_client_connect_nsp(esp_socketio_client_handle_t client, const char *nsp)
{
    esp_err_t ret = ESP_OK;
    if (!esp_websocket_client_is_connected(client->ws_client)
        || client->socketio_state < SOCKETIO_STATE_OPENED
        || client->socketio_state > SOCKETIO_STATE_DISCONNECTED) {
        return ESP_ERR_INVALID_STATE;
    }

    if (esp_socketio_ns_list_is_nsp_exist(client->ns_list, nsp)) {
        ESP_LOGE(TAG, "Namespace \"%s\" already connected.", nsp);
        return ESP_ERR_INVALID_ARG;
    }

    char *sio_connect = NULL;
    int len = (nsp == NULL) ? 2 : 3 + strlen(nsp);
    sio_connect = calloc(1, len);
    if (sio_connect == NULL) {
        ESP_LOGE(TAG, "Error allocating sio_connect memory.");
        return ESP_ERR_NO_MEM;
    }
    sio_connect[0] = EIO_PACKET_TYPE_MESSAGE;
    sio_connect[1] = SIO_PACKET_TYPE_CONNECT;
    if (nsp != NULL) {
        sprintf(&sio_connect[2], "%s,", nsp);
    }
    int send_len = esp_websocket_client_send_text(client->ws_client, sio_connect, len, portMAX_DELAY);
    if (send_len > 0) {
        ESP_LOGI(TAG, "Send connect (size: %d) to \"%s\" successfully.", send_len, (nsp == NULL) ? "/" : nsp);
        ret = ESP_OK;
    } else {
        ESP_LOGE(TAG, "Send connect failed.");
        ret = ESP_FAIL;
    }
    free(sio_connect);
    return ret;
}

esp_err_t esp_socketio_client_send_data(esp_socketio_client_handle_t client, esp_socketio_packet_handle_t packet)
{
    esp_err_t ret = ESP_OK;
    if (!esp_websocket_client_is_connected(client->ws_client)) {
        return ESP_ERR_INVALID_STATE;
    }

    const char *nsp = esp_socketio_packet_get_nsp(packet);
    if (!esp_socketio_ns_list_is_nsp_exist(client->ns_list, nsp)) {
        ESP_LOGE(TAG, "Namespace \"%s\" not connected.", nsp);
        return ESP_ERR_INVALID_ARG;
    }

    esp_socketio_packet_encode_message(packet);
    int data_len = 0;
    char *data = esp_socketio_packet_get_raw_data(packet, &data_len);
    esp_websocket_client_send_text(client->ws_client, data, data_len, portMAX_DELAY);
    int binary_count = esp_socketio_packet_count_binary_data(packet);
    unsigned char *current_binary = NULL;
    size_t binary_size = 0;
    int binary_index = 0;
    while (binary_count--) {
        ret = esp_socketio_packet_get_current_binary_data(packet, &current_binary, &binary_size, &binary_index);
        if (ret == ESP_OK) {
            esp_websocket_client_send_bin(client->ws_client, (const char *)current_binary, (int)binary_size, portMAX_DELAY);
        }
    }
    return ESP_OK;
}

esp_err_t esp_socketio_client_close(esp_socketio_client_handle_t client, TickType_t timeout)
{
    char close_packet = EIO_PACKET_TYPE_CLOSE;
    esp_websocket_client_send_text(client->ws_client, &close_packet, 1, timeout);
    esp_websocket_client_close(client->ws_client, portMAX_DELAY);
    return ESP_OK;
}

esp_err_t esp_socketio_client_destroy(esp_socketio_client_handle_t client)
{
    ESP_LOGI(TAG, "%s called", __FUNCTION__);
    esp_websocket_client_destroy(client->ws_client);
    client->ws_client = NULL;
    esp_timer_stop(client->sio_ping_timer);
    esp_timer_delete(client->sio_ping_timer);
    esp_sio_client_destroy_and_free_client(client);
    return ESP_OK;
}

esp_socketio_packet_handle_t esp_socketio_client_get_tx_packet(esp_socketio_client_handle_t client)
{
    if (client == NULL) {
        return NULL;
    }
    return client->tx_packet;
}

int esp_socketio_client_get_max_payload(esp_socketio_client_handle_t client)
{
    if (!esp_websocket_client_is_connected(client->ws_client) || SOCKETIO_STATE_OPENED != client->socketio_state) {
        return -1;
    }
    return client->max_payload;
}

esp_err_t esp_socketio_register_events(esp_socketio_client_handle_t client,
                                        esp_socketio_event_id_t event,
                                        esp_event_handler_t event_handler,
                                        void *event_handler_arg)
{
    if (client == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    return esp_event_handler_register_with(client->event_handle, SOCKETIO_EVENTS, event, event_handler, event_handler_arg);
}

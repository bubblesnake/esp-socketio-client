/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdio.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"

#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_socketio_client.h"

static const char *TAG = "socketio";
static esp_socketio_packet_handle_t tx_packet = NULL;

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

static void log_websocket_event_error(esp_websocket_event_data_t *ws_event_data)
{
    log_error_if_nonzero("HTTP status code",  ws_event_data->error_handle.esp_ws_handshake_status_code);
    if (ws_event_data->error_handle.error_type == WEBSOCKET_ERROR_TYPE_TCP_TRANSPORT) {
        log_error_if_nonzero("reported from esp-tls", ws_event_data->error_handle.esp_tls_last_esp_err);
        log_error_if_nonzero("reported from tls stack", ws_event_data->error_handle.esp_tls_stack_err);
        log_error_if_nonzero("captured as transport's socket errno",  ws_event_data->error_handle.esp_transport_sock_errno);
    }
    return;
}

static void print_socketio_packet(esp_socketio_packet_handle_t packet)
{
    ESP_LOGI(TAG, "EIO: %c", esp_socketio_packet_get_eio_type(packet));
    ESP_LOGI(TAG, "SIO: %c", esp_socketio_packet_get_sio_type(packet));
    char *nsp = esp_socketio_packet_get_nsp(packet);
    ESP_LOGI(TAG, "Namespace: %s", nsp);
    ESP_LOGI(TAG, "Event ID: %d", esp_socketio_packet_get_event_id(packet));
    cJSON *json = esp_socketio_packet_get_json(packet);
    char *print_string = cJSON_Print(json);
    ESP_LOGI(TAG, "%s", print_string);
    // print_cJSON(json);
    free(print_string);
    int num = esp_socketio_packet_count_binary_data(packet);
    while (num-- > 0) {
        unsigned char *bin_data;
        size_t bin_size;
        int bin_index;
        esp_socketio_packet_get_current_binary_data(packet, &bin_data, &bin_size, &bin_index);
        if (bin_data != NULL) {
            printf("+ <Buffer %d <", bin_index);
            for (int i = 0; i < bin_size; i++) {
                printf("%02X%c", bin_data[i], (i == bin_size - 1) ? '\0' : ' ');
            }
            printf(">>\n");
        }

    }
}

static void socketio_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_socketio_event_data_t *data = (esp_socketio_event_data_t *)event_data;
    esp_websocket_event_id_t ws_event_id = data->websocket_event_id;
    esp_websocket_event_data_t *ws_event_data = data->websocket_event;
    esp_socketio_packet_handle_t packet = data->socketio_packet;
    switch (ws_event_id) {
    case WEBSOCKET_EVENT_CONNECTED:
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_CONNECTED");
        break;
    case WEBSOCKET_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_DISCONNECTED");
        log_websocket_event_error(ws_event_data);
        break;
    case WEBSOCKET_EVENT_DATA:
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_DATA");

        break;
    case WEBSOCKET_EVENT_ERROR:
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_ERROR");
        log_websocket_event_error(ws_event_data);
        break;
    default:
        break;
    }

    switch (event_id)
    {
    case SOCKETIO_EVENT_OPENED:
        ESP_LOGI(TAG, "Received Socket.IO OPEN packet.");
        esp_socketio_client_connect_nsp(data->client, NULL);
        break;

    case SOCKETIO_EVENT_NS_CONNECTED:
        char * nsp = esp_socketio_packet_get_nsp(packet);
        if (strcmp(nsp, "/") == 0) {
            ESP_LOGI(TAG, "Socket.IO connected to default namespace \"/\"");
            esp_socketio_client_connect_nsp(data->client, "/chat");
        } else {
            ESP_LOGI(TAG, "Socket.IO connected to namespace: \"%s\"", nsp);
            if (esp_socketio_packet_set_header(tx_packet, EIO_PACKET_TYPE_MESSAGE, SIO_PACKET_TYPE_BINARY_EVENT, nsp, 0) == ESP_OK) {
                cJSON *array = cJSON_CreateArray();
                cJSON_AddItemToArray(array, cJSON_CreateString("hello"));
                cJSON_AddItemToArray(array, cJSON_CreateNumber(1));
                cJSON_AddItemToArray(array, cJSON_CreateBool(1));
                cJSON_AddItemToArray(array, cJSON_CreateNumber(3.14));
                cJSON *bin_object_1 = cJSON_CreateObject();
                cJSON_AddBoolToObject(bin_object_1, "_placeholder", true);
                cJSON_AddNumberToObject(bin_object_1, "num", 0);
                cJSON_AddItemToArray(array, bin_object_1);
                cJSON *bin_object_2 = cJSON_CreateObject();
                cJSON_AddBoolToObject(bin_object_2, "_placeholder", true);
                cJSON_AddNumberToObject(bin_object_2, "num", 1);
                cJSON_AddItemToArray(array, bin_object_2);
                esp_socketio_packet_set_json(tx_packet, array);
                ESP_LOGI(TAG, "address of bin_object_1: %p, size: %lu", (void *)bin_object_1, sizeof(cJSON *));
                ESP_LOGI(TAG, "address of bin_object_2: %p, size: %lu", (void *)bin_object_2, sizeof(cJSON *));
                esp_socketio_packet_add_binary_data(tx_packet, (unsigned char *)&bin_object_1, sizeof(cJSON *), true);
                esp_socketio_packet_add_binary_data(tx_packet, (unsigned char *)&bin_object_2, sizeof(cJSON *), true);
                esp_socketio_client_send_data(data->client, tx_packet);
                esp_socketio_packet_reset(tx_packet);
            }
        }
        break;

    case SOCKETIO_EVENT_DATA:
        ESP_LOGI(TAG, "Received data from server:");
        print_socketio_packet(packet);
        break;

    case SOCKETIO_EVENT_ERROR:
        ESP_LOGE(TAG, "Socket.IO error. Need to close.");
        esp_socketio_client_close(data->client, pdMS_TO_TICKS(2000));
        esp_socketio_client_destroy(data->client);

    default:
        break;
    }
}


static void socketio_app_start(void)
{
    esp_socketio_client_config_t socketio_cfg = {};

    socketio_cfg.websocket_config.uri = CONFIG_WEBSOCKET_URI;

    ESP_LOGI(TAG, "Connecting to %s...", socketio_cfg.websocket_config.uri);

    esp_socketio_client_handle_t client = esp_socketio_client_init(&socketio_cfg);
    tx_packet = esp_socketio_client_get_tx_packet(client);

    esp_socketio_register_events(client, SOCKETIO_EVENT_ANY, socketio_event_handler, (void *)client);

    esp_socketio_client_start(client);
    // int count = 3;
    // while (count--)
    while(1) {
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
    esp_socketio_client_close(client, pdMS_TO_TICKS(2000));
    esp_socketio_client_destroy(client);
}

int main(void)
{

    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("socketio_client", ESP_LOG_DEBUG);    
    esp_log_level_set("websocket_client", ESP_LOG_DEBUG);
    esp_log_level_set("transport_ws", ESP_LOG_DEBUG);
    esp_log_level_set("trans_tcp", ESP_LOG_DEBUG);

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());

    socketio_app_start();
    return 0;
}

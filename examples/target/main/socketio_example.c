/*
 * SPDX-FileCopyrightText: 2022-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include <stdio.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "protocol_examples_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"

#include "esp_log.h"
#include "esp_socketio_client.h"
#include "esp_event.h"
// #include <cJSON.h>

#define NO_DATA_TIMEOUT_SEC 50

static const char *TAG = "socketio";
static esp_socketio_packet_handle_t tx_packet = NULL;

static TimerHandle_t shutdown_signal_timer;
static SemaphoreHandle_t shutdown_sema;

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

static void shutdown_signaler(TimerHandle_t xTimer)
{
    ESP_LOGI(TAG, "No data received for %d seconds, signaling shutdown", NO_DATA_TIMEOUT_SEC);
    xSemaphoreGive(shutdown_sema);
}

#if CONFIG_WEBSOCKET_URI_FROM_STDIN
static void get_string(char *line, size_t size)
{
    int count = 0;
    while (count < size) {
        int c = fgetc(stdin);
        if (c == '\n') {
            line[count] = '\0';
            break;
        } else if (c > 0 && c < 127) {
            line[count] = c;
            ++count;
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

#endif /* CONFIG_WEBSOCKET_URI_FROM_STDIN */

static void socketio_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_socketio_event_data_t *data = (esp_socketio_event_data_t *)event_data;
    esp_websocket_event_id_t ws_event_id = data->websocket_event_id;
    esp_websocket_event_data_t *ws_event_data = (esp_websocket_event_data_t *)event_data;
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
        xTimerReset(shutdown_signal_timer, portMAX_DELAY);
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
                ESP_LOGI(TAG, "address of bin_object_1: %p, size: %u", (void *)bin_object_1, sizeof(cJSON *));
                ESP_LOGI(TAG, "address of bin_object_2: %p, size: %u", (void *)bin_object_2, sizeof(cJSON *));
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

    shutdown_signal_timer = xTimerCreate("Websocket shutdown timer", NO_DATA_TIMEOUT_SEC * 1000 / portTICK_PERIOD_MS,
                                         pdFALSE, NULL, shutdown_signaler);
    shutdown_sema = xSemaphoreCreateBinary();

#if CONFIG_SOCKETIO_URI_FROM_STDIN
    char line[128];

    ESP_LOGI(TAG, "Please enter uri of websocket endpoint");
    get_string(line, sizeof(line));

    socketio_cfg.websocket_config.uri = line;
    ESP_LOGI(TAG, "Endpoint uri: %s\n", line);

#else
    socketio_cfg.websocket_config.uri = CONFIG_SOCKETIO_URI;
#endif /* CONFIG_SOCKETIO_URI_FROM_STDIN */

#if CONFIG_WS_OVER_TLS_MUTUAL_AUTH
    /* Configuring client certificates for mutual authentification */
    extern const char cacert_start[] asm("_binary_ca_cert_pem_start"); // CA certificate
    extern const char cert_start[] asm("_binary_client_cert_pem_start"); // Client certificate
    extern const char cert_end[]   asm("_binary_client_cert_pem_end");
    extern const char key_start[] asm("_binary_client_key_pem_start"); // Client private key
    extern const char key_end[]   asm("_binary_client_key_pem_end");

    socketio_cfg.websocket_config.cert_pem = cacert_start;
    socketio_cfg.websocket_config.client_cert = cert_start;
    socketio_cfg.websocket_config.client_cert_len = cert_end - cert_start;
    socketio_cfg.websocket_config.client_key = key_start;
    socketio_cfg.websocket_config.client_key_len = key_end - key_start;
#elif CONFIG_WS_OVER_TLS_SERVER_AUTH
    extern const char cacert_start[] asm("_binary_ca_cert_pem_start"); // CA certificate, modify it if using another server
    socketio_cfg.websocket_config.cert_pem = cacert_start;
#endif

#if CONFIG_WS_OVER_TLS_SKIP_COMMON_NAME_CHECK
    socketio_cfg.websocket_config.skip_cert_common_name_check = true;
#endif

    ESP_LOGI(TAG, "Connecting to %s...", socketio_cfg.websocket_config.uri);

    esp_socketio_client_handle_t client = esp_socketio_client_init(&socketio_cfg);
    tx_packet = esp_socketio_client_get_tx_packet(client);

    esp_socketio_register_events(client, SOCKETIO_EVENT_ANY, socketio_event_handler, (void *)client);

    esp_socketio_client_start(client);
    xTimerStart(shutdown_signal_timer, portMAX_DELAY);

    xSemaphoreTake(shutdown_sema, portMAX_DELAY);
    esp_socketio_client_close(client, portMAX_DELAY);
    ESP_LOGI(TAG, "Websocket Stopped");
    esp_socketio_client_destroy(client);
}

void app_main(void)
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
}

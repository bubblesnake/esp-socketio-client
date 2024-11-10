/*
 * SPDX-FileCopyrightText: 2015-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include "esp_socketio_packet.h"
#include "esp_socketio_internal.h"

static const char *TAG = "socketio_packet";
static const char *default_nsp = "/";
static const char *bin_placeholder = "_placeholder";

typedef struct esp_socketio_binary_data {
    uint8_t *buffer;
    size_t buffer_size;
    int index;
    struct esp_socketio_binary_data *next;
} esp_socketio_binary_data_t;

struct esp_socketio_packet {
    esp_engineio_packet_type_t  eio_type;
    esp_socketio_packet_type_t  sio_type;
    char                        *nsp;   // NULL means default namespace "/"
    int                         event_id;
    cJSON                       *json_payload;
    char                        *socketio_payload;
    int                         data_len;
    esp_socketio_binary_data_t  *binary_data_list;
    int                         binary_data_count;
    esp_socketio_binary_data_t  *current_binary_data;
};

static bool is_eio_packet_type_valid(char type);
static bool is_sio_packet_type_valid(char eio_type, char sio_type);
static bool is_json_valid(cJSON *json);

esp_socketio_packet_handle_t esp_socketio_packet_init()
{
    esp_socketio_packet_handle_t packet = calloc(1, sizeof(struct esp_socketio_packet));
    ESP_SOCKETIO_MEM_CHECK(TAG, packet, return NULL);

    return packet;
}

esp_err_t esp_socketio_packet_set_header(esp_socketio_packet_handle_t packet, esp_engineio_packet_type_t eio_type, esp_socketio_packet_type_t sio_type, char *nsp, int event_id)
{
    if (packet == NULL || !is_sio_packet_type_valid(eio_type, sio_type)) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_socketio_packet_reset(packet);

    if (nsp != NULL) {
        packet->nsp = calloc(1, strlen(nsp) + 1);
        ESP_SOCKETIO_MEM_CHECK(TAG, packet->nsp, {
            return ESP_ERR_NO_MEM;
        });
        strcpy(packet->nsp, nsp);
    }

    packet->eio_type = eio_type;
    packet->sio_type = sio_type;
    packet->event_id = event_id;
    return ESP_OK;
}

esp_err_t esp_socketio_packet_reset(esp_socketio_packet_handle_t packet)
{
    if (packet->nsp) {
        free(packet->nsp);
    }

    esp_socketio_packet_destroy_binary_data(packet);

    if (packet->socketio_payload) {
        free(packet->socketio_payload);
    }

    cJSON_Delete(packet->json_payload);

    memset(packet, 0, sizeof(struct esp_socketio_packet));
    packet->event_id = -1;
    return ESP_OK;
}

void esp_socketio_packet_destroy(esp_socketio_packet_handle_t packet)
{
    esp_socketio_packet_reset(packet);
    free(packet);
    return;
}

esp_engineio_packet_type_t esp_socketio_packet_get_eio_type(esp_socketio_packet_handle_t packet)
{
    if (packet == NULL) {
        return EIO_PACKET_TYPE_UNKNOWN;
    }
    return packet->eio_type;
}

esp_socketio_packet_type_t esp_socketio_packet_get_sio_type(esp_socketio_packet_handle_t packet)
{
    if (packet == NULL) {
        return SIO_PACKET_TYPE_UNKNOWN;
    }
    return packet->sio_type;
}

char *esp_socketio_packet_get_nsp(esp_socketio_packet_handle_t packet)
{
    if (packet == NULL) {
        return NULL;
    }
    if (packet->nsp == NULL) {
        return (char *)default_nsp;
    }
    return packet->nsp;
}

cJSON *esp_socketio_packet_get_json(esp_socketio_packet_handle_t packet)
{
    if (packet == NULL) {
        return NULL;
    }
    return packet->json_payload;
}

char *esp_socketio_packet_get_raw_data(esp_socketio_packet_handle_t packet, int *data_len_ptr)
{
    if (packet == NULL || data_len_ptr == NULL) {
        return NULL;
    }
    *data_len_ptr = packet->data_len;
    return packet->socketio_payload;
}

int esp_socketio_packet_get_event_id(esp_socketio_packet_handle_t packet)
{
    if (packet == NULL) {
        return -1;
    }
    return packet->event_id;
}

esp_err_t esp_socketio_packet_set_json(esp_socketio_packet_handle_t packet, const cJSON *json)
{
    if (packet == NULL || json == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    packet->json_payload = cJSON_Duplicate(json, true);
    return ESP_OK;
}

int esp_socketio_packet_add_binary_data(esp_socketio_packet_handle_t packet, const unsigned char *data, size_t data_size, bool increment)
{
    if (packet == NULL || data == NULL) {
        return -1;
    }
    esp_socketio_binary_data_t *new_binary = (esp_socketio_binary_data_t *)malloc(sizeof(esp_socketio_binary_data_t));
    ESP_SOCKETIO_MEM_CHECK(TAG, new_binary, return -1);

    new_binary->buffer = (unsigned char *)malloc(data_size);
    ESP_SOCKETIO_MEM_CHECK(TAG, new_binary->buffer, {
        free(new_binary);
        return -1;
    });

    memcpy(new_binary->buffer, data, data_size);
    new_binary->buffer_size = data_size;
    new_binary->index = (packet->binary_data_list == NULL) ? 0 : packet->binary_data_list->index + 1;
    new_binary->next = NULL;
    if (packet->binary_data_list == NULL) {
        packet->binary_data_list = new_binary;
        packet->current_binary_data = new_binary;
    } else {
        esp_socketio_binary_data_t *current = packet->binary_data_list;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = new_binary;
    }

    if (increment) {
        packet->binary_data_count++;
    }
    return new_binary->index;
}

void esp_socketio_packet_destroy_binary_data(esp_socketio_packet_handle_t packet)
{
    if (packet == NULL || packet->binary_data_list == NULL) {
        return;
    }

    esp_socketio_binary_data_t *current = packet->binary_data_list;
    esp_socketio_binary_data_t *next = NULL;
    while (current != NULL) {
        next = current->next;
        free(current->buffer);
        free(current);
        current = next;
    }
    packet->binary_data_list = NULL;
    packet->current_binary_data = NULL;
}

int esp_socketio_packet_count_binary_data(esp_socketio_packet_handle_t packet)
{
    if (packet == NULL) {
        return -1;
    }

    return packet->binary_data_count;
}

int esp_socketio_packet_get_last_binary_index(esp_socketio_packet_handle_t packet)
{
    if (packet == NULL) {
        return -1;
    }
    esp_socketio_binary_data_t *current = packet->binary_data_list;

    if (current == NULL) {
        return -1;
    }
    while (current->next != NULL) {
        current = current->next;
    }
    return current->index;
}

esp_err_t esp_socketio_packet_get_current_binary_data(esp_socketio_packet_handle_t packet, unsigned char **data_ptr, size_t *data_size_ptr, int *index_ptr)
{
    if (packet == NULL || data_ptr == NULL || data_size_ptr == NULL || index_ptr == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (packet->current_binary_data == NULL) {
        ESP_LOGI(TAG, "No more binary data.");
        *data_ptr = NULL;
        *data_size_ptr = 0;
        *index_ptr = -1;
    } else {
        *data_ptr = packet->current_binary_data->buffer;
        *data_size_ptr = packet->current_binary_data->buffer_size;
        *index_ptr = packet->current_binary_data->index;
        packet->current_binary_data = packet->current_binary_data->next;
    }
    return ESP_OK;
}

esp_err_t esp_socketio_parse_open_packet(const char *buf, int len, char *sid_ptr, int *ping_interval_ptr, int *ping_timeout_ptr, int *max_payload_ptr)
{
    if (buf == NULL || sid_ptr == NULL || ping_interval_ptr == NULL || ping_timeout_ptr == NULL || max_payload_ptr == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    // parse the JSON data
    cJSON *json = cJSON_Parse(buf);
    if (!is_json_valid(json)) {
        return ESP_FAIL;
    }

    // access the JSON data
    cJSON *cjson_sid = cJSON_GetObjectItem(json, "sid");
    if (cJSON_IsString(cjson_sid) && (cjson_sid->valuestring != NULL)) {
        memcpy(sid_ptr, cjson_sid->valuestring,
            (ESP_SOCKETIO_CLIENT_SID_LEN < strlen(cjson_sid->valuestring))
                ? ESP_SOCKETIO_CLIENT_SID_LEN : strlen(cjson_sid->valuestring));
    } else {
        return ESP_ERR_NOT_FOUND;
    }

    cJSON *cjson_pingInterval = cJSON_GetObjectItem(json, "pingInterval");
    if (cJSON_IsNumber(cjson_pingInterval)) {
        *ping_interval_ptr = cjson_pingInterval->valueint;
    } else {
        return ESP_ERR_NOT_FOUND;
    }

    cJSON *cjson_pingTimeout = cJSON_GetObjectItem(json, "pingTimeout");
    if (cJSON_IsNumber(cjson_pingTimeout)) {
        *ping_timeout_ptr = cjson_pingTimeout->valueint;
    } else {
        return ESP_ERR_NOT_FOUND;
    }

    cJSON *cjson_maxPayload = cJSON_GetObjectItem(json, "maxPayload");
    if (cJSON_IsNumber(cjson_maxPayload)) {
        *max_payload_ptr = cjson_maxPayload->valueint;
    } else {
        return ESP_ERR_NOT_FOUND;
    }

    // delete the JSON object
    cJSON_Delete(json);

    ESP_LOGI(TAG, "Received Engine.IO OPEN packet: sid = %s, pingInterval = %d, pingTimeout = %d, maxPayload = %d.\n",
        sid_ptr,
        *ping_interval_ptr,
        *ping_timeout_ptr,
        *max_payload_ptr);

    return ESP_OK;
}

esp_err_t esp_socketio_packet_parse_message(esp_socketio_packet_handle_t packet, const char *buf, int len)
{
    if (packet == NULL || buf == NULL || !is_sio_packet_type_valid(buf[0], buf[1])) {
        return ESP_ERR_INVALID_ARG;
    }

    if (buf[0] != EIO_PACKET_TYPE_MESSAGE) {
        ESP_LOGW(TAG, "Not an Engine.IO MESSAGE packet.");
        return ESP_ERR_INVALID_ARG;
    }

    esp_socketio_packet_reset(packet);
    packet->eio_type = (esp_engineio_packet_type_t)buf[0];
    packet->sio_type = (esp_socketio_packet_type_t)buf[1];
    char *nsp = NULL;
    cJSON *json = NULL;
    char *slash_pos = strchr(buf, '/');
    char *open_square_pos = strchr(buf, '[');
    char *comma_pos = strchr(buf, ',');
    if (slash_pos != NULL) {
        if (comma_pos != NULL && (open_square_pos == NULL || comma_pos < open_square_pos)) {
            // A custom namespace has been found
            int nsp_len = comma_pos - slash_pos;
            nsp = calloc(1, nsp_len + 1);
            ESP_SOCKETIO_MEM_CHECK(TAG, nsp, {
                return ESP_ERR_NO_MEM;
            });
            memcpy(nsp, slash_pos, nsp_len);
            ESP_LOGI(TAG, "Custom namespace: \"%s\"", nsp);
            packet->nsp = nsp;
        }
    }

    switch (packet->sio_type)
    {
    case SIO_PACKET_TYPE_CONNECT:
    case SIO_PACKET_TYPE_CONNECT_ERROR:
        char *open_brace_pos = strchr(buf, '{');
        json = cJSON_Parse(open_brace_pos);
        break;

    case SIO_PACKET_TYPE_EVENT:
    case SIO_PACKET_TYPE_ACK:
    case SIO_PACKET_TYPE_BINARY_EVENT:
    case SIO_PACKET_TYPE_BINARY_ACK:
        char *end_ptr;
        json = cJSON_Parse(open_square_pos);
        if (packet->sio_type == SIO_PACKET_TYPE_BINARY_EVENT || packet->sio_type == SIO_PACKET_TYPE_BINARY_ACK) {
            long bin_num = strtol(&buf[2], &end_ptr, 10);
            if (end_ptr == &buf[2] || end_ptr[0] != '-') {
                ESP_LOGE(TAG, "Error parsing binary count.");
                esp_socketio_packet_reset(packet);
                return ESP_ERR_NOT_FOUND;
            }

            int placeholder_count = 0;
            char *pos = (char *)buf;
            while ((pos = strstr(pos, bin_placeholder)) != NULL) {
                placeholder_count++;
                pos += strlen(bin_placeholder);
            }

            if (placeholder_count != bin_num) {
                ESP_LOGE(TAG, "Binary data count doesn't match with number of _placeholder");
                esp_socketio_packet_reset(packet);
                return ESP_ERR_NOT_FOUND;
            }

            packet->binary_data_count = bin_num;
        }

        char *event_id_pos = NULL;
        if (packet->nsp != NULL) {
            event_id_pos = comma_pos + 1;
        } else if (packet->binary_data_count > 0) {
            event_id_pos = strchr(buf, '-') + 1;
        } else {
            event_id_pos = (char *)&buf[2];
        }

        if (event_id_pos != open_square_pos) {
            ESP_LOGI(TAG, "Parsing event ID.");
            long event_id = strtol(event_id_pos, &end_ptr, 10);
            if (end_ptr == event_id_pos || end_ptr[0] != '[') {
                ESP_LOGE(TAG, "Error parsing event ID.");
                esp_socketio_packet_reset(packet);
                return ESP_ERR_NOT_FOUND;
            }
            packet->event_id = event_id;
        }
        break;
    
    default:
        break;
    }

    if (!is_json_valid(json)) {
        ESP_LOGE(TAG, "Invalid Socket.IO json message received.");
        esp_socketio_packet_reset(packet);
        return ESP_ERR_NOT_FOUND;
    }
    packet->json_payload = json;
    return ESP_OK;
}

esp_err_t esp_socketio_packet_encode_message(esp_socketio_packet_handle_t packet)
{
    char *json_string = NULL;
    if (packet == NULL || packet->eio_type != EIO_PACKET_TYPE_MESSAGE || !is_sio_packet_type_valid(packet->eio_type, packet->sio_type)) {
        return ESP_ERR_INVALID_ARG;
    }
    packet->data_len = 2; // eio_type + sio_type

    int binary_count = esp_socketio_packet_count_binary_data(packet);
    if (binary_count > 0) {
        packet->data_len += snprintf(NULL, 0, "%d-", binary_count);
    }

    if (packet->nsp != NULL) {
        packet->data_len += strlen(packet->nsp) + 1; // Namespace length + delimiter ","
    }

    if (packet->event_id >= 0) {
        packet->data_len += snprintf(NULL, 0, "%d", packet->event_id);
    }

    json_string = cJSON_PrintUnformatted(packet->json_payload);
    if (json_string != NULL) {
        packet->data_len += strlen(json_string);
    }

    if (packet->socketio_payload != NULL) {
        free(packet->socketio_payload);
    }
    packet->socketio_payload = malloc(packet->data_len + 1);
    ESP_SOCKETIO_MEM_CHECK(TAG, packet->socketio_payload, {
        if (json_string != NULL) {
            free(json_string);
        }
        return ESP_ERR_NO_MEM;
    })

    char *ptr = packet->socketio_payload;
    ptr += sprintf(ptr, "%c%c", packet->eio_type, packet->sio_type);
    if (binary_count > 0) {
        ptr += sprintf(ptr, "%d-", binary_count);
    }

    if (packet->nsp != NULL) {
        ptr += sprintf(ptr, "%s,", packet->nsp);
    }

    if (packet->event_id >= 0) {
        ptr += sprintf(ptr, "%d", packet->event_id);
    }

    if (json_string != NULL) {
        sprintf(ptr, "%s", json_string);
        free(json_string);
    }
    return ESP_OK;
}

bool is_eio_packet_type_valid(char type)
{
    if (type >= EIO_PACKET_TYPE_OPEN || type <= EIO_PACKET_TYPE_NOOP) {
        return true;
    }
    ESP_LOGE(TAG, "Invalid eio type.");
    return false;
}

bool is_sio_packet_type_valid(char eio_type, char sio_type)
{
    if (is_eio_packet_type_valid(eio_type)) {
        if (eio_type == EIO_PACKET_TYPE_MESSAGE) {
            if (sio_type >= SIO_PACKET_TYPE_CONNECT || sio_type <= SIO_PACKET_TYPE_BINARY_ACK) {
                return true;
            }
            ESP_LOGE(TAG, "Invalid sio type.");
            return false;
        }
        return true;
    }
    return false;
}

bool is_json_valid(cJSON *json)
{
    if (json == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            ESP_LOGE(TAG, "Error: %s\n", error_ptr);
        }
        return false;
    }
    return true;
}
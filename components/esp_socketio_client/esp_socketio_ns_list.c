/*
 * SPDX-FileCopyrightText: 2015-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include "esp_socketio_ns_list.h"
#include "esp_socketio_internal.h"

static const char *TAG = "socketio_ns_list";

typedef struct esp_socketio_ns {
    char *nsp;
    char *sid;
    struct esp_socketio_ns *next;
} esp_socketio_ns_t;

struct esp_socketio_ns_list {
    esp_socketio_ns_t   *namespaces;
    int                 num_namespaces;
};

esp_socketio_ns_list_handle_t esp_socketio_ns_list_create()
{
    esp_socketio_ns_list_handle_t ns_list = calloc(1, sizeof(struct esp_socketio_ns_list));
    ESP_SOCKETIO_MEM_CHECK(TAG, ns_list, return NULL);
    return ns_list;
}

esp_err_t esp_socketio_ns_list_add_ns(esp_socketio_ns_list_handle_t ns_list, char *nsp, char *sid)
{
    ESP_SOCKETIO_MEM_CHECK(TAG, (ns_list != NULL && sid != NULL), return ESP_ERR_INVALID_ARG);

    esp_socketio_ns_t *new_namespace = calloc(1, sizeof(esp_socketio_ns_t));
    // NULL represents default namespace
    if (nsp != NULL) {
        new_namespace->nsp = strdup(nsp);
        ESP_SOCKETIO_MEM_CHECK(TAG, new_namespace->nsp, return ESP_ERR_NO_MEM);
    }
    new_namespace->sid = strdup(sid);
    ESP_SOCKETIO_MEM_CHECK(TAG, new_namespace->sid, return ESP_ERR_NO_MEM);
    new_namespace->next = NULL;

    if (ns_list->namespaces == NULL) {
        ns_list->namespaces = new_namespace;
    } else {
        esp_socketio_ns_t *current = ns_list->namespaces;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = new_namespace;
    }
    ns_list->num_namespaces++;
    return ESP_OK;
}

char *esp_socketio_ns_list_search_sid(esp_socketio_ns_list_handle_t ns_list, char *nsp)
{
    ESP_SOCKETIO_MEM_CHECK(TAG, ns_list, return NULL);

    esp_socketio_ns_t *current = ns_list->namespaces;
    while (current != NULL) {
        if ((nsp == NULL && current->nsp == NULL) ||
            (nsp != NULL && current->nsp != NULL && strcmp(current->nsp, nsp) == 0)) {
            return current->sid;
        }
        current = current->next;
    }
    return NULL;
}

bool esp_socketio_ns_list_is_nsp_exist(esp_socketio_ns_list_handle_t ns_list, const char *nsp)
{
    ESP_SOCKETIO_MEM_CHECK(TAG, ns_list, return false);

    esp_socketio_ns_t *current = ns_list->namespaces;
    while (current != NULL) {
        if ((nsp == NULL && current->nsp == NULL) ||
            (nsp != NULL && current->nsp != NULL && strcmp(current->nsp, nsp) == 0)) {
            return true;
        }
        current = current->next;
    }
    return false;
}

int esp_socketio_ns_list_get_num(esp_socketio_ns_list_handle_t ns_list)
{
    ESP_SOCKETIO_MEM_CHECK(TAG, ns_list, return -1);
    return ns_list->num_namespaces;
}

esp_err_t esp_socketio_ns_list_delete_ns(esp_socketio_ns_list_handle_t ns_list, char *nsp)
{
    esp_socketio_ns_t *current = ns_list->namespaces;
    esp_socketio_ns_t *prev = NULL;

    while (current != NULL) {
        if ((nsp == NULL && current->nsp == NULL) ||
            (nsp != NULL && current->nsp != NULL && strcmp(current->nsp, nsp) == 0)) {
            if (prev == NULL) {
                ns_list->namespaces = current->next;
            } else {
                prev->next = current->next;
            }
            if (current->nsp != NULL) {
                free(current->nsp);
            }
            if (current->sid != NULL) {
                free(current->sid);
            }
            free(current);
            ns_list->num_namespaces--;
            return ESP_OK;
        }
        prev = current;
        current = current->next;
    }
    return ESP_ERR_NOT_FOUND;
}

void esp_socketio_ns_list_destroy(esp_socketio_ns_list_handle_t ns_list)
{
    if (ns_list == NULL) {
        return;
    }
    esp_socketio_ns_t *current = ns_list->namespaces;
    esp_socketio_ns_t *temp = NULL;
    while (current != NULL) {
        temp = current;
        current = current->next;
        if (temp->nsp != NULL) {
            free(temp->nsp);
        }
        if (temp->sid != NULL) {
            free(temp->sid);
        }
        free(temp);
    }
    free(ns_list);
    return;
}

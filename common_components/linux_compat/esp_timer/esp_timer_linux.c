/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "esp_err.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <pthread.h>

void *create_tt(esp_timer_cb_t cb, void *arg);

void destroy_tt(void *tt);

void set_tout(void *tt, uint32_t ms, bool periodic);

void cancel_tout(void *tt);

esp_err_t esp_timer_create(const esp_timer_create_args_t *create_args,
                           esp_timer_handle_t *out_handle)
{
    *out_handle = (esp_timer_handle_t)create_tt(create_args->callback, create_args->arg);
    return ESP_OK;
}

esp_err_t esp_timer_start_periodic(esp_timer_handle_t timer, uint64_t period)
{
    set_tout(timer, period / 1000, true);
    return ESP_OK;
}

esp_err_t esp_timer_start_once(esp_timer_handle_t timer, uint64_t period)
{
    set_tout(timer, period / 1000, false);
    return ESP_OK;
}

esp_err_t esp_timer_stop(esp_timer_handle_t timer)
{
    cancel_tout(timer);
    return ESP_OK;
}

esp_err_t esp_timer_delete(esp_timer_handle_t timer)
{
    destroy_tt(timer);
    return ESP_OK;
}

int64_t esp_timer_get_time(void)
{
    struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
    return spec.tv_nsec / 1000 + spec.tv_sec * 1000000;
}

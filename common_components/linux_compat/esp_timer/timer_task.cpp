/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "timer_task.hpp"
#include <cstdint>
#include <vector>
#include <memory>
#include <cstring>

extern "C" void *create_tt(cb_t cb, void *arg)
{
    auto *tt = new TimerTaskMock(cb, arg);
    return tt;
}

extern "C" void destroy_tt(void *tt)
{
    auto *timer_task = static_cast<TimerTaskMock *>(tt);
    delete (timer_task);
}


extern "C" void set_tout(void *tt, uint32_t ms, bool periodic)
{
    auto *timer_task = static_cast<TimerTaskMock *>(tt);
    timer_task->SetTimeout(ms, periodic);
}

extern "C" void cancel_tout(void *tt)
{
    auto *timer_task = static_cast<TimerTaskMock *>(tt);
    timer_task->CancelTimeout();
}
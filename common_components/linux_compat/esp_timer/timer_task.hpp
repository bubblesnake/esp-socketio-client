/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <thread>
#include <atomic>

typedef void (*cb_t)(void *arg);

class TimerTaskMock {
public:
    TimerTaskMock(cb_t cb, void *arg): cb(cb), arg(arg), active(false), periodic(false), ms(INT32_MAX) {}
    ~TimerTaskMock(void)
    {
        active = false;
        t.join();
    }

    void SetTimeout(uint32_t m, bool p)
    {
        ms = m;
        active = true;
        periodic = p;
        t = std::thread(run_static, this);
    }

    void CancelTimeout(void)
    {
        active = false;
        t.join();
    }

private:

    static void run_static(TimerTaskMock *timer)
    {
        timer->run();
    }

    void run(void)
    {
        while (!active.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        while (active.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            if (!--ms) {
                cb(arg);
                if (!periodic.load()) {
                    break;
                }
            }
        }
    }

    cb_t cb;
    void *arg;
    std::thread t;
    std::atomic<bool> active;
    std::atomic<bool> periodic;
    uint32_t ms;

};

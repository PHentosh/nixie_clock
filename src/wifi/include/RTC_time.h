#ifndef EXPERIMENTS_RTC_TIME_H
#define EXPERIMENTS_RTC_TIME_H

#include "osal.h"
#include "esp_sntp.h"

enum timer_event_t {
    TIMER_SYNC,

    TIMER_EVENT_SIZE
};

struct timer_msg_t {
    timer_event_t event;

    union {
        uint8_t nothing;
    } u;
};

/**
 * @brief timer callback function
 *
 * Will be called on timer event
 *
 * @param [in] event event that happened
 */
typedef void(*timer_cb_t)(timer_event_t event);

/**
 * @brief init timer tasks
 *
 * @param [in] timer_init Rx task options
 */
void timer_init(OSAL::Task::init_t* timer_init);

/**
 * @brief deinit timer tasks
 */
bool timer_deinit();

/**
 * @brief register callback for events
 *
 * @param [in] func     callback function
 * @param [in] on_event event to call func
 *
 * @retval true  if callback registered successfully
 * @retval false if callback didn't register
 */
bool timer_register_cb(timer_event_t on_event, timer_cb_t func);

/**
 * @brief timer callback
 *
 * Used to pass event to board
 *
 * @param [in] msg
 */
void timer_cb(timer_msg_t* msg);

#endif //EXPERIMENTS_RTC_TIME_H

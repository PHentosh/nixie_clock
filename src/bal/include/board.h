#ifndef EXPERIMENTS_BOARD_H
#define EXPERIMENTS_BOARD_H

#include "osal.h"
#include "esp_sntp.h"

enum board_event_t {
    BTN1_SINGLE_CLICK,
    BTN1_DOUBLE_CLICK,

    BTN2_SINGLE_CLICK,
    BTN2_DOUBLE_CLICK,

    BTN3_SINGLE_CLICK,
    BTN3_DOUBLE_CLICK,

    DIAL_SET_TIME,

    LAMP1_SET_VALUE,
    LAMP2_SET_VALUE,
    LAMP3_SET_VALUE,
    LAMP4_SET_VALUE,

    BUZZER_PLAY,
    BUZZER_STOP,

    BOARD_EVENT_SIZE
};

struct board_msg_t {
    board_event_t event;

    union {
        uint8_t value;
        tm      timeinfo;
    } u;
};

/**
 * @brief board callback function
 *
 * Will be called on board event
 *
 * @param [in] event event that happened
 */
typedef void(*board_cb_t)(board_event_t event);

/**
 * @brief init board tasks
 *
 * @param [in] rx_init Rx task options
 * @param [in] tx_init Tx task options
 */
void board_init(const OSAL::Task::init_t& rx_init, const OSAL::Task::init_t& tx_init);

/**
 * @brief deinit board tasks
 */
bool board_deinit();

/**
 * @brief register callback for events
 *
 * @param [in] func     callback function
 * @param [in] on_event event to call func
 *
 * @retval true  if callback registered successfully
 * @retval false if callback didn't register
 */
bool board_register_cb(board_event_t on_event, board_cb_t func);

/**
 * @brief board callback
 *
 * Used to pass event to board
 *
 * @param [in] msg
 */
void board_cb(board_msg_t* msg);


#endif //EXPERIMENTS_BOARD_H

#include "esp_wifi.h"

#include "osal.h"
#include "board.h"
#include "RTC_time.h"

#include <array>

enum tsk_e
{
    TSK_BOARD_RX,
    TSK_BOARD_TX,
    TSK_TIMER,

    TSK_ENUM_SIZE
};

extern "C" {
void app_start(void);
}

static int counter = 0;
std::array<uint32_t, 7> notes = {262, 294, 330, 350, 392, 440, 494};

const OSAL::Task::init_t tasks[TSK_ENUM_SIZE] = {
        [TSK_BOARD_RX] = { nullptr, 4096, "board_rx", 1 },
        [TSK_BOARD_TX] = { nullptr, 4096, "board_tx", 1 },
        [TSK_TIMER]    = { nullptr, 4096, "timer", 2 },
};

void timer_cb(tm& timeinfo)
{
    board_msg_t msg
    {
        .event = BOARD_DIAL_SET_TIME,
        .u = {
                .timeinfo = timeinfo,
        }
    };

    board_cb(&msg);
}

void start_buzzer()
{
    board_msg_t msg
    {
        .event = BOARD_BUZZER_PLAY,
        .u = {}
    };

    board_cb(&msg);
}

void stop_buzzer()
{
    board_msg_t msg
    {
        .event = BOARD_BUZZER_STOP,
        .u = {}
    };

    board_cb(&msg);
}

void buzzer_set_fr()
{
    board_msg_t msg
    {
        .event = BOARD_BUZZER_SET_FREQUENCY,
        .u = { .u32 = notes[counter/7]}
    };

    counter++;

    board_cb(&msg);
}

void app_start() {
    ESP_ERROR_CHECK(esp_netif_init());

    board_init(tasks[TSK_BOARD_RX], tasks[TSK_BOARD_RX]);
//    timer_init(tasks[TSK_TIMER]);

//    timer_register_cb(TIMER_SET_TIME, timer_cb);

    board_register_cb(BOARD_BTN1_SINGLE_CLICK, start_buzzer);
    board_register_cb(BOARD_BTN1_DOUBLE_CLICK, stop_buzzer);
    board_register_cb(BOARD_BTN2_SINGLE_CLICK, buzzer_set_fr);
}

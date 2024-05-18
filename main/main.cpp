#include "osal.h"
#include "board.h"
#include "RTC_time.h"

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

const OSAL::Task::init_t tasks[TSK_ENUM_SIZE] = {
        [TSK_BOARD_RX] = { nullptr, 2048, "board_rx", 1 },
        [TSK_BOARD_TX] = { nullptr, 2048, "board_tx", 1 },
        [TSK_TIMER]    = { nullptr, 4096, "timer", 2 },
};

void app_start() {
    board_init(tasks[TSK_BOARD_RX], tasks[TSK_BOARD_RX]);
    timer_init(tasks[TSK_TIMER]);
}

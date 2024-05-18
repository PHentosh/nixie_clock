#include <cstdio>
#include <cstring>
#include "esp_log.h"
#include "sdkconfig.h"

#include "osal.h"
#include "board.h"

enum tsk_e
{
    TSK_BOARD_RX,
    TSK_BOARD_TX,
    TSK_SNTP,

    TSK_ENUM_SIZE
};

extern "C" {
void app_start(void);
}

const OSAL::Task::init_t tasks[TSK_ENUM_SIZE] = {
        [TSK_BOARD_RX] = { nullptr, 2048, "heartbeat.d", 1 },
        [TSK_BOARD_TX] = { nullptr, 2048, "heartbeat.d", 1 },
};

void app_start() {
    board_init(tasks[TSK_BOARD_RX], tasks[TSK_BOARD_RX]);

}

#include <array>
#include <vector>

#include "esp_log.h"
#include "nvs_flash.h"

#include "board.h"
#include "mcp23017.h"
#include "dial.h"
#include "buttons.h"

#define I2C_SDA_IO 14
#define I2C_SCL_IO 15

#define MAX_CALLBACKS 5

static const char *TAG = "BOARD";

static std::array<std::pair<board_event_t, board_cb_t>, MAX_CALLBACKS> callbacks;
static size_t callback_num = 0;

static class BoardRx* _task_rx = nullptr;

class BoardRx final : public OSAL::Task
{
public:
    OSAL::Queue<board_msg_t, 10> m_queue {nullptr};

private:
    mcp23017_t mcp_cfg;
    Dial       dial;

public:
    explicit BoardRx() noexcept : OSAL::Task{} {}

private:
    void setup() noexcept final;
    void run() noexcept final;
    void teardown() noexcept final;
};

class BoardTx final : public OSAL::Task
{
private:
    std::vector<Button> buttons;

public:
    explicit BoardTx() noexcept : OSAL::Task{} {}

private:
    void setup() noexcept final;
    void run() noexcept final;
    void teardown() noexcept final;
};

static bool init_mcp23017(mcp23017_t* mcp_cfg)
{
    mcp_cfg->i2c_addr = 0x20;
    mcp_cfg->port = I2C_NUM_1;
    mcp_cfg->sda_pin = I2C_SDA_IO;
    mcp_cfg->scl_pin = I2C_SCL_IO;
    mcp_cfg->sda_pullup_en = GPIO_PULLUP_ENABLE;
    mcp_cfg->scl_pullup_en = GPIO_PULLUP_ENABLE;

    if (ESP_OK != mcp23017_init(mcp_cfg))
    {
        ESP_LOGE(TAG, "Could not initialise mcp23017!");
        return false;
    }

    bool ret;
    ret =    not mcp23017_write_register(mcp_cfg, MCP23017_IODIR, GPIOA, 0x0);
    ret = ret || mcp23017_write_register(mcp_cfg, MCP23017_IODIR, GPIOB, 0x0);
    ret = ret || mcp23017_write_register(mcp_cfg, MCP23017_GPPU, GPIOA, 0x0);
    ret = ret || mcp23017_write_register(mcp_cfg, MCP23017_GPPU, GPIOB, 0x0);

    return ret;
}

void BoardRx::setup() noexcept
{
    if (not init_mcp23017(&mcp_cfg))
    {
        ESP_LOGE(TAG, "Error initializing i2c");
    }

    dial.add_lamp(&mcp_cfg, 0xF0, GPIOA);
    dial.add_lamp(&mcp_cfg, 0x0F, GPIOB);
    dial.add_lamp(&mcp_cfg, 0xF0, GPIOB);
    dial.add_lamp(&mcp_cfg, 0x0F, GPIOA);
}

void BoardRx::run() noexcept
{
    while (1) {
        board_msg_t msg;
        if (m_queue.receive(&msg, 0))
        {
            switch (msg.event) {

                case BOARD_DIAL_SET_TIME:
                {
                    dial.set_time(msg.u.timeinfo);
                    break;
                }
                case BOARD_LAMP1_SET_VALUE:
                {
                    dial.set_lamp_value(0, msg.u.value);
                    break;
                }
                case BOARD_LAMP2_SET_VALUE:
                {
                    dial.set_lamp_value(1, msg.u.value);
                    break;
                }
                case BOARD_LAMP3_SET_VALUE:
                {
                    dial.set_lamp_value(2, msg.u.value);
                    break;
                }
                case BOARD_LAMP4_SET_VALUE:
                {
                    dial.set_lamp_value(3, msg.u.value);
                    break;
                }
                case BOARD_BUZZER_PLAY:
                {
                    ESP_LOGW(TAG, "Buzzer play mock");
                    break;
                }
                case BOARD_BUZZER_STOP:
                {
                    ESP_LOGW(TAG, "Buzzer stop mock");
                    break;
                }
                case BOARD_BTN1_SINGLE_CLICK:
                case BOARD_BTN1_DOUBLE_CLICK:
                case BOARD_BTN2_SINGLE_CLICK:
                case BOARD_BTN2_DOUBLE_CLICK:
                case BOARD_BTN3_SINGLE_CLICK:
                case BOARD_BTN3_DOUBLE_CLICK:
                case BOARD_EVENT_SIZE:
                    break;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void BoardRx::teardown() noexcept
{
    m_queue.~Queue();
    dial.~Dial();
}

void BoardTx::setup() noexcept
{
    nvs_flash_init();

    buttons.emplace_back(GPIO_NUM_12);
    buttons.emplace_back(GPIO_NUM_13);
    buttons.emplace_back(GPIO_NUM_14);
}

void BoardTx::run() noexcept
{
    while (1) {
//        if()
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void BoardTx::teardown() noexcept
{

}

void board_init(const OSAL::Task::init_t& rx_init, const OSAL::Task::init_t& tx_init)
{
    static std::aligned_storage_t<sizeof(BoardRx), alignof(BoardRx)> _task_rx_storage;

    assert(not _task_rx);
    _task_rx = new(&_task_rx_storage) BoardRx{};
    bool ret = _task_rx->start(rx_init);
    assert(ret);

    static BoardTx tx{};
    ret = tx.start(tx_init);
    assert(ret);

}

bool board_deinit()
{
    if(not _task_rx )
        return true;

    _task_rx->~BoardRx();
    return true;
}

bool board_register_cb(board_event_t on_event, board_cb_t func)
{
    if (callback_num >= MAX_CALLBACKS)
        return false;

    callbacks[callback_num] = {on_event, func};
    callback_num++;
    return true;
}

void board_cb(board_msg_t* msg)
{
    if (not _task_rx){
        ESP_LOGE(TAG, "Trying to send message before task inited");
        assert(false);
    }

    if (not _task_rx->m_queue.send(msg, 0)){
        ESP_LOGE(TAG, "Event queue full or an internal error occurred");
        assert(false);
    }
}

#include <array>
#include <vector>

#include "esp_log.h"
#include "nvs_flash.h"

#include "board.h"
#include "mcp23017.h"
#include "dial.h"
#include "buttons.h"
#include "buzzer.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#define I2C_SDA_IO 14
#define I2C_SCL_IO 15

#define MAX_CALLBACKS 5

#define LONG_PRESS_MS 2000
#define SINGLE_PRESS_MS 100

static const char *TAG = "BOARD";

static std::array<std::pair<board_event_t, board_cb_t>, MAX_CALLBACKS> callbacks;
static size_t callback_num = 0;

static class BoardRx* _task_rx = nullptr;

struct button_state_msg_t {
    button_state_t state;
    int button_num;
};
static OSAL::Queue<button_state_msg_t, 10> _tx_queue {nullptr};
//static QueueHandle_t _tx_queue = xQueueCreate(10, sizeof(button_state_msg_t));

class BoardRx final : public OSAL::Task
{
public:
    OSAL::Queue<board_msg_t, 10> m_queue {nullptr};

private:
    mcp23017_t mcp_cfg;
    Dial       dial;
    Buzzer*    buzzer;

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

    struct _button_isr_ctx_t {
        int button_num;
    };

    static void _buttons_isr_cb(button_state_t state, void* ctx);

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

    buzzer = new Buzzer(GPIO_NUM_18);

}

void BoardRx::run() noexcept
{
    while (1) {
        board_msg_t msg;
        if (m_queue.receive(&msg, 0))
        {
            ESP_LOGI(TAG, "Read event %d", msg.event);
            switch (msg.event) {

                case BOARD_DIAL_SET_TIME:
                {
                    dial.set_time(msg.u.timeinfo);
                    break;
                }
                case BOARD_LAMP1_SET_VALUE:
                {
                    dial.set_lamp_value(0, msg.u.u8);
                    break;
                }
                case BOARD_LAMP2_SET_VALUE:
                {
                    dial.set_lamp_value(1, msg.u.u8);
                    break;
                }
                case BOARD_LAMP3_SET_VALUE:
                {
                    dial.set_lamp_value(2, msg.u.u8);
                    break;
                }
                case BOARD_LAMP4_SET_VALUE:
                {
                    dial.set_lamp_value(3, msg.u.u8);
                    break;
                }
                case BOARD_BUZZER_PLAY:
                {
                    ESP_LOGI(TAG, "Buzzer play");
                    buzzer->start();
                    break;
                }
                case BOARD_BUZZER_STOP:
                {
                    ESP_LOGI(TAG, "Buzzer stop");
                    buzzer->stop();
                    break;
                }
                case BOARD_BUZZER_SET_FREQUENCY:
                {
                    ESP_LOGI(TAG, "Buzzer set fr");
                    buzzer->set_frequency(msg.u.u32);
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
    buzzer->~Buzzer();
}

void BoardTx::setup() noexcept
{
    nvs_flash_init();
    gpio_install_isr_service(0);

    _button_isr_ctx_t button_1 = {0};
    _button_isr_ctx_t button_2 = {1};

    buttons.emplace_back(GPIO_NUM_34, &_buttons_isr_cb, &button_1);
//    buttons.emplace_back(GPIO_NUM_35, &_buttons_isr_cb, &button_2);
}

void BoardTx::run() noexcept
{

    while (1) {
        button_state_msg_t msg;
        if(_tx_queue.receive(&msg, 0))
        {
            ESP_LOGI(TAG, "Message received state: %d", msg.state);
            if(msg.state == PRESSED)
            {
                buttons[msg.button_num].set_state(PRESSED);
            }

            if(msg.state == RELEASED)
            {
                button_last_state_t last_state = buttons[msg.button_num].get_state();
                if (last_state.state != PRESSED)
                    continue;
                TickType_t now = xTaskGetTickCount();
                if (now - last_state.ticks > _ms2ticks(LONG_PRESS_MS))
                {
                    if (msg.button_num == 0)
                    {
                        for (auto &cb: callbacks)
                        {
                            if (cb.first == BOARD_BTN1_DOUBLE_CLICK)
                                cb.second();
                        }
                    } else if (msg.button_num == 1)
                    {
                        for (auto &cb: callbacks)
                        {
                            if (cb.first == BOARD_BTN2_DOUBLE_CLICK)
                                cb.second();
                        }
                    }
                } else if (now - last_state.ticks > _ms2ticks(SINGLE_PRESS_MS))
                {
                    if (msg.button_num == 0)
                    {
                        for (auto &cb: callbacks)
                        {
                            if (cb.first == BOARD_BTN1_SINGLE_CLICK)
                                cb.second();
                        }
                    } else if (msg.button_num == 1)
                    {
                        for (auto &cb: callbacks)
                        {
                            if (cb.first == BOARD_BTN2_SINGLE_CLICK)
                                cb.second();
                        }
                    }
                }
            }
        } else
        {
            ESP_LOGW(TAG, "Could not read message board_tx");
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void BoardTx::teardown() noexcept
{
    for(auto& button : buttons)
    {
        button.~Button();
    }
}

void BoardTx::_buttons_isr_cb(button_state_t state, void *ctx)
{
    auto* board_ctx = static_cast<_button_isr_ctx_t*>(ctx);

    button_state_msg_t msg = {
            .state = state,
            .button_num = board_ctx->button_num
    };

    assert(_tx_queue.send_form_isr(&msg));
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

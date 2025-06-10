#include "buttons.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "BUTTON";

static gptimer_handle_t gptimer = nullptr;

static struct isr_timer_context_t {
    gpio_num_t  port;
    button_cb_t cb_func;
    void*       ctx;
    int start_level;
} ist_ctx;

struct isr_gpio_context_t {
    isr_timer_context_t* timer_isr_ctx;
    gptimer_handle_t timer_num;
};

static bool timer_interrupt_handler(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *args)
{
    auto* user_ctx = static_cast<isr_timer_context_t*>(args);
//    ESP_LOGI(TAG, "ISR of timer triggered");
    if (ist_ctx.start_level == gpio_get_level(ist_ctx.port))
    {
        ist_ctx.cb_func(static_cast<button_state_t>(ist_ctx.start_level), ist_ctx.ctx);
    }
    gptimer_set_raw_count(timer, 0);
    gptimer_stop(timer);
    return ESP_OK == gpio_intr_enable(ist_ctx.port);
}

static void IRAM_ATTR gpio_interrupt_handler(void *args)
{
//    ESP_LOGI(TAG, "ISR of gpio triggered");
    auto* user_ctx = static_cast<isr_gpio_context_t*>(args);
    gpio_intr_disable(ist_ctx.port);

    ist_ctx.start_level = gpio_get_level(ist_ctx.port);

    gptimer_start(gptimer);
//    ESP_LOGI(TAG, "Timer start result: %d", ret);
//    timer_start (user_ctx->timer_group, user_ctx->timer_num);
}

Button::Button(gpio_num_t port, button_cb_t func, void* ctx) noexcept
{
    m_port = port;
    m_state = {.state = RELEASED, .ticks = 0};

    ist_ctx = {
            .port = m_port,
            .cb_func = func,
            .ctx = ctx,
    };

    gptimer_config_t timer_config = {
            .clk_src = GPTIMER_CLK_SRC_DEFAULT,
            .direction = GPTIMER_COUNT_UP,
            .resolution_hz = 1 * 1000 * 1000, // 1MHz, 1 tick = 1us
    };
    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &gptimer));


    gptimer_alarm_config_t alarm_cfg = {
            .alarm_count = 50 * 1000,
            .reload_count = 0,
            .flags = {
                    .auto_reload_on_alarm = 1
            }
    };
    gptimer_event_callbacks_t cb_struct = {
            .on_alarm = timer_interrupt_handler,
    };
    gptimer_set_alarm_action(gptimer, &alarm_cfg);
    gptimer_register_event_callbacks(gptimer, &cb_struct, &ist_ctx);
    gptimer_enable(gptimer);


    isr_gpio_context_t isr_gpio_ctx = {
            .timer_isr_ctx = &ist_ctx,
    };


    gpio_set_direction(m_port, GPIO_MODE_INPUT);
    gpio_set_pull_mode(m_port, GPIO_PULLUP_ONLY);
    gpio_set_intr_type(m_port, GPIO_INTR_ANYEDGE);
    gpio_isr_handler_add(m_port, gpio_interrupt_handler, &isr_gpio_ctx);

//    ESP_LOGI(TAG, "Button configured, timer %p", gptimer);
}

Button::~Button()
{
    gpio_isr_handler_remove(m_port);
}

button_last_state_t &Button::get_state()
{
    return m_state;
}

void Button::set_state(button_state_t state)
{
    m_state.state = state;
    m_state.ticks = xTaskGetTickCount();
}

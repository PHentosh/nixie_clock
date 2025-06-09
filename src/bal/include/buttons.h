#ifndef EXPERIMENTS_BUTTONS_H
#define EXPERIMENTS_BUTTONS_H

#include "cstdint"
#include <freertos/FreeRTOS.h>
#include "driver/gpio.h"
#include "driver/gptimer.h"

enum button_state_t {
    PRESSED,
    RELEASED
};

struct button_last_state_t {
    button_state_t state;
    TickType_t     ticks;
};

typedef void(*button_cb_t)(button_state_t state, void* ctx);

class Button
{
private:
    gpio_num_t m_port;
    button_last_state_t m_state;


public:
    explicit Button(gpio_num_t port, button_cb_t func, void* ctx) noexcept;
//    Button() noexcept;
    ~Button();

    Button(const Button &) = delete;
    Button(Button &&) = default;
    Button &operator=(const Button &) = delete;
    Button &operator=(Button &&) = default;

    button_last_state_t& get_state();
    void set_state(button_state_t state);
};

#endif //EXPERIMENTS_BUTTONS_H

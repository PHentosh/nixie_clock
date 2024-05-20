#ifndef EXPERIMENTS_BUTTONS_H
#define EXPERIMENTS_BUTTONS_H

#include "cstdint"
#include "driver/gpio.h"


class Button
{
private:
    gpio_num_t m_port;

public:
    explicit Button(gpio_num_t port) noexcept;
    ~Button() = default;

    Button(const Button &) = default;
    Button(Button &&) = default;
    Button &operator=(const Button &) = default;
    Button &operator=(Button &&) = default;

    bool is_pressed() const;
};

#endif //EXPERIMENTS_BUTTONS_H

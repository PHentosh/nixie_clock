#ifndef EXPERIMENTS_BUZZER_H
#define EXPERIMENTS_BUZZER_H

#include "cstdint"
#include "driver/gpio.h"


class Buzzer
{
private:
    gpio_num_t m_port;

public:
    explicit Buzzer(gpio_num_t port) noexcept;
    ~Buzzer();

    Buzzer(const Buzzer &) = default;
    Buzzer(Buzzer &&) = default;
    Buzzer &operator=(const Buzzer &) = default;
    Buzzer &operator=(Buzzer &&) = default;

    bool start();
    bool stop();
    bool set_frequency(uint32_t frequency);
};

#endif //EXPERIMENTS_BUZZER_H

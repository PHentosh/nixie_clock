#include "buttons.h"


#include "driver/gpio.h"

Button::Button(gpio_num_t port) noexcept
{
    m_port = port;

    gpio_set_direction(m_port, GPIO_MODE_INPUT);
    gpio_set_pull_mode(m_port, GPIO_PULLUP_ONLY);
    gpio_set_intr_type(m_port, GPIO_INTR_NEGEDGE);
    gpio_intr_enable(m_port);
    gpio_isr_register(intr_cb, (void *) "Test", 0, nullptr);
//    esp_intr_alloc(ETS_GPIO_INTR_SOURCE, 0, intr_cb, (void *) "Test", NULL);
}

bool Button::is_pressed() const
{
    return gpio_get_level(m_port);
}

static void Button::intr_cb(void *arg)
{

}

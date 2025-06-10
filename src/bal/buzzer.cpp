#include "buzzer.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "esp_assert.h"
#include "esp_err.h"


#define PWM_TIMER LEDC_TIMER_1
#define PWM_CHANNEL LEDC_CHANNEL_0
#define PWM_MODE LEDC_HIGH_SPEED_MODE
//#define PWM_TIMER LEDC_TIMER_0
//#define PWM_TIMER LEDC_TIMER_0
//#define PWM_TIMER LEDC_TIMER_0

static const char *TAG = "BUZZER";

Buzzer::Buzzer(gpio_num_t port) noexcept
{
    ledc_timer_config_t ledc_timer = {
            .speed_mode       = PWM_MODE,
            .duty_resolution  = LEDC_TIMER_13_BIT,
            .timer_num        = PWM_TIMER,
            .freq_hz          = 2400    ,
            .clk_cfg          = LEDC_AUTO_CLK
    };
    if (ESP_OK != ledc_timer_config(&ledc_timer))
        ESP_LOGE(TAG, "Failed to configure timer for buzzer PWM signal");

    ledc_channel_config_t ledc_channel = {
            .gpio_num       = port,
            .speed_mode     = PWM_MODE,
            .channel        = PWM_CHANNEL,
            .intr_type      = LEDC_INTR_DISABLE,
            .timer_sel      = PWM_TIMER,
            .duty           = 4096,
            .hpoint         = 0
    };
    if (ESP_OK != ledc_channel_config(&ledc_channel))
        ESP_LOGE(TAG, "Failed to configure channel for buzzer PWM signal");

    ledc_set_duty(ledc_channel.speed_mode, ledc_channel.channel, 4096);
    ledc_update_duty(ledc_channel.speed_mode, ledc_channel.channel);

    if (ESP_OK != ledc_timer_pause(PWM_MODE, PWM_TIMER))
        ESP_LOGE(TAG, "Failed to pause timer");
}

bool Buzzer::stop()
{
    return ESP_OK == ledc_timer_pause(PWM_MODE, PWM_TIMER);
}

bool Buzzer::start()
{
    return ESP_OK == ledc_timer_resume(PWM_MODE, PWM_TIMER);
}

bool Buzzer::set_frequency(uint32_t frequency)
{
    return ESP_OK == ledc_set_freq(PWM_MODE, PWM_TIMER, frequency);
}

Buzzer::~Buzzer()
{
    ledc_timer_config_t time_config = {
            .speed_mode = PWM_MODE,
            .timer_num = PWM_TIMER,
            .deconfigure = true
    };
    esp_err_t ret = ledc_timer_config(&time_config);
    assert(!ret);

}

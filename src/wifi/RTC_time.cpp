#include <array>
#include <ctime>
#include <cstdlib>

#include "RTC_time.h"

//SNTP
#include <sys/time.h>
#include "esp_attr.h"
#include "esp_sntp.h"

//wifi
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#define PRINT_TIME

#define MAX_CALLBACKS 5

static const char *TAG = "TIMER";

#define EXAMPLE_ESP_WIFI_SSID      "iHomeWave"
#define EXAMPLE_ESP_WIFI_PASS      "b@r@b01@"

static EventGroupHandle_t s_wifi_event_group;

static uint8_t s_retry_num = 0;

#define WIFI_CONNECTED_BIT        BIT0
#define WIFI_FAIL_BIT             BIT1
#define EXAMPLE_ESP_MAXIMUM_RETRY 3

static std::array<std::pair<timer_event_t, timer_cb_t>, MAX_CALLBACKS> callbacks;
static size_t callback_num = 0;

static class Timer* _task_timer = nullptr;

extern "C" int setenv (const char *__string, const char *__value, int __overwrite);
extern "C" void tzset();

class Timer final : public OSAL::Task
{
public:
    OSAL::Queue<timer_msg_t, 10> m_queue {nullptr};

private:
    struct current_time {
        uint8_t hour;
        uint8_t min;
        uint8_t sec;

        bool operator== (const current_time& rhs) const noexcept
        {
            return hour == rhs.hour and min == rhs.min;
        }

        bool operator== (const tm& rhs) const noexcept
        {
            return hour == rhs.tm_hour and min == rhs.tm_min;
        }

    };

    current_time now;

public:
    explicit Timer() noexcept : OSAL::Task{} {}

private:
    void setup() noexcept final;
    void run() noexcept final;
    void teardown() noexcept final;
};

static void time_sync_notification_cb(timeval *tv)
{
    ESP_LOGI(TAG, "Time synchronized event");
}

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT)
    {
        switch (event_id)
        {
            case WIFI_EVENT_STA_START:
            {
                esp_wifi_connect();
                break;
            }
            case WIFI_EVENT_STA_DISCONNECTED:
            {
                if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY)
                {
                    esp_wifi_connect();
                    s_retry_num++;
                    ESP_LOGI(TAG, "retry to connect to the AP");
                } else {
                    xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
                }
                ESP_LOGI(TAG,"connect to the AP fail");
                break;
            }
        }
    } else if (event_base == IP_EVENT)
    {
        switch (event_id)
        {
            case IP_EVENT_STA_GOT_IP:
            {
                s_retry_num = 0;
                auto* event = static_cast<ip_event_got_ip_t*>(event_data);
                ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
                xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
            }
        }
    }
}

static void wifi_init_sta()
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        nullptr,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        nullptr,
                                                        &instance_got_ip));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        WIFI_EVENT_STA_DISCONNECTED,
                                                        &event_handler,
                                                        nullptr,
                                                        nullptr));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS,
            .threshold {
                    .authmode = WIFI_AUTH_WPA2_PSK
            },
            .pmf_cfg = {
                    .capable  = true,
                    .required = false
            },
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s", EXAMPLE_ESP_WIFI_SSID);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s", EXAMPLE_ESP_WIFI_SSID);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }

    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
    vEventGroupDelete(s_wifi_event_group);
}

static void init_sntp()
{
    ESP_LOGI(TAG, "Initializing SNTP");
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    sntp_set_time_sync_notification_cb(time_sync_notification_cb);
#ifdef CONFIG_SNTP_TIME_SYNC_METHOD_SMOOTH
    sntp_set_sync_mode(SNTP_SYNC_MODE_SMOOTH);
#endif
    esp_sntp_init();
}

static void sync_system_time()
{
    init_sntp();
    time_t now = 0;
    tm     timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 10;
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET and ++retry < retry_count)
    {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
    time(&now);
    localtime_r(&now, &timeinfo);
}

static void get_time(tm& timeinfo)  {

    time_t now;
    time(&now);
    localtime_r(&now, &timeinfo);
    // Is time set? If not, tm_year will be (1970 - 1900).
    if (timeinfo.tm_year < (2016 - 1900))
    {
        ESP_LOGI(TAG, "Time is not set yet. Connecting to WiFi and getting time over NTP.");
        sync_system_time();
        time(&now);
    }

    setenv("TZ", "GMT-3", 1);
    tzset();
    localtime_r(&now, &timeinfo);
}

void Timer::setup() noexcept
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES or ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    wifi_init_sta();
    sync_system_time();

    now = {
        .hour = 0,
        .min  = 0,
        .sec  = 0,
    };
}

void Timer::run() noexcept
{
    while (1)
    {
        timer_msg_t msg;
        if (m_queue.receive(&msg, 0))
        {
            switch (msg.event)
            {
                case TIMER_SYNC:
                {
                    sync_system_time();
                    break;
                }
                case TIMER_EVENT_SIZE:
                    break;
            }
        }

        tm     timeinfo;
        get_time(timeinfo);

        if (now != timeinfo)
        {
            now.hour = timeinfo.tm_hour;
            now.min  = timeinfo.tm_min;
            for (auto& el: callbacks)
            {
                if (el.first == TIMER_SET_TIME)
                {
                    el.second(timeinfo);
                }
            }
        }

#ifdef PRINT_TIME
        char strftime_buf[64];
        strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
        ESP_LOGI(TAG, "The current date/time in Lviv is: %s", strftime_buf);
#endif
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void Timer::teardown() noexcept
{
    m_queue.~Queue();
}


void timer_init(const OSAL::Task::init_t& timer_init)
{
    static std::aligned_storage_t<sizeof(Timer), alignof(Timer)> _task_rx_storage;

    assert(not _task_timer);
    _task_timer = new(&_task_rx_storage) Timer{};
    bool ret = _task_timer->start(timer_init);
    assert(ret);
}

bool timer_deinit()
{
    if(not _task_timer )
        return true;

    _task_timer->~Timer();
    return true;
}

bool timer_register_cb(timer_event_t on_event, timer_cb_t func)
{
    if (callback_num >= MAX_CALLBACKS)
        return false;

    callbacks[callback_num] = {on_event, func};
    callback_num++;
    return true;
}

void timer_cb(timer_msg_t* msg)
{

    if (not _task_timer)
    {
        ESP_LOGE(TAG, "Trying to send message before task inited");
        assert(false);
    }

    if (not _task_timer->m_queue.send(msg, 0))
    {
        ESP_LOGE(TAG, "Event queue full or an internal error occurred");
        assert(false);
    }
}

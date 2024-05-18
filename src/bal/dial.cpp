#include "esp_log.h"

#include "dial.h"

static const char *TAG = "DIAL";

bool Lamp::set_value(uint8_t val)
{

    uint8_t reg;
    if (ESP_OK != mcp23017_read_register(mcp_cfg, MCP23017_GPIO, group, &reg))
    {
        ESP_LOGE(TAG, "Reading register");
        return false;
    }
    assert(reg);

    val = reg & address;
    reg = reg & address;
    reg = reg | val;

    if (ESP_OK != mcp23017_write_register(mcp_cfg, MCP23017_GPIO, group, reg))
    {
        ESP_LOGE(TAG, "Writing register");
        return false;
    }
    return true;
}

Dial::~Dial() noexcept
{
    for(auto& el: lamps)
    {
        el.~Lamp();
    }
}

bool Dial::set_lamp_value(size_t ind, uint8_t value)
{
    if (lamps.size() < ind + 1)
    {
        ESP_LOGE("DIAL", "Index out of range");
        return false;
    }
    bool ret;

    switch (value)
    {
        case 0: ret = lamps[ind].set_value(NULY);  break;
        case 1: ret = lamps[ind].set_value(ONE);   break;
        case 2: ret = lamps[ind].set_value(TWO);   break;
        case 3: ret = lamps[ind].set_value(THREE); break;
        case 4: ret = lamps[ind].set_value(FOUR);  break;
        case 5: ret = lamps[ind].set_value(FIVE);  break;
        case 6: ret = lamps[ind].set_value(SIX);   break;
        case 7: ret = lamps[ind].set_value(SEVEN); break;
        case 8: ret = lamps[ind].set_value(EIGHT); break;
        case 9: ret = lamps[ind].set_value(NINE);  break;
        case UINT8_MAX: ret = lamps[ind ].set_value(DOT); break;
        default:
        {
            ESP_LOGE("DIAL", "Unknown number to set");
            ret = false;
        }
    }
    return ret;
}

bool Dial::set_time(tm *timeinfo)
{

    if (lamps.size() < 2)
    {
        ESP_LOGW("DIAL", "Can`t display time without at least 2 lamps");
        return true;
    }

    bool ret;

    ret =         set_lamp_value(0, timeinfo->tm_hour / 10);
    ret = ret and set_lamp_value(1, timeinfo->tm_hour % 10);

    if (lamps.size() < 4)
        return ret;

    ret = ret and set_lamp_value(2, timeinfo->tm_min / 10);
    ret = ret and set_lamp_value(3, timeinfo->tm_min % 10);

    if (lamps.size() < 6)
        return ret;

    ret = ret and set_lamp_value(5, timeinfo->tm_sec / 10);
    ret = ret and set_lamp_value(6, timeinfo->tm_sec % 10);

    return ret;
}

void Dial::add_lamp(mcp23017_t *mcp_cfg, uint8_t addr, mcp23017_gpio_t group)
{
    lamps.emplace_back(mcp_cfg, addr, group);
}

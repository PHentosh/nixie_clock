#ifndef EXPERIMENTS_DIAL_H
#define EXPERIMENTS_DIAL_H

#include <cstdint>
#include <cstddef>
#include <cassert>
#include <vector>

#include "esp_sntp.h"

#include "mcp23017.h"

#define NULY  0x00
#define ONE   0x11
#define TWO   0x22
#define THREE 0x33
#define FOUR  0x44
#define FIVE  0x55
#define SIX   0x66
#define SEVEN 0x77
#define EIGHT 0x88
#define NINE  0x99
#define DOT   0xAA

class Lamp
{
private:
    uint8_t         value = 0;
    mcp23017_t*     mcp_cfg;
    mcp23017_gpio_t group;
    uint8_t         address;

public:
    Lamp(mcp23017_t* mcp_cfg, uint8_t addr, mcp23017_gpio_t group) noexcept :
        mcp_cfg{mcp_cfg}, address{addr}, group{group} {};
    ~Lamp() noexcept = default;

    Lamp(const Lamp &) = default;
    Lamp(Lamp &&) = default;
    Lamp &operator=(const Lamp &) = default;
    Lamp &operator=(Lamp &&) = default;

    bool set_value(uint8_t val);
    uint8_t get_value() const noexcept { return value; };
};

class Dial
{
private:
    std::vector<Lamp> lamps;

public:
    Dial() noexcept = default;
    ~Dial() noexcept;

    Dial(const Dial &) = delete;
    Dial(Dial &&) = delete;
    Dial &operator=(const Dial &) = delete;
    Dial &operator=(Dial &&) = delete;

    void add_lamp(mcp23017_t* mcp_cfg, uint8_t addr, mcp23017_gpio_t group);

    bool set_time(tm& timeinfo);
    bool set_lamp_value(size_t ind, uint8_t value);

};

#endif //EXPERIMENTS_DIAL_H

#pragma once

#include <driver/gpio.h>

namespace EspFc
{

enum EscProtocolType
{
    UNKNOWN = 0,
    ONESHOT125,
    DSHOT150,
    DSHOT300,
    DSHOT600,
};

class BaseProtocol
{
public:
    BaseProtocol() = default;
    virtual ~BaseProtocol() = default;

    virtual esp_err_t init(gpio_num_t gpio_num) = 0;
    virtual esp_err_t sendCommand(unsigned int throttle) = 0;
    virtual esp_err_t cleanup() = 0;
};

}
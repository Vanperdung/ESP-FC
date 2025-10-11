#pragma once

#include <memory>
#include <vector>

#include <esp_err.h>
#include <driver/gpio.h>

#include "esc_protocol_factory.h"

#define ESC_CHANNEL_COUNT 4

namespace EspFc
{

class EscController
{
public:
    struct Channel
    {
        std::unique_ptr<BaseProtocol> protocol;
        EscProtocolType type;
    };

    EscController();

    esp_err_t initChannel(unsigned int channel_index,
                          EscProtocolType type,
                          gpio_num_t gpio_num);
    esp_err_t execute();

private:
    std::vector<Channel> channels_;
};

}
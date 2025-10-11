#pragma once

#include <esp_err.h>
#include <driver/rmt_tx.h>

#include "base_protocol.h"

namespace EspFc
{

class OneShot125Protocol : public BaseProtocol
{
public:
    OneShot125Protocol();
    ~OneShot125Protocol();

    esp_err_t init(gpio_num_t gpio_num);
    esp_err_t sendCommand(unsigned int throttle);
    esp_err_t cleanup();

private:
    inline rmt_symbol_word_t generate_command(unsigned int pulse);

    rmt_channel_handle_t channel_;
    rmt_encoder_handle_t encoder_; 
};

}
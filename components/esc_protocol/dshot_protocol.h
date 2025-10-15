#pragma once

#include <esp_err.h>
#include <driver/rmt_tx.h>

#include "base_protocol.h"

#define DSHOTXXX_RESOLUTION_HZ 40000000
#define DSHOTXXX_BLOCK_SYMBOLS 64

#define DEFAULT_DSHOT300_BAUD_RATE 300000
#define DEFAULT_DSHOT300_DELAY_US 50

namespace EspFc
{

class DShotProtocol : public BaseProtocol
{
public:
    struct DShotEncoder
    {
        RMT_ENCODER_FUNC_ATTR static size_t encode(rmt_encoder_t *encoder, rmt_channel_handle_t tx_channel,
                                                   const void *primary_data, size_t data_size, 
                                                   rmt_encode_state_t *ret_state);
        RMT_ENCODER_FUNC_ATTR static esp_err_t reset(rmt_encoder_t *encoder);
        RMT_ENCODER_FUNC_ATTR static esp_err_t del(rmt_encoder_t *encoder);

        rmt_encoder_t base;
        rmt_encoder_t *copy_encoder;
        rmt_encoder_t *bytes_encoder;
        rmt_symbol_word_t delay_symbol;
        int state;
    };

    explicit DShotProtocol(EscProtocolType type);
    ~DShotProtocol();

    esp_err_t init(gpio_num_t gpio_num);
    esp_err_t sendCommand(unsigned int throttle);
    esp_err_t cleanup();

private:
    esp_err_t createDShotEncoder();
    inline uint16_t generate_command(unsigned int throttle);

    unsigned int baud_rate_;
    unsigned int delay_us_;
    rmt_channel_handle_t channel_;
    DShotEncoder dshot_encoder_;
};

}
#include <esp_log.h>

#include "oneshot125_protocol.h"
#include "esc_protocol_factory.h"

#define ONESHOT125_MIN_PULSE 125
#define ONESHOT125_MAX_PULSE 250
#define ONESHOT125_SPACE 300

using namespace EspFc;

static const char *TAG = "ONESHOT125_PROTOCOL";

OneShot125Protocol::OneShot125Protocol()
{
    EscProtocolFactory::instance().registerType<OneShot125Protocol>(ONESHOT125);
}

OneShot125Protocol::~OneShot125Protocol()
{
    cleanup();
}

esp_err_t OneShot125Protocol::init(gpio_num_t gpio_num)
{
    rmt_tx_channel_config_t config = {
        .gpio_num = gpio_num,
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 1000000,
        .mem_block_symbols = 64,
        .trans_queue_depth = 1,
        .flags = {
            .with_dma = 0,
            .allow_pd = 0,
        }
    };

    ESP_ERROR_CHECK(rmt_new_tx_channel(&config, &channel_));
    ESP_ERROR_CHECK(rmt_enable(channel_));

    rmt_copy_encoder_config_t copy_encoder_config = {};
    ESP_ERROR_CHECK(rmt_new_copy_encoder(&copy_encoder_config, &encoder_));

    return ESP_OK;
}

esp_err_t OneShot125Protocol::sendCommand(unsigned int throttle)
{
    unsigned int pulse = ONESHOT125_MIN_PULSE + throttle * (ONESHOT125_MAX_PULSE - ONESHOT125_MIN_PULSE) / 100;
    if (pulse > ONESHOT125_MAX_PULSE)
    {
        ESP_LOGE(TAG, "Pulse exceeds the maximun supported range (%d)", pulse);
        return ESP_FAIL;
    }

    rmt_symbol_word_t command = generate_command(pulse);

    rmt_transmit_config_t transmit_config = {
        .loop_count = 0 // send once
    };

    ESP_ERROR_CHECK(rmt_transmit(channel_, encoder_, &command, sizeof(command), &transmit_config));

    return ESP_OK;
}

esp_err_t OneShot125Protocol::cleanup()
{
    ESP_ERROR_CHECK(rmt_disable(channel_));
    ESP_ERROR_CHECK(rmt_del_encoder(encoder_));
    ESP_ERROR_CHECK(rmt_del_channel(channel_));

    return ESP_OK;
}

inline rmt_symbol_word_t OneShot125Protocol::generate_command(unsigned int pulse)
{
    rmt_symbol_word_t command = {
        .duration0 = static_cast<short unsigned int>(pulse),
        .level0 = 1,
        .duration1 = static_cast<short unsigned int>(ONESHOT125_SPACE - pulse),
        .level1 = 0
    };

    return command;
}

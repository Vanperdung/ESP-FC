#include <esp_check.h>

#include "dshot_protocol.h"

using namespace EspFc;

static const char *TAG = "DSHOT_PROTOCOL";

DShotProtocol::DShotProtocol(EscProtocolType type)
{
    switch (type)
    {
    case DSHOT150:
        break;
    case DSHOT300:
        baud_rate_ = DEFAULT_DSHOT300_BAUD_RATE;
        delay_us_ = DEFAULT_DSHOT300_DELAY_US;
        break;
    case DSHOT600:
        break;
    
    default:
        break;
    }
}

DShotProtocol::~DShotProtocol()
{
    cleanup();
}

RMT_ENCODER_FUNC_ATTR size_t DShotProtocol::DShotEncoder::encode(rmt_encoder_t *encoder,
                                                                 rmt_channel_handle_t tx_channel,
                                                                 const void *primary_data,
                                                                 size_t data_size,
                                                                 rmt_encode_state_t *ret_state)
{
    DShotEncoder *dshot_encoder = __containerof(encoder, DShotEncoder, base);
    rmt_encode_state_t session_state;
    size_t len = 0; 

    *ret_state = RMT_ENCODING_RESET;

    switch (dshot_encoder->state)
    {
    case 0:
        /* Dispatch DShot frame */
        len = dshot_encoder->bytes_encoder->encode(dshot_encoder->bytes_encoder, tx_channel, 
                              primary_data, data_size, &session_state);
        if (session_state & RMT_ENCODING_COMPLETE)
        {
            /* Dispatched DShot frame, gonna send the delay */
            dshot_encoder->state = 1;
        }
        else if (session_state & RMT_ENCODING_MEM_FULL)
        {        

            *ret_state = static_cast<rmt_encode_state_t>(*ret_state | RMT_ENCODING_MEM_FULL);
            goto out;
        }
        /* Fall-through */
    case 1:
        /* Dispatch the delay between two DShot frames */
        len += dshot_encoder->copy_encoder->encode(dshot_encoder->copy_encoder, tx_channel, 
                              &dshot_encoder->delay_symbol, sizeof(rmt_symbol_word_t),
                              &session_state);
        if (session_state & RMT_ENCODING_COMPLETE)
        {
            /* Dispatched the delay, back to initial state */
            *ret_state = static_cast<rmt_encode_state_t>(*ret_state | RMT_ENCODING_COMPLETE);
            dshot_encoder->state = 0;
        }
        else if (session_state & RMT_ENCODING_MEM_FULL)
        {
            *ret_state = static_cast<rmt_encode_state_t>(*ret_state | RMT_ENCODING_MEM_FULL);
            goto out;
        }
    }

out:
    return len;
}

RMT_ENCODER_FUNC_ATTR esp_err_t DShotProtocol::DShotEncoder::reset(rmt_encoder_t *encoder)
{
    DShotEncoder *dshot_encoder = __containerof(encoder, DShotEncoder, base);

    rmt_encoder_reset(dshot_encoder->bytes_encoder);
    rmt_encoder_reset(dshot_encoder->copy_encoder);

    dshot_encoder->state = 0;

    return ESP_OK;
}

RMT_ENCODER_FUNC_ATTR esp_err_t DShotProtocol::DShotEncoder::del(rmt_encoder_t *encoder)
{
    DShotEncoder *dshot_encoder = __containerof(encoder, DShotEncoder, base);

    rmt_del_encoder(dshot_encoder->bytes_encoder);
    rmt_del_encoder(dshot_encoder->copy_encoder);
    
    return ESP_OK;
}

esp_err_t DShotProtocol::init(gpio_num_t gpio_num)
{
    rmt_tx_channel_config_t config = {
        .gpio_num = gpio_num,
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = DSHOTXXX_RESOLUTION_HZ,
        .mem_block_symbols = DSHOTXXX_BLOCK_SYMBOLS,
        .trans_queue_depth = 1,
        .flags = {
            .with_dma = 0,
            .allow_pd = 0,
        }};

    ESP_ERROR_CHECK(rmt_new_tx_channel(&config, &channel_));

    /* Initialize DShot encoder */
    ESP_ERROR_CHECK(createDShotEncoder());

    /* Enable TX channel */
    ESP_ERROR_CHECK(rmt_enable(channel_));

    return ESP_OK;
}

esp_err_t DShotProtocol::sendCommand(unsigned int throttle)
{
    uint16_t dshot_command = generate_command(throttle);

    rmt_transmit_config_t transmit_config = {
        .loop_count = 0, // send once
    };

    ESP_ERROR_CHECK(rmt_transmit(channel_, &dshot_encoder_.base, &dshot_command, sizeof(dshot_command), &transmit_config));

    return ESP_OK;
}

esp_err_t DShotProtocol::cleanup()
{
    ESP_ERROR_CHECK(rmt_disable(channel_));
    if (dshot_encoder_.bytes_encoder)
        ESP_ERROR_CHECK(rmt_del_encoder(dshot_encoder_.bytes_encoder));
    if (dshot_encoder_.copy_encoder)
        ESP_ERROR_CHECK(rmt_del_encoder(dshot_encoder_.copy_encoder));
    ESP_ERROR_CHECK(rmt_del_channel(channel_));

    return ESP_OK;
}

esp_err_t DShotProtocol::createDShotEncoder()
{
    /* Register callbacks for DShot encoder */
    dshot_encoder_.base.encode = DShotEncoder::encode;
    dshot_encoder_.base.reset = DShotEncoder::reset;
    dshot_encoder_.base.del = DShotEncoder::del;

    /* Calculate delay ticks between DShot frames */
    unsigned int delay_ticks = DSHOTXXX_RESOLUTION_HZ * delay_us_ / 1000000;

    dshot_encoder_.delay_symbol.level0 = 0;
    dshot_encoder_.delay_symbol.duration0 = delay_ticks / 2;
    dshot_encoder_.delay_symbol.level1 = 0;
    dshot_encoder_.delay_symbol.duration1 = delay_ticks / 2;

    /* In DSHOTXXX protocol, bit 1 and 0 are represented by a 74.850% and 37.425% duty cycle respectively */
    float period_ticks = static_cast<float>(DSHOTXXX_RESOLUTION_HZ) / baud_rate_;

    unsigned int t1h_ticks = static_cast<unsigned int>(period_ticks * 0.7485);
    unsigned int t1l_ticks = static_cast<unsigned int>(period_ticks) - t1h_ticks;
    unsigned int t0h_ticks = static_cast<unsigned int>(period_ticks * 0.37425);
    unsigned int t0l_ticks = static_cast<unsigned int>(period_ticks) - t0h_ticks;

    ESP_LOGI(TAG, "delay_ticks = %d, period_ticks = %f", delay_ticks, period_ticks);
    ESP_LOGI(TAG, "t1h_ticks = %d, t1l_ticks = %d", t1h_ticks, t1l_ticks);
    ESP_LOGI(TAG, "t0h_ticks = %d, t0l_ticks = %d", t0h_ticks, t0l_ticks);

    rmt_bytes_encoder_config_t bytes_encoder_config;
    bytes_encoder_config.bit0.level0 = 1;
    bytes_encoder_config.bit0.duration0 = t0h_ticks;
    bytes_encoder_config.bit0.level1 = 0;
    bytes_encoder_config.bit0.duration1 = t0l_ticks;
    bytes_encoder_config.bit1.level0 = 1;
    bytes_encoder_config.bit1.duration0 = t1h_ticks;
    bytes_encoder_config.bit1.level1 = 0;
    bytes_encoder_config.bit1.duration1 = t1l_ticks,
    bytes_encoder_config.flags.msb_first = 1;
    ESP_ERROR_CHECK(rmt_new_bytes_encoder(&bytes_encoder_config, &dshot_encoder_.bytes_encoder));

    rmt_copy_encoder_config_t copy_encoder_config = {};
    ESP_ERROR_CHECK(rmt_new_copy_encoder(&copy_encoder_config, &dshot_encoder_.copy_encoder));

    return ESP_OK;
};  

inline uint16_t DShotProtocol::generate_command(unsigned int throttle)
{
    uint16_t telemetry = 0;
    uint16_t val = ((static_cast<uint16_t>(throttle) << 1) | telemetry) & 0x0FFF;
    uint8_t crc = ((val ^ (val >> 4) ^ (val >> 8)) & 0x0F);
    uint16_t command = (val << 4) | crc;
    uint16_t frame = ((command & 0xFF) << 8) | ((command & 0xFF00) >> 8);

    ESP_LOGD(TAG, "Throttle=%u | CRC=%u | Command=0x%04X", throttle, crc, command);

    return frame;
}
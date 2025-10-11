#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <esp_log.h>

#include "esc_controller.h"

using namespace EspFc;

static const char *TAG = "ESC_CONTROLLER";

EscController::EscController()
    : channels_(ESC_CHANNEL_COUNT)
{
}

esp_err_t EscController::initChannel(unsigned int channel_index,
                                     EscProtocolType type,
                                     gpio_num_t gpio_num)
{
    if (channels_[channel_index].protocol)
    {
        ESP_LOGW(TAG, "Re-initializing the channel (%d)...", channel_index);
    }

    channels_[channel_index].protocol = EscProtocolFactory::instance().create(type);
    if (channels_[channel_index].protocol == nullptr)
    {
        ESP_LOGE(TAG, "Failed to initialize ESC protocol with type (%d)", type);
        return ESP_ERR_INVALID_ARG;
    }

    channels_[channel_index].type = type;

    ESP_ERROR_CHECK(channels_[channel_index].protocol->init(gpio_num));

    return ESP_OK;
}

esp_err_t EscController::execute()
{
    unsigned int throttle = 0;
    unsigned int count = 0;

    for (int i = 0; i < 10000; i++)
    {
        channels_[0].protocol->sendCommand(throttle);
        // vTaskDelay(pdMS_TO_TICKS(1));
    }

    ESP_LOGI(TAG, "Start increasing throttle!");

    while (throttle <= 100)
    {
        count++;
        if (count == 500)
        {
            throttle += 1;
            count = 0;
        }

        channels_[0].protocol->sendCommand(throttle);
        // vTaskDelay(pdMS_TO_TICKS(1));
    }

    return ESP_OK;
}
#include <esp_err.h>
#include <esp_log.h>

static const char *TAG = "MAIN.CPP";

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "Hello world!");

    esp_err_t ret = ESP_OK;

    ESP_LOGI(TAG, "Bye world!, ret = 0x%x", ret);
}

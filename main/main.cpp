#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <esp_err.h>
#include <esp_log.h>

#include "esc_controller.h"

static const char *TAG = "MAIN.CPP";

extern "C" void app_main(void)
{
    EspFc::EscController esc_controller;

    esc_controller.initChannel(0, EspFc::DSHOT300, GPIO_NUM_18);

    esc_controller.execute();

}

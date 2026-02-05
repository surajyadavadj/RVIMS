#include "wifi_hal.h"
#include "app_wifi_task.h"
#include "app_camera.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

void app_main(void)
{
    WiFi_Init();
    while (!WiFi_IsConnected()) {
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    App_Camera_Init();
    App_Camera_StartTask();
    App_WiFi_StartTask();

    ESP_LOGI("MAIN", "SYSTEM READY");
}

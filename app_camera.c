#include "app_camera.h"
#include "camera_hal.h"
#include "esp_camera.h"
#include "esp_log.h"
#include "app_wifi_task.h"  

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "APP_CAMERA";

static volatile bool capture_request = false;

static void camera_task(void *arg)
{
    ESP_LOGI(TAG, "Camera task started");

    while (1) {
        if (!capture_request) {
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }

        capture_request = false;

        camera_fb_t *fb = esp_camera_fb_get();
        if (!fb) {
            ESP_LOGE(TAG, "Capture failed");
            continue;
        }

        ESP_LOGI(TAG, "Image captured, size=%d bytes", fb->len);

        /*  NEXT STEP: send JPEG here (UDP) */

        esp_camera_fb_return(fb);
    }
}

void App_Camera_Init(void)
{
    if (Camera_Init() != CAM_OK) {
        ESP_LOGE(TAG, "Camera init failed");
        return;
    }

    Camera_Start();
    ESP_LOGI(TAG, "Camera initialized");
}

void App_Camera_StartTask(void)
{
    xTaskCreatePinnedToCore(
        camera_task,
        "camera_task",
        8192,
        NULL,
        5,
        NULL,
        1
    );
}

// void App_Camera_CaptureOnce(void)
// {
//     capture_request = true;
//     ESP_LOGI(TAG, "Capture request received");
// }

void App_Camera_CaptureOnce(void)
{
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        ESP_LOGE("CAM", "Capture failed");
        return;
    }

    WiFi_SendJPEG(fb->buf, fb->len);

    esp_camera_fb_return(fb);
    ESP_LOGI("CAM", "Image captured & sent");
}

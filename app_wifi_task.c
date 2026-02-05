#include "app_wifi_task.h"
#include "wifi_hal.h"
#include "app_camera.h"

#include "lwip/sockets.h"
#include "lwip/inet.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "APP_WIFI";

#define SERVER_IP   "192.168.1.106"
#define EVENT_PORT  5000
#define CMD_PORT    9100
#define IMAGE_PORT   9200
#define MAX_PAYLOAD  1400

static int event_sock = -1;
static struct sockaddr_in server_addr;

static void wifi_task(void *arg)
{
    while (!WiFi_IsConnected()) {
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    event_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(EVENT_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    ESP_LOGI(TAG, "STEP-1: Ready to send events");

    vTaskDelete(NULL);
}

static void command_rx_task(void *arg)
{
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(CMD_PORT),
        .sin_addr.s_addr = htonl(INADDR_ANY)
    };

    bind(sock, (struct sockaddr *)&addr, sizeof(addr));
    ESP_LOGI(TAG, "Waiting for server command");

    char rx[64];
    while (1) {
        int len = recvfrom(sock, rx, sizeof(rx)-1, 0, NULL, NULL);
        if (len > 0) {
            rx[len] = 0;

            ESP_LOGI(TAG, "RX CMD: %s", rx);

            if (strcmp(rx, "CAPTURE:1") == 0) {
                App_Camera_CaptureOnce();
            }
        }
    }
}

void App_WiFi_StartTask(void)
{
    xTaskCreate(wifi_task, "wifi_evt", 4096, NULL, 5, NULL);
    xTaskCreate(command_rx_task, "wifi_cmd", 4096, NULL, 5, NULL);
}

void WiFi_SendEvent(const char *msg)
{
    if (event_sock < 0) return;

    sendto(event_sock,
           msg,
           strlen(msg),
           0,
           (struct sockaddr *)&server_addr,
           sizeof(server_addr));

    ESP_LOGI(TAG, "EVENT sent → %s", msg);
}

typedef struct __attribute__((packed)) {
    uint16_t frame_id;
    uint16_t chunk_id;
    uint16_t total_chunks;
    uint16_t payload_size;
} jpeg_hdr_t;

void WiFi_SendJPEG(const uint8_t *data, size_t len)
{
    static uint16_t frame_id = 0;
    frame_id++;

    int img_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (img_sock < 0) {
        ESP_LOGE(TAG, "Image socket create failed");
        return;
    }

    struct sockaddr_in img_addr = {
        .sin_family = AF_INET,
        .sin_port   = htons(IMAGE_PORT),
        .sin_addr.s_addr = inet_addr(SERVER_IP),
    };

    uint16_t total_chunks = (len + MAX_PAYLOAD - 1) / MAX_PAYLOAD;

    for (uint16_t i = 0; i < total_chunks; i++) {
        uint8_t packet[sizeof(jpeg_hdr_t) + MAX_PAYLOAD];

        jpeg_hdr_t *hdr = (jpeg_hdr_t *)packet;
        hdr->frame_id = frame_id;
        hdr->chunk_id = i;
        hdr->total_chunks = total_chunks;

        size_t offset = i * MAX_PAYLOAD;
        size_t chunk = (len - offset > MAX_PAYLOAD) ? MAX_PAYLOAD : (len - offset);
        hdr->payload_size = chunk;

        memcpy(packet + sizeof(jpeg_hdr_t), data + offset, chunk);

        sendto(img_sock,
               packet,
               sizeof(jpeg_hdr_t) + chunk,
               0,
               (struct sockaddr *)&img_addr,
               sizeof(img_addr));
    }

    close(img_sock);

    ESP_LOGI(TAG, "JPEG sent: size=%d bytes, chunks=%d", len, total_chunks);
}

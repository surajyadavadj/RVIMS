#pragma once

#include <stdint.h>
#include <stddef.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

/* JPEG frame container */
typedef struct {
    uint8_t *data;
    size_t   len;
} jpeg_frame_t;

/* Global queue: Camera → WiFi */
extern QueueHandle_t g_frame_queue;

/* Camera APIs */
void App_Camera_Init(void);
void App_Camera_StartTask(void);

/* Trigger one-shot capture (called from WiFi CMD task) */
void App_Camera_CaptureOnce(void);

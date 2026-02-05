#pragma once
#include <stdint.h>   
#include <stddef.h>  

#ifdef __cplusplus
extern "C" {
#endif

/* Start WiFi background task */
void App_WiFi_StartTask(void);

/* Send event to server (STEP-1) */
void WiFi_SendEvent(const char *msg);

/* Command RX task start */
void App_WiFi_StartCmdRxTask(void);
void WiFi_SendJPEG(const uint8_t *data, size_t len);


#ifdef __cplusplus
}
#endif

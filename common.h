#pragma once
#include <string>

#define EVENT_PORT        5000
#define ESP32_CMD_PORT    9100
#define IMAGE_PORT        9200

#define ESP32_IP          "192.168.1.101"   // ESP32-CAM IP
#define SERVER_IP         "0.0.0.0"

struct EventData {
    int         event_id;
    std::string raw;
};

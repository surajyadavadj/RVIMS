#include "common.h"
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <unistd.h>

int main(int argc, char* argv[]) {
    if (argc < 2) return -1;

    int event_id = atoi(argv[1]);

    int sock = socket(AF_INET, SOCK_DGRAM, 0);

    sockaddr_in esp{};
    esp.sin_family = AF_INET;
    esp.sin_port   = htons(ESP32_CMD_PORT);
    inet_pton(AF_INET, ESP32_IP, &esp.sin_addr);

    std::string cmd = "CAPTURE:" + std::to_string(event_id);

    sendto(sock, cmd.c_str(), cmd.size(), 0,
           (sockaddr*)&esp, sizeof(esp));

    std::cout << "[CMD] Sent to ESP32 → " << cmd << std::endl;

    close(sock);
}

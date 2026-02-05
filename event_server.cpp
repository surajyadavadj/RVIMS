#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <fstream>
#include <algorithm>
/* ================= CONFIG ================= */
#define TCP_PORT        5000
#define ESP32_IP        "192.168.1.100"
#define ESP32_CMD_PORT  9100


std::string clean(const std::string& s)
{
    std::string out = s;
    out.erase(std::remove(out.begin(), out.end(), '\n'), out.end());
    out.erase(std::remove(out.begin(), out.end(), '\r'), out.end());
    return out;
}

/* ================= HELPERS ================= */
std::string getValue(const std::string& s, const std::string& k)
{
    size_t p = s.find(k + ":");
    if (p == std::string::npos) return "NA";

    p += k.size() + 1;
    size_t e = s.find(",", p);
    if (e == std::string::npos) e = s.size();

    return s.substr(p, e - p);
}

void write_stm32_json(const std::string& event,
                      const std::string& lat,
                      const std::string& lon,
                      const std::string& date,
                      const std::string& time)
{
    std::ofstream f("data/stm32.json");
    if (!f.is_open()) return;

    f << "{\n";
    f << "  \"event\": \"" << event << "\",\n";
    f << "  \"lat\": \""   << lat   << "\",\n";
    f << "  \"lon\": \""   << lon   << "\",\n";
    f << "  \"date\": \""  << date  << "\",\n";
    f << "  \"time\": \""  << time  << "\"\n";
    f << "}\n";

    f.close();
}

/* ================= MAIN ================= */
int main()
{
    /* ---------- TCP SERVER (STM32) ---------- */
    int tcp_sock = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in srv{};
    srv.sin_family = AF_INET;
    srv.sin_port   = htons(TCP_PORT);
    srv.sin_addr.s_addr = INADDR_ANY;

    bind(tcp_sock, (sockaddr*)&srv, sizeof(srv));
    listen(tcp_sock, 1);

    std::cout << "[SERVER] Waiting for STM32...\n";
    int client = accept(tcp_sock, nullptr, nullptr);
    std::cout << "[SERVER] STM32 connected\n";

    /* ---------- UDP SOCKET (ESP32) ---------- */
    int udp = socket(AF_INET, SOCK_DGRAM, 0);

    sockaddr_in esp{};
    esp.sin_family = AF_INET;
    esp.sin_port   = htons(ESP32_CMD_PORT);
    inet_pton(AF_INET, ESP32_IP, &esp.sin_addr);

    /* ---------- MAIN LOOP ---------- */
    while (1) {
        char buf[256]{};

        int n = recv(client, buf, sizeof(buf) - 1, 0);
        if (n <= 0) continue;

        buf[n] = 0;
        std::string rx(buf);

        std::cout << "[SERVER] RX: " << rx << std::endl;

std::string event = clean(getValue(rx, "EVENT"));
std::string lat   = clean(getValue(rx, "LAT"));
std::string lon   = clean(getValue(rx, "LON"));
std::string date  = clean(getValue(rx, "DATE"));
std::string time  = clean(getValue(rx, "TIME"));


        /* WRITE LIVE STM32 DATA */
        write_stm32_json(event, lat, lon, date, time);

        /* 2G DETECT → ESP32 IMAGE CAPTURE */
        if (event == "2G") {
            sendto(udp, "CAPTURE:1", 9, 0,
                   (sockaddr*)&esp, sizeof(esp));

            std::cout << "[SERVER] CMD → ESP32: CAPTURE\n";
        }
    }

    close(client);
    close(tcp_sock);
    close(udp);
    return 0;
} 
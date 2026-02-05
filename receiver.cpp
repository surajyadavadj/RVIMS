#include <arpa/inet.h>
#include <unistd.h>
#include <vector>
#include <fstream>
#include <ctime>
#include <iostream>
#include <map>
#include <sys/stat.h>
#include <sys/types.h>
#include <cstring>

#define IMAGE_PORT 9200

#pragma pack(push,1)
struct jpeg_hdr_t {
    uint16_t frame_id;
    uint16_t chunk_id;
    uint16_t total_chunks;
    uint16_t payload_size;
};
#pragma pack(pop)

struct FrameBuffer {
    uint16_t total = 0;
    uint16_t received = 0;
    std::vector<std::vector<uint8_t>> chunks;
};

int main()
{
    int sock = socket(AF_INET, SOCK_DGRAM, 0);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(IMAGE_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(sock, (sockaddr*)&addr, sizeof(addr));
    std::cout << "[IMAGE] Receiver ready on port 9200\n";

    // ensure data folder exists
    mkdir("data", 0777);

    std::map<uint16_t, FrameBuffer> frames;

    while (1) {
        uint8_t buf[1500];
        int len = recv(sock, buf, sizeof(buf), 0);
        if (len <= 0) continue;

        auto *hdr = (jpeg_hdr_t*)buf;
        uint16_t frame_id = hdr->frame_id;

        // safety: payload size check
        if (hdr->payload_size > len - sizeof(jpeg_hdr_t))
            continue;

        auto &f = frames[frame_id];

        if (f.chunks.empty()) {
            f.total = hdr->total_chunks;
            f.received = 0;
            f.chunks.resize(hdr->total_chunks);
        }

        // duplicate UDP packet protection
        if (hdr->chunk_id < f.total && f.chunks[hdr->chunk_id].empty()) {
            f.chunks[hdr->chunk_id].assign(
                buf + sizeof(jpeg_hdr_t),
                buf + sizeof(jpeg_hdr_t) + hdr->payload_size
            );
            f.received++;
        }

        if (f.received == f.total) {

            // create image folder 
            std::string folder = "image_" + std::to_string(frame_id);
            mkdir(folder.c_str(), 0777);

            // save image 
            std::string image_path =
                folder + "/image_" + std::to_string(frame_id) + ".jpg";

            std::ofstream img(image_path, std::ios::binary);
            for (auto &c : f.chunks)
                img.write((char*)c.data(), c.size());
            img.close();

            //  update esp32.json 
            std::ofstream js("data/esp32.json");

            time_t now = time(nullptr);
            char ts[64];
            strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", localtime(&now));

            js << "[\n";
            js << "  {\n";
            js << "    \"image\": \"" << image_path << "\",\n";
            js << "    \"lat\": 28.6141,\n";
            js << "    \"lon\": 77.2092,\n";
            js << "    \"time\": \"" << ts << "\"\n";
            js << "  }\n";
            js << "]\n";
            js.close();

            frames.erase(frame_id);

            std::cout << "[IMAGE] Saved & JSON updated: "
                      << image_path << std::endl;
        }
    }
}

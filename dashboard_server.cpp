#include <iostream>
#include <fstream>
#include <string>

#include "httplib.h"   // cpp-httplib header

// Utility: read file into string
std::string read_file(const std::string &path, bool binary = false)
{
    std::ios::openmode mode = std::ios::in;
    if (binary) mode |= std::ios::binary;

    std::ifstream file(path, mode);
    if (!file.is_open()) return "";

    return std::string(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>()
    );
}

int main()
{
    httplib::Server svr;

    //  HOME PAGE
       
    svr.Get("/", [](const httplib::Request &, httplib::Response &res) {
        std::string html = read_file("web/index.html");
        if (html.empty()) {
            res.status = 404;
            res.set_content("index.html not found", "text/plain");
            return;
        }
        res.set_content(html, "text/html");
    });

   
       //  STM32 PAGE
      
    svr.Get("/stm32", [](const httplib::Request &, httplib::Response &res) {
        std::string html = read_file("web/stm32.html");
        if (html.empty()) {
            res.status = 404;
            res.set_content("stm32.html not found", "text/plain");
            return;
        }
        res.set_content(html, "text/html");
    });

    
     //  ESP32 PAGE
      
    svr.Get("/esp32", [](const httplib::Request &, httplib::Response &res) {
        std::string html = read_file("web/esp32.html");
        if (html.empty()) {
            res.status = 404;
            res.set_content("esp32.html not found", "text/plain");
            return;
        }
        res.set_content(html, "text/html");
    });

   
       //  STM32 DATA API
      
    svr.Get("/api/stm32", [](const httplib::Request &, httplib::Response &res) {
        std::string json = read_file("data/stm32.json");
        if (json.empty()) {
            res.status = 404;
            res.set_content("{\"error\":\"stm32.json not found\"}", "application/json");
            return;
        }
        res.set_content(json, "application/json");
    });

    
     //  ESP32 DATA API
     
    svr.Get("/api/esp32", [](const httplib::Request &, httplib::Response &res) {
        std::string json = read_file("data/esp32.json");
        if (json.empty()) {
            res.status = 404;
            res.set_content("{\"error\":\"esp32.json not found\"}", "application/json");
            return;
        }
        res.set_content(json, "application/json");
    });

    /* 
       IMAGE SERVING
       Example URL:
       /images/image_14/image_14.jpg
       */
    svr.Get(R"(/images/(.*))", [](const httplib::Request &req, httplib::Response &res) {
        std::string path = req.matches[1];

        std::string file = read_file(path, true);
        if (file.empty()) {
            res.status = 404;
            res.set_content("Image not found", "text/plain");
            return;
        }

        res.set_content(file, "image/jpeg");
    });

   
     //  STATIC JS
      
    svr.Get(R"(/js/(.*))", [](const httplib::Request &req, httplib::Response &res) {
        std::string path = "web/js/" + req.matches[1].str();
        std::string js = read_file(path);
        if (js.empty()) {
            res.status = 404;
            res.set_content("JS not found", "text/plain");
            return;
        }
        res.set_content(js, "application/javascript");
    });

    
     //  STATIC CSS
      
    svr.Get(R"(/css/(.*))", [](const httplib::Request &req, httplib::Response &res) {
        std::string path = "web/css/" + req.matches[1].str();
        std::string css = read_file(path);
        if (css.empty()) {
            res.status = 404;
            res.set_content("CSS not found", "text/plain");
            return;
        }
        res.set_content(css, "text/css");
    });

    std::cout << "====================================\n";
    std::cout << " DASHBOARD SERVER RUNNING\n";
    std::cout << " URL: http://localhost:8080\n";
    std::cout << "====================================\n";

    svr.listen("0.0.0.0", 8080);
}

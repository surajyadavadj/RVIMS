# RVIMS
Railway Vibration & Impact Monitoring System

#  RVIMS – Railway Vibration & Impact Monitoring System

RVIMS is an IoT-based safety monitoring system designed for Indian Railways.
It detects abnormal vibration (≥2G), logs GPS location, captures images,
and displays real-time data on a web dashboard.
## System Architecture

STM32 (ADXL345 + GPS)
   ↓ TCP
Server (Event Processing)
   ↓ UDP
ESP32-CAM (Image Capture)
   ↓ UDP
Server (Image Store + Metadata)
   ↓
Web Dashboard

# Features
- 2G vibration detection
- GPS-based fault localization
- Automatic image capture
- Real-time web dashboard
- Railway fault classification logic
- Modular firmware & server design

##  Sensors Used

- ADXL345 – 3-axis accelerometer
- GPS module (UART)
- ESP32-CAM (OV2640)

##  Dashboard
- Live STM32 vibration & GPS data
- Image preview from ESP32-CAM
- Interactive map (Leaflet)
- Fault severity indicators

##  How to Run

### Server
bash
cd server
make
./event_server
./dashboard_server

# ESP32
Build using ESP-IDF v5.2
Flash to ESP32-CAM

# STM32
Flash firmware via STM32CubeIDE

📸 ESP32-CAM High-Speed TFT Camera
License Platform Status

A high-performance digital camera project built on the ESP32-CAM module. This firmware streams live video at 15+ FPS to a 2.4" ILI9341 TFT display and captures high-quality photos to an SD card with a modern, responsive UI.

✨ Key Features
⚡ High Performance: Optimized SPI/Buffer handling achieving 15+ FPS live viewfinder (QVGA 320x240).
🖥️ Real-time Display: Direct JPEG decoding and rendering to TFT screen.
💾 SD Card Photography: Capture and save images to SD card with a physical button press.
🎨 Modern UI:
On-screen FPS Counter/Gauge.
Status Bar (CPU Frequency, SD Card Status).
Visual Feedback for Capture (Success/Error animations).
Loading Bar on startup.
🛠️ Robustness: Error handling for Camera Init and SD Card mounting.
🛠️ Hardware Requirements
ESP32-CAM (AI-Thinker Model recommended).
TFT Display: ILI9341 (240x320) or compatible SPI display supported by TFT_eSPI.
MicroSD Card (Formatted FAT32).
Push Button (Tactile switch).
Wires & Breadboard/PCB.
🔌 Pinout Configuration
1. Camera Module (Internal)
Standard AI-Thinker Pinout is pre-configured in 
camera_pins.h
.

2. SD Card (SPI)
Explicitly defined in main code:

Pin Name	GPIO
CS	13
MOSI	15
MISO	2
SCK	14
⚠️ Note: GPIO 0, 2, 4, 12, 13, 14, 15, 16 are often shared or strapped on ESP32-CAM. Ensure no conflicts with the camera or boot mode. Using an external SD card slot on these pins requires careful wiring. If using the onboard SD card slot of the AI-Thinker module, these pins vary (often CS=13, MOSI=15, MISO=2, SCK=14), but verify your specific board schematic.

3. User Controls
Component	GPIO	Mode
Button	16	INPUT_PULLUP (Active Low)
4. TFT Display
Crucial: You must configure the TFT_eSPI library's User_Setup.h file to match your wiring.

MISO: Not usually needed for TFT write-only.
MOSI: Connect to ESP32 MOSI (or free pin).
SCLK: Connect to ESP32 SCK.
CS/DC/RST: Choose available free pins (e.g., 12, 4*).
Warning: GPIO 4 is the Flash LED. GPIO 12 is a strapping pin. Use with caution.
📦 Installation & Setup
1. Library Dependencies
Install the following libraries via Arduino IDE Library Manager:

TFT_eSPI by Bodmer.
JPEGDecoder by Bodmer.
ESP32 Board Support: Ensure esp32 by Espressif Systems is installed in Board Manager.
2. Configure TFT_eSPI
Navigate to your Arduino libraries folder: Documents/Arduino/libraries/TFT_eSPI.
Edit User_Setup.h.
Select your driver (e.g., #define ILI9341_DRIVER).
Define your pins.
Tip: Since pins are scarce on ESP32-CAM, you might need to share SPI bus with SD card or precise pin selection.
3. Flash Firmware
Open 
ESP32Cam_TFTDisplay.ino
 in Arduino IDE.
Select Board: AI Thinker ESP32-CAM.
Connect ESP32-CAM via FTDI/UART adapter (GPIO 0 -> GND for flashing).
Upload!
Disconnect GPIO 0 and Reset.
📸 Usage
Power On: The device will show a loading bar.
Viewfinder: Once ready, you will see the live camera feed.
Top Bar: Shows CPU Speed, SD Status, and Button Pin.
Top Left: FPS Gauge.
Take Photo: Press the button on GPIO 16.
Screen shows "CAPTURING...".
If successful: "PHOTO SAVED!" with image count.
Photos are saved in /photos/ directory on the SD card.
🤝 Contributing
Contributions, issues, and feature requests are welcome!

📄 License
This project is licensed under the MIT License.

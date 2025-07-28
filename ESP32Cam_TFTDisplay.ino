#include "esp_camera.h"
#include <TFT_eSPI.h>
#include <JPEGDecoder.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"

// Select camera model
#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"

// PIN DEFINITIONS
#define CAPTURE_BUTTON_PIN 16
#define SD_CS_PIN 13
#define SD_MOSI_PIN 15
#define SD_MISO_PIN 2
#define SD_SCK_PIN 14

// TFT display instance
TFT_eSPI tft = TFT_eSPI();

// OPTIMIZED BUFFERS
uint16_t *displayBuffer = NULL;
bool bufferReady = false;

// FPS TRACKING
uint32_t frameCount = 0;
uint32_t fpsTimer = 0;
float currentFPS = 0;
uint32_t lastFPSUpdate = 0;

// Enhanced UI elements
uint16_t uiBackgroundColor = tft.color565(15, 15, 25);  // Dark blue-gray
uint16_t uiBorderColor = tft.color565(60, 120, 200);    // Blue accent
uint16_t uiTextColor = TFT_WHITE;
uint16_t uiSuccessColor = tft.color565(0, 200, 100);    // Modern green
uint16_t uiErrorColor = tft.color565(255, 80, 80);      // Modern red

// Status bar variables
bool showStatusBar = true;
uint32_t statusBarTimer = 0;

// CAPTURE VARIABLES
volatile bool captureRequested = false;
unsigned long lastButtonPress = 0;
const unsigned long debounceDelay = 200;
int photoCount = 0;
bool sdCardAvailable = false;

// INTERRUPT HANDLER (MINIMAL)
void IRAM_ATTR buttonPressed() {
  unsigned long currentTime = millis();
  if (currentTime - lastButtonPress > debounceDelay) {
    captureRequested = true;
    lastButtonPress = currentTime;
  }
}

// MODERN PHOTO CAPTURE WITH ENHANCED UI
bool capturePhoto() {
  if (!sdCardAvailable) {
    // Show error message
    tft.fillRoundRect(50, 100, 220, 40, 8, uiErrorColor);
    tft.setTextColor(TFT_WHITE, uiErrorColor);
    tft.setTextSize(1);
    tft.setCursor(85, 115);
    tft.println("NO SD CARD DETECTED");
    delay(1500);
    return false;
  }
  
  // Show capturing indicator
  tft.fillRoundRect(80, 100, 160, 40, 8, uiBackgroundColor);
  tft.drawRoundRect(80, 100, 160, 40, 8, uiBorderColor);
  tft.setTextColor(uiTextColor, uiBackgroundColor);
  tft.setTextSize(1);
  tft.setCursor(125, 115);
  tft.println("CAPTURING...");
  
  // Switch to higher quality temporarily
  sensor_t *s = esp_camera_sensor_get();
  if (s) {
    s->set_framesize(s, FRAMESIZE_VGA);
    s->set_quality(s, 4);
    delay(100);
  }
  
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    if (s) {
      s->set_framesize(s, FRAMESIZE_QVGA);
      s->set_quality(s, 8);
    }
    return false;
  }
  
  photoCount++;
  String filename = "/photos/IMG_" + String(millis()) + ".jpg";
  
  File file = SD.open(filename.c_str(), FILE_WRITE);
  if (file) {
    file.write(fb->buf, fb->len);
    file.close();
    
    // Modern success notification
    tft.fillRoundRect(60, 90, 200, 60, 10, uiSuccessColor);
    tft.setTextColor(TFT_WHITE, uiSuccessColor);
    tft.setTextSize(1);
    tft.setCursor(85, 105);
    tft.println("PHOTO SAVED!");
    tft.setCursor(105, 120);
    tft.printf("Image #%d", photoCount);
    
    // Add a nice checkmark symbol
    tft.setTextSize(2);
    tft.setCursor(130, 130);
    tft.println("✓");
    
    delay(1000);
  }
  
  esp_camera_fb_return(fb);
  
  if (s) {
    s->set_framesize(s, FRAMESIZE_QVGA);
    s->set_quality(s, 8);
  }
  
  return true;
}

// FAST SD CARD INIT
void initSDCard() {
  SPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
  
  if (SD.begin(SD_CS_PIN, SPI, 4000000)) {
    sdCardAvailable = true;
    
    // Create photos directory if needed
    if (!SD.exists("/photos")) {
      SD.mkdir("/photos");
    }
  }
}

void setup() {
  Serial.begin(115200);
  
  // Setup button
  pinMode(CAPTURE_BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(CAPTURE_BUTTON_PIN), buttonPressed, FALLING);
  
  // Initialize SD card (quick)
  initSDCard();
  
  // Initialize display with enhanced settings
  tft.begin();
  tft.setRotation(0);
  tft.setSwapBytes(true);
  tft.fillScreen(TFT_BLACK);
  
  // Enhanced startup screen
  tft.fillScreen(TFT_BLACK);
  
  // Modern gradient-like header
  for (int i = 0; i < 25; i++) {
    uint16_t color = tft.color565(0, 40 + i * 4, 80 + i * 2);
    tft.drawFastHLine(0, i, 320, color);
  }
  
  // Main title
  tft.setTextColor(TFT_WHITE, TFT_TRANSPARENT);
  tft.setTextSize(2);
  tft.setCursor(85, 5);
  tft.println("ESP32-CAM");
  
  // Subtitle
  tft.setTextColor(TFT_CYAN, TFT_TRANSPARENT);
  tft.setTextSize(1);
  tft.setCursor(105, 22);
  tft.println("HIGH SPEED MODE");
  
  // Status indicators
  tft.setTextColor(TFT_GREEN, TFT_TRANSPARENT);
  tft.setCursor(5, 35);
  tft.printf("CPU: %dMHz", getCpuFrequencyMhz());
  
  tft.setTextColor(sdCardAvailable ? TFT_GREEN : TFT_RED, TFT_TRANSPARENT);
  tft.setCursor(5, 45);
  tft.printf("SD: %s", sdCardAvailable ? "READY" : "NONE");
  
  tft.setTextColor(TFT_YELLOW, TFT_TRANSPARENT);
  tft.setCursor(5, 55);
  tft.println("BTN: PIN 16");
  
  delay(2000);
  
  // OPTIMIZED CAMERA CONFIG
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  
  // SPEED-OPTIMIZED SETTINGS
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_QVGA;    // 320x240
  config.jpeg_quality = 8;               // Faster decode, still good quality
  config.fb_count = 2;                   // Double buffering
  config.fb_location = CAMERA_FB_IN_DRAM;
  config.grab_mode = CAMERA_GRAB_LATEST;
  
  if (esp_camera_init(&config) != ESP_OK) {
    tft.setTextColor(TFT_RED);
    tft.setCursor(5, 20);
    tft.println("CAMERA INIT FAILED!");
    while (true) delay(1000);
  }
  
  // SENSOR OPTIMIZATION FOR SPEED
  sensor_t *s = esp_camera_sensor_get();
  if (s) {
    s->set_framesize(s, FRAMESIZE_QVGA);
    s->set_quality(s, 6);                // Faster JPEG compression
    s->set_brightness(s, 1);
    s->set_contrast(s, 2);
    s->set_saturation(s, 2);
    s->set_gainceiling(s, GAINCEILING_2X); // Lower gain for speed
    s->set_colorbar(s, 0);
    s->set_whitebal(s, 1);
    s->set_gain_ctrl(s, 1);
    s->set_exposure_ctrl(s, 1);
    s->set_hmirror(s, 0);
    s->set_vflip(s, 0);
    s->set_aec2(s, 1);                   // Disable advanced exposure for speed
    s->set_awb_gain(s, 1);
    s->set_agc_gain(s, 1);               // Lower gain for faster processing
    s->set_aec_value(s, 300);            // Faster exposure
    s->set_special_effect(s, 0);
    s->set_wb_mode(s, 0);
    s->set_ae_level(s, 0);
    s->set_dcw(s, 1);
    s->set_bpc(s, 1);                    // Disable for speed
    s->set_wpc(s, 1);                    // Disable for speed
    s->set_raw_gma(s, 1);                // Disable for speed
    s->set_lenc(s, 1);                   // Disable for speed
  }
  
  // Disable flash
  pinMode(4, OUTPUT);
  digitalWrite(4, LOW);
  
  // Allocate display buffer
  displayBuffer = (uint16_t*)malloc(320 * 240 * 2);
  if (displayBuffer) {
    bufferReady = true;
  }
  
  // Max CPU frequency
  setCpuFrequencyMhz(240);
  
  delay(100);
  
  // Modern loading screen
  tft.fillScreen(TFT_BLACK);
  
  // Animated loading bar
  for (int i = 0; i <= 100; i += 5) {
    tft.fillRoundRect(60, 140, 200, 8, 4, uiBackgroundColor);
    tft.fillRoundRect(60, 140, (200 * i) / 100, 8, 4, uiBorderColor);
    
    tft.setTextColor(uiTextColor, TFT_BLACK);
    tft.setTextSize(1);
    tft.setCursor(135, 160);
    tft.printf("Loading %d%%", i);
    
    delay(20);
  }
  
  tft.fillScreen(TFT_BLACK);
  fpsTimer = millis();
  
  Serial.println("OPTIMIZED 15+ FPS MODE READY!");
}

void loop() {
  // Handle capture request
  if (captureRequested) {
    captureRequested = false;
    capturePhoto();
  }
  
  // Get frame
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) return;
  
  // OPTIMIZED JPEG DECODE
  if (JpegDec.decodeArray(fb->buf, fb->len)) {
    uint16_t w = JpegDec.width;
    uint16_t h = JpegDec.height;
    
    // Center image on display
    int16_t x = (tft.width() - w) / 2;
    int16_t y = (tft.height() - h) / 2;
    
    if (bufferReady) {
      // FAST BUFFER DECODE
      uint16_t *pBuf = displayBuffer;
      uint16_t bufIndex = 0;
      
      // Read all MCUs quickly
      while (JpegDec.read()) {
        uint16_t *pImg = JpegDec.pImage;
        uint16_t mcuPixels = JpegDec.MCUWidth * JpegDec.MCUHeight;
        
        // Fast memcpy to buffer
        if (bufIndex + mcuPixels <= (320 * 240)) {
          memcpy(&pBuf[bufIndex], pImg, mcuPixels * 2);
          bufIndex += mcuPixels;
        }
      }
      
      // INSTANT DISPLAY UPDATE
      tft.pushImage(x, y, w, h, displayBuffer);
      
    } else {
      // Direct rendering fallback
      while (JpegDec.read()) {
        tft.pushImage(
          JpegDec.MCUx * JpegDec.MCUWidth + x,
          JpegDec.MCUy * JpegDec.MCUHeight + y,
          JpegDec.MCUWidth,
          JpegDec.MCUHeight,
          JpegDec.pImage
        );
      }
    }
    
    // FPS CALCULATION (every 30 frames for efficiency)
    frameCount++;
    if (frameCount >= 30) {
      uint32_t elapsed = millis() - fpsTimer;
      currentFPS = (frameCount * 1000.0) / elapsed;
      
      // Modern FPS display with glass-like effect
      tft.fillRoundRect(5, 5, 90, 35, 6, uiBackgroundColor);
      tft.drawRoundRect(5, 5, 90, 35, 6, uiBorderColor);
      
      // Add subtle inner glow
      tft.drawRoundRect(6, 6, 88, 33, 5, tft.color565(40, 80, 120));
      
      tft.setTextColor(uiTextColor, uiBackgroundColor);
      tft.setTextSize(1);
      tft.setCursor(12, 12);
      tft.println("FPS");
      
      // Large FPS number
      tft.setTextSize(2);
      tft.setCursor(12, 22);
      if (currentFPS >= 15.0) {
        tft.setTextColor(uiSuccessColor, uiBackgroundColor);
      } else if (currentFPS >= 10.0) {
        tft.setTextColor(TFT_YELLOW, uiBackgroundColor);
      } else {
        tft.setTextColor(uiErrorColor, uiBackgroundColor);
      }
      tft.printf("%.1f", currentFPS);
      
      // Add small performance indicator
      tft.setTextSize(1);
      tft.setTextColor(uiTextColor, uiBackgroundColor);
      tft.setCursor(68, 28);
      if (currentFPS >= 15.0) {
        tft.println("●");  // Full circle for good performance
      } else if (currentFPS >= 10.0) {
        tft.println("◐");  // Half circle for moderate performance
      } else {
        tft.println("○");  // Empty circle for poor performance
      }
      
      // Reset counters
      frameCount = 0;
      fpsTimer = millis();
    }
  }
  
  // Return frame buffer immediately
  esp_camera_fb_return(fb);
}

// Cleanup function
void cleanup() {
  if (displayBuffer) {
    free(displayBuffer);
    displayBuffer = NULL;
  }
}

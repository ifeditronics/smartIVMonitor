#include "display.h"
#include "config.h"
#include <SPI.h>

// Instantiate TFT controller using specified CS, DC, and RST pins.
// MOSI (23) and SCLK (18) are handled by the ESP32 hardware VSPI.
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

/**
 * @brief Configures display registers and clears the screen.
 */
void initDisplay() {
    // Initialize the ST7735S 1.44" TFT screen with the green-tab driver settings
    tft.initR(INITR_144GREENTAB);
    
    // Set orientation (0 is default portrait mode, which is 128x128 square)
    tft.setRotation(0); 
    
    // Clear screen to start with a clean black canvas
    tft.fillScreen(COLOR_BLACK);
}

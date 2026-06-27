#ifndef DISPLAY_H
#define DISPLAY_H

#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>

// Global TFT instance shared across the project
extern Adafruit_ST7735 tft;

// Initialisation function
void initDisplay();

#endif // DISPLAY_H

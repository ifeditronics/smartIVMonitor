#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// Firmware Metadata
#define FIRMWARE_VERSION "v1.0.0"
#define OTA_HOSTNAME "DripCounter"

// GPIO Pin Assignments
// DO NOT CHANGE THESE PINS
#define TFT_CS      5
#define TFT_DC      2
#define TFT_RST     4
#define TFT_MOSI   23
#define TFT_SCLK   18

#define PIN_IR_SENSOR 34  // Analog Input (ADC1_CH6)
#define PIN_BUZZER    27  // Digital Output / DAC2

// Display Resolution (ST7735S 1.44")
#define DISPLAY_WIDTH  128
#define DISPLAY_HEIGHT 128

// Color Palette (RGB565 Formatted)
#define COLOR_BLACK     0x0000
#define COLOR_WHITE     0xFFFF
#define COLOR_CYAN      0x07FF  // Active status / highlights
#define COLOR_GREEN     0x07E0  // Ready / Normal state
#define COLOR_ORANGE    0xFD20  // Flowing state / Warning
#define COLOR_RED       0xF800  // Stopped state / Critical Alarm
#define COLOR_DARKGRAY  0x3186  // Borders and text labels
#define COLOR_MEDGRAY   0x7BEF  // Secondary details

// Sensor Calibration & Algorithm Settings
#define SENSOR_SAMPLE_INTERVAL_MS 10    // 100Hz sampling
#define SENSOR_DEBOUNCE_MS        250   // Minimum time between drops (prevents double triggers)
#define SENSOR_EMA_ALPHA          0.35f // Exponential Moving Average smoothing factor (light filtering)

// Pulse Detector Configuration Constants
#define SENSOR_RISE_THRESHOLD     6     // ADC counts above baseline to trigger transition to RISING
#define SENSOR_PEAK_DROP_THRESHOLD 2    // ADC counts drop below highest peak to trigger PEAK_FOUND
#define SENSOR_MIN_PULSE_HEIGHT   20    // Minimum height (peak - baseline) to qualify a real drip pulse
#define SENSOR_RETURN_TOLERANCE   4     // ADC counts within baseline range to qualify return

// Flow Status Timing
#define FLOW_TIMEOUT_MS           6000  // Time without drops before declaring FLOW STOPPED (6s)

// Battery Simulation Settings
#define BATTERY_DRAIN_TOTAL_MIN   180   // Simulates 3 hours to go 100% -> 0%
#define BATTERY_UPDATE_INTERVAL_MS 60000 // Update battery % and icon every minute

// Buzzer Durations (Active Buzzer, no frequency needed)
#define BUZZER_DUR_STARTUP_1   100
#define BUZZER_DUR_STARTUP_2   150
#define BUZZER_DUR_DRIP        40
#define BUZZER_DUR_ALARM       300

#endif // CONFIG_H

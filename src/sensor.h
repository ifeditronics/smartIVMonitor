#ifndef SENSOR_H
#define SENSOR_H

#include <Arduino.h>

// Flow State Enum
enum FlowStatus {
    FLOW_READY,     // Monitor is on, no drips detected yet
    FLOW_FLOWING,   // Drips are actively being detected
    FLOW_STOPPED    // Flow was active but has stopped
};

// Public Functions
void initSensor();
void updateSensor(); // Should be called periodically (every 10ms)

// Drip Detection Core Functions (requested by user)
int readSensor();
bool detectPeak(int smoothedSample);
void updateDripRate();

// Getters for UI and Main loop
int getRawValue();
float getSmoothedValue();
int getDripCount();
float getDripRateDPM();
FlowStatus getFlowStatus();
const char* getFlowStatusStr();
int getRunningMax();
int getRunningMin();
int getDynamicThreshold();

// Function to trigger a simulated drip (useful for testing or demonstration)
void simulateDrip();

#endif // SENSOR_H

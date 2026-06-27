#include "sensor.h"
#include "config.h"
#include "ui.h"

// State Variables
static int rawValue = 0;
static float smoothedValue = 0.0f;
static int dripCount = 0;
static float dripRateDPM = 0.0f;
static FlowStatus flowStatus = FLOW_READY;
static uint32_t lastDripTime = 0;

static int previousSample = 0;
static uint8_t fallingSamples = 0;

// State Machine Enum
enum PulseState {
    PULSE_IDLE,
    PULSE_RISING,
    PULSE_PEAK_FOUND,
    PULSE_RETURNING,
    PULSE_COUNT
};

// Pulse State Variables
static PulseState pulseState = PULSE_IDLE;
static float baseline = 0.0f;
static int highestPeak = 0;

// Drip Interval History (for DPM average calculation)
#define DRIP_HISTORY_SIZE 5
static uint32_t dripIntervals[DRIP_HISTORY_SIZE];
static int intervalIndex = 0;
static int intervalCount = 0;

/**
 * @brief Initializes the IR sensor and state variables.
 */
void initSensor() {
    pinMode(PIN_IR_SENSOR, INPUT);
    
    // Read initial raw value
    rawValue = analogRead(PIN_IR_SENSOR);
    smoothedValue = (float)rawValue;
    baseline = (float)rawValue;
    
    // Reset state trackers
    lastDripTime = 0;
    dripCount = 0;
    dripRateDPM = 0.0f;
    flowStatus = FLOW_READY;
    
    highestPeak = 0;
    pulseState = PULSE_IDLE;
    
    // Initialize intervals history
    for (int i = 0; i < DRIP_HISTORY_SIZE; i++) {
        dripIntervals[i] = 0;
    }
    intervalIndex = 0;
    intervalCount = 0;
}

/**
 * @brief Reads the raw ADC value of the IR sensor.
 * @return The raw ADC value (0-4095).
 */
int readSensor() {
    rawValue = analogRead(PIN_IR_SENSOR);
    return rawValue;
}

/**
 * @brief Pulse-based state machine for drip detection.
 * 
 * Flow:
 * IDLE -> Learn baseline. Transition to RISING if signal > baseline + SENSOR_RISE_THRESHOLD.
 * RISING -> Track highest Peak. Transition to PEAK_FOUND if sample stops rising.
 * PEAK_FOUND -> Calculate pulseHeight = highestPeak - baseline. If >= SENSOR_MIN_PULSE_HEIGHT,
 *               transition to RETURNING. Otherwise discard (noise) and return to IDLE.
 * RETURNING -> Wait until sample returns close to baseline. Transition to COUNT.
 * COUNT -> Trigger drip event (beep + DPM math) if debounce passes, then reset to IDLE.
 * 
 * @param smoothedSample The smoothed sensor sample.
 * @return True if a drip was counted.
 */
bool detectPeak(int smoothedSample) {
    uint32_t now = millis();
    bool dripDetected = false;
    
    switch (pulseState) {
        case PULSE_IDLE:
            // Step 1: Continuously learn baseline when detector is idle
            baseline = (baseline * 0.998f) + ((float)smoothedSample * 0.005f);
            
            // Step 2: Signal begins rising significantly above baseline
            if ((float)smoothedSample - baseline > SENSOR_RISE_THRESHOLD) {
                pulseState = PULSE_RISING;
                highestPeak = smoothedSample;
            }
            break;
            
        case PULSE_RISING:
            // Step 2: Track highest sample reached while pulse rises
            if (smoothedSample > highestPeak) {
                highestPeak = smoothedSample;
            }
            
            // Detect a real downward trend instead of one noisy sample
            if (smoothedSample < previousSample) {
                fallingSamples++;
            } else {
                fallingSamples = 0;
            }

            if (fallingSamples >= 3) {
                pulseState = PULSE_PEAK_FOUND;
                fallingSamples = 0;
            }

            previousSample = smoothedSample;
            break;
            
        case PULSE_PEAK_FOUND: {
            // Step 5: Calculate pulseHeight = highestPeak - baseline
            float pulseHeight = (float)highestPeak - baseline;
            
            if (pulseHeight >= SENSOR_MIN_PULSE_HEIGHT) {
                pulseState = PULSE_RETURNING;
            } else {
                // Ignore tiny peaks caused by noise - reset immediately
                highestPeak = 0;
                pulseState = PULSE_IDLE;
            }
            break;
        }
            
        case PULSE_RETURNING:
            // Step 4: Wait until signal naturally returns close to the learned baseline
            if (abs((float)smoothedSample - baseline) <= SENSOR_RETURN_TOLERANCE) {
                pulseState = PULSE_COUNT;
            }
            break;
            
        case PULSE_COUNT:
            // Step 6: Only count ONE drip and update stats
            // Debounce check exists only to reject double counts (e.g. signal bounce at baseline)
            if (now - lastDripTime >= SENSOR_DEBOUNCE_MS) {
                // Record interval
                if (lastDripTime > 0) {
                    uint32_t interval = now - lastDripTime;
                    dripIntervals[intervalIndex] = interval;
                    intervalIndex = (intervalIndex + 1) % DRIP_HISTORY_SIZE;
                    if (intervalCount < DRIP_HISTORY_SIZE) {
                        intervalCount++;
                    }
                }
                
                lastDripTime = now;
                dripCount++;
                flowStatus = FLOW_FLOWING;
                
                updateDripRate();
                
                // Beep active buzzer
                playDripChirp();
                
                dripDetected = true;
            }
            
            // Immediately reset trackers and return to IDLE
            highestPeak = 0;
            pulseState = PULSE_IDLE;
            break;
    }
    
    return dripDetected;
}

/**
 * @brief Calculates the drip rate in Drops Per Minute (DPM).
 */
void updateDripRate() {
    if (intervalCount == 0) {
        dripRateDPM = 0.0f;
        return;
    }
    
    // Average the recorded intervals
    uint32_t sum = 0;
    for (int i = 0; i < intervalCount; i++) {
        sum += dripIntervals[i];
    }
    
    float avgIntervalMs = (float)sum / intervalCount;
    if (avgIntervalMs > 0.0f) {
        dripRateDPM = 60000.0f / avgIntervalMs;
    } else {
        dripRateDPM = 0.0f;
    }
}

/**
 * @brief Periodically samples the sensor and runs the peak detection state machine.
 * Should be called every SENSOR_SAMPLE_INTERVAL_MS (10ms).
 */
void updateSensor() {
    uint32_t now = millis();
    
    // 1. Read sensor and apply Exponential Moving Average (EMA) filter with light filtering (alpha = 0.35)
    int raw = readSensor();
    smoothedValue = (SENSOR_EMA_ALPHA * (float)raw) + ((1.0f - SENSOR_EMA_ALPHA) * smoothedValue);
    
    // 2. Run state-machine pulse detection
    detectPeak((int)smoothedValue);
    
    // 3. Handle flow timeout (if no drips detected for FLOW_TIMEOUT_MS)
    if (flowStatus == FLOW_FLOWING && (now - lastDripTime > FLOW_TIMEOUT_MS)) {
        flowStatus = FLOW_STOPPED;
        dripRateDPM = 0.0f; // Flow has stopped
    }
}

/**
 * @brief Manually triggers a simulated drip (for UI demo or manual trigger).
 */
void simulateDrip() {
    uint32_t now = millis();
    
    // Calculate interval since last drip
    if (lastDripTime > 0) {
        uint32_t interval = now - lastDripTime;
        // Don't register intervals that are physically impossible
        if (interval > SENSOR_DEBOUNCE_MS) {
            dripIntervals[intervalIndex] = interval;
            intervalIndex = (intervalIndex + 1) % DRIP_HISTORY_SIZE;
            if (intervalCount < DRIP_HISTORY_SIZE) {
                intervalCount++;
            }
        }
    }
    
    lastDripTime = now;
    dripCount++;
    flowStatus = FLOW_FLOWING;
    updateDripRate();
}

// Getters to interface with UI (preserved exactly to keep UI compatibility)
int getRawValue() { return rawValue; }
float getSmoothedValue() { return smoothedValue; }
int getDripCount() { return dripCount; }
float getDripRateDPM() { return dripRateDPM; }
FlowStatus getFlowStatus() { return flowStatus; }
int getRunningMax() { return highestPeak; }
int getRunningMin() { return (int)baseline; }
int getDynamicThreshold() { return (int)(baseline + SENSOR_MIN_PULSE_HEIGHT); }

const char* getFlowStatusStr() {
    switch (flowStatus) {
        case FLOW_READY:   return "READY";
        case FLOW_FLOWING: return "FLOWING";
        case FLOW_STOPPED: return "STOPPED";
        default:           return "UNKNOWN";
    }
}

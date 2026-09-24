#pragma once
#include <Arduino.h>

// Two-point linear calibration: raw sensor voltage (mV) -> estimated concentration.
// You MUST fill these in yourself by measuring known-concentration glucose
// standards (e.g. 0, 50, 100, 200 mg/dL solutions) with each electrode and
// recording the AFE output voltage. This is per-electrode, per-batch, and
// drifts over time -- re-calibrate before trusting any reading.
struct LinearCal {
    // Defaults intentionally left uncalibrated (mvHigh == mvLow) so apply()
    // returns NaN and the display shows "(uncalibrated)" until you fill
    // these in from real measurements -- avoids silently showing a
    // meaningless number.
    float mvLow = 0.0f;    // sensor output at low-concentration standard
    float concLow = 0.0f;  // known concentration at that standard (mg/dL)
    float mvHigh = 0.0f;
    float concHigh = 200.0f;

    float apply(float mv) const {
        if (mvHigh == mvLow) return NAN;
        float slope = (concHigh - concLow) / (mvHigh - mvLow);
        return concLow + slope * (mv - mvLow);
    }
};

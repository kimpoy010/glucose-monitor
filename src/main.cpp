// ESP32-C3 dual-channel electrochemical glucose biosensor reader.
//
// Channel A (sweat): GOx enzyme electrode -> LMP91000 AFE (I2C, addr 0x48)
// Channel B (urine): GOx enzyme electrode -> ADS1115 16-bit ADC (I2C, addr 0x48/0x49
//   depending on ADDR pin strap -- change one AFE's address jumper if both are 0x48)
//
// IMPORTANT: readings are a relative electrochemical signal only until you
// run your own two-point calibration (see include/Calibration.h). Do not
// use this for medical decisions.

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include "LMP91000.h"
#include "Calibration.h"

// ---- Pins (adjust to your wiring) ----
static constexpr int PIN_SDA = 8;
static constexpr int PIN_SCL = 9;

// ---- Devices ----
LMP91000 sweatAFE(0x48);
Adafruit_ADS1115 urineADC;

// ---- Calibration (fill in from your own measurements) ----
LinearCal sweatCal;
LinearCal urineCal;

static constexpr uint32_t SAMPLE_INTERVAL_MS = 5000;
uint32_t lastSample = 0;

float readSweatMillivolts() {
    // LMP91000's VOUT pin must be wired to an ESP32-C3 ADC-capable GPIO.
    // Native ADC is noisy for small signals but adequate once amplified by the TIA.
    static constexpr int PIN_SWEAT_VOUT = 0; // ADC1_CH0 on C3
    uint32_t raw = analogReadMilliVolts(PIN_SWEAT_VOUT);
    return static_cast<float>(raw);
}

float readUrineMillivolts() {
    int16_t raw = urineADC.readADC_SingleEnded(0);
    return urineADC.computeVolts(raw) * 1000.0f;
}

void setup() {
    Serial.begin(115200);
    delay(500);

    Wire.begin(PIN_SDA, PIN_SCL);

    sweatAFE.begin(Wire);
    // gainSel=4 (~35k TIA gain), internal 2.5V ref, ~20% bias, negative polarity.
    // Tune per your electrode's expected current range and required bias voltage.
    sweatAFE.configure(/*gainSel=*/4, /*refSel=*/0, /*biasPercentSel=*/4, /*biasPolarity=*/0);
    sweatAFE.setMode(LMP91000::MODE_3LEAD_AMP);

    if (!urineADC.begin(0x49)) {
        Serial.println("ADS1115 (urine channel) not found -- check wiring/address");
    }
    urineADC.setGain(GAIN_ONE); // +/-4.096V range; narrow it once you know your signal range

    Serial.println("mv_sweat,mg_dl_sweat,mv_urine,mg_dl_urine");
}

void loop() {
    uint32_t now = millis();
    if (now - lastSample < SAMPLE_INTERVAL_MS) return;
    lastSample = now;

    float sweatMv = readSweatMillivolts();
    float urineMv = readUrineMillivolts();

    float sweatConc = sweatCal.apply(sweatMv);
    float urineConc = urineCal.apply(urineMv);

    Serial.printf("%.1f,%.1f,%.1f,%.1f\n", sweatMv, sweatConc, urineMv, urineConc);
}

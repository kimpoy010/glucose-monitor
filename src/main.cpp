// ESP32-C3 dual-channel electrochemical glucose biosensor reader + OLED display.
// Bare-bones discrete version: no LMP91000/ADS1115 breakouts.
//
// Each channel is a 2-electrode amperometric cell (working + reference/counter
// tied together) feeding a discrete op-amp transimpedance amplifier (TIA):
//   working electrode -> op-amp inverting input (virtual ground)
//   feedback resistor Rf between op-amp inverting input and output
//   op-amp non-inverting input -> bias voltage (e.g. VCC/2 divider)
//   reference/counter electrodes -> same bias voltage node
//   op-amp output -> ESP32-C3 ADC pin
//
// A 128x64 SSD1306 I2C OLED displays both readings, refreshed each sample.
//
// This trades away the LMP91000's active bias control loop and the ADS1115's
// 16-bit resolution for near-zero extra hardware cost. The native ESP32-C3
// ADC is noisy and nonlinear, especially at low voltages -- this firmware
// leans on heavy oversampling to compensate, but expect more drift and less
// repeatability than the AFE-based version.
//
// IMPORTANT: readings are a relative electrochemical signal only until you
// run your own two-point calibration (see include/Calibration.h). Do not
// use this for medical decisions.

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "Calibration.h"

// ---- Analog channel pins (adjust to your wiring) ----
static constexpr int PIN_SWEAT_TIA_OUT = 0; // ADC1_CH0
static constexpr int PIN_URINE_TIA_OUT = 1; // ADC1_CH1

static constexpr int OVERSAMPLE_COUNT = 64; // averaged per reading to fight ADC noise

// ---- OLED (I2C, SDA/SCL) ----
static constexpr int PIN_SDA = 8;
static constexpr int PIN_SCL = 9;
static constexpr uint8_t OLED_ADDR = 0x3C; // common default; some boards are 0x3D
static constexpr int OLED_WIDTH = 128;
static constexpr int OLED_HEIGHT = 64;
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);

// ---- Calibration (fill in from your own measurements) ----
LinearCal sweatCal;
LinearCal urineCal;

static constexpr uint32_t SAMPLE_INTERVAL_MS = 5000;
uint32_t lastSample = 0;

float readMillivoltsOversampled(int pin) {
    uint64_t sum = 0;
    for (int i = 0; i < OVERSAMPLE_COUNT; i++) {
        sum += analogReadMilliVolts(pin);
        delayMicroseconds(200);
    }
    return static_cast<float>(sum) / OVERSAMPLE_COUNT;
}

void updateDisplay(float sweatMv, float sweatConc, float urineMv, float urineConc) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    display.setCursor(0, 0);
    display.println("Glucose Monitor (DIY)");
    display.drawLine(0, 10, OLED_WIDTH - 1, 10, SSD1306_WHITE);

    display.setCursor(0, 16);
    display.printf("Sweat: %.0f mV\n", sweatMv);
    display.setCursor(0, 28);
    if (isnan(sweatConc)) {
        display.println("  (uncalibrated)");
    } else {
        display.printf("  ~%.0f mg/dL\n", sweatConc);
    }

    display.setCursor(0, 42);
    display.printf("Urine: %.0f mV\n", urineMv);
    display.setCursor(0, 54);
    if (isnan(urineConc)) {
        display.println("  (uncalibrated)");
    } else {
        display.printf("  ~%.0f mg/dL\n", urineConc);
    }

    display.display();
}

void setup() {
    Serial.begin(115200);
    delay(500);

    analogReadResolution(12);
    analogSetPinAttenuation(PIN_SWEAT_TIA_OUT, ADC_11db); // full ~0-3.3V range
    analogSetPinAttenuation(PIN_URINE_TIA_OUT, ADC_11db);

    Wire.begin(PIN_SDA, PIN_SCL);
    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
        Serial.println("SSD1306 OLED not found -- check wiring/address");
    } else {
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 0);
        display.println("Glucose Monitor");
        display.println("Warming up...");
        display.display();
    }

    Serial.println("mv_sweat,mg_dl_sweat,mv_urine,mg_dl_urine");
}

void loop() {
    uint32_t now = millis();
    if (now - lastSample < SAMPLE_INTERVAL_MS) return;
    lastSample = now;

    float sweatMv = readMillivoltsOversampled(PIN_SWEAT_TIA_OUT);
    float urineMv = readMillivoltsOversampled(PIN_URINE_TIA_OUT);

    float sweatConc = sweatCal.apply(sweatMv);
    float urineConc = urineCal.apply(urineMv);

    Serial.printf("%.1f,%.1f,%.1f,%.1f\n", sweatMv, sweatConc, urineMv, urineConc);
    updateDisplay(sweatMv, sweatConc, urineMv, urineConc);
}

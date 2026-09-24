# Glucose Monitor (ESP32-C3, sweat + urine, DIY biosensor)

**⚠️ Not a medical device.** This is a research/educational electrochemical
biosensor rig. Sweat and urine glucose do NOT reliably track blood glucose
(see "Limitations" below). Do not use it to make insulin or treatment decisions.

## What this measures

Two independent electrochemical channels, each a 3-electrode glucose oxidase
(GOx) enzyme sensor read through an analog front-end (AFE):

- **Sweat channel** — GOx electrode → LMP91000 AFE (I2C) → ESP32-C3 ADC
- **Urine channel** — GOx electrode → ADS1115 16-bit ADC (I2C)

The GOx enzyme oxidizes glucose and produces a current proportional
(roughly, over a limited range) to glucose concentration. The AFE converts
that current to a voltage the ESP32-C3 can read.

## Hardware

- ESP32-C3 dev board
- LMP91000 breakout (SparkFun BOB-17124 or similar)
- ADS1115 breakout
- 2x three-electrode GOx enzyme strips/electrodes (working, reference, counter)
- I2C wiring: SDA → GPIO8, SCL → GPIO9 (change in `src/main.cpp` if different)
- LMP91000 VOUT → an ADC-capable GPIO (GPIO0 by default in the code)

Set the ADS1115's ADDR pin so it doesn't collide with the LMP91000's I2C
address (code assumes LMP91000 = 0x48, ADS1115 = 0x49).

## Firmware

PlatformIO project targeting `esp32-c3-devkitm-1`.

```
pio run -t upload
pio device monitor
```

Output is CSV over serial: `mv_sweat,mg_dl_sweat,mv_urine,mg_dl_urine`.

## Calibration (required, and the hard part)

`include/Calibration.h` holds a simple two-point linear map from AFE output
voltage to concentration. Out of the box it's a placeholder — you must:

1. Prepare known-concentration glucose standards (e.g. 0, 50, 100, 200 mg/dL
   in saline/artificial sweat or urine).
2. Apply each standard to the relevant electrode, let the reading settle,
   and record the millivolt output printed over serial.
3. Fill in `LinearCal::mvLow/concLow` and `mvHigh/concHigh` for each channel
   in `src/main.cpp`.

Enzyme electrodes drift and degrade — re-calibrate each time you swap
electrodes, and expect the linear range to be narrow (enzymatic sensors
saturate at higher concentrations).

## Limitations (read before you build)

- **Urine glucose** only appears once blood glucose exceeds the renal
  threshold (~180 mg/dL). Below that, urine glucose reads ~zero regardless
  of blood glucose — it's a threshold indicator, not a continuous curve.
- **Sweat glucose** concentration is roughly 100x lower than blood glucose,
  lags it by several minutes, and is heavily confounded by sweat rate,
  contamination, and electrode fouling. Expect a relative trend signal at
  best, not an absolute number, without extensive per-session calibration
  against a real glucometer.
- Native ESP32 ADC (used for the sweat channel via LMP91000 VOUT) is
  nonlinear at low voltages; consider routing that channel through a second
  ADS1115 channel instead if you need better accuracy.

## Repo layout

```
platformio.ini        - build config, library deps
include/LMP91000.h     - minimal driver for the sweat-channel AFE
include/Calibration.h  - two-point linear calibration struct
src/main.cpp           - sampling loop, wiring/pin config
```

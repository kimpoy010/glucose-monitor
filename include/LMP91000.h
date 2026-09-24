#pragma once
#include <Arduino.h>
#include <Wire.h>

// Minimal driver for the TI LMP91000 electrochemical AFE (sweat channel).
// Datasheet register map: TI SNAS489.
class LMP91000 {
public:
    explicit LMP91000(uint8_t i2cAddr = 0x48) : _addr(i2cAddr) {}

    bool begin(TwoWire &wire = Wire) {
        _wire = &wire;
        // Unlock the register write protection.
        writeReg(REG_LOCK, 0x00);
        return true;
    }

    // gain: TIA feedback resistor select (0..7 -> external..350k), see datasheet Table 5.
    // refSource: 0 = internal 2.5V ref, 1 = external.
    // biasPercent: 0..24 (of source voltage), biasPolarity: 0 = negative, 1 = positive.
    void configure(uint8_t gainSel, uint8_t refSel, uint8_t biasPercentSel, uint8_t biasPolarity) {
        uint8_t tiacn = (gainSel & 0x07) << 2;
        writeReg(REG_TIACN, tiacn);

        uint8_t refcn = ((refSel & 0x01) << 7) | ((biasPolarity & 0x01) << 6) | (biasPercentSel & 0x1F);
        writeReg(REG_REFCN, refcn);

        // 3-lead amperometric cell, enter deep sleep initially.
        writeReg(REG_MODECN, MODE_DEEP_SLEEP);
    }

    void setMode(uint8_t mode) { writeReg(REG_MODECN, mode); }

    static constexpr uint8_t MODE_DEEP_SLEEP   = 0x00;
    static constexpr uint8_t MODE_2LEAD_GALV   = 0x01;
    static constexpr uint8_t MODE_STANDBY      = 0x02;
    static constexpr uint8_t MODE_3LEAD_AMP    = 0x03;
    static constexpr uint8_t MODE_TEMP_TIA_OFF = 0x06;
    static constexpr uint8_t MODE_TEMP_TIA_ON  = 0x07;

private:
    static constexpr uint8_t REG_STATUS = 0x00;
    static constexpr uint8_t REG_LOCK   = 0x01;
    static constexpr uint8_t REG_TIACN  = 0x10;
    static constexpr uint8_t REG_REFCN  = 0x11;
    static constexpr uint8_t REG_MODECN = 0x12;

    uint8_t _addr;
    TwoWire *_wire = &Wire;

    void writeReg(uint8_t reg, uint8_t val) {
        _wire->beginTransmission(_addr);
        _wire->write(reg);
        _wire->write(val);
        _wire->endTransmission();
    }
};

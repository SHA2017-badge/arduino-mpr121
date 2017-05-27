#include <Wire.h>
#include "mpr121.h"

const unsigned int sdaPin = 26;
const unsigned int sclPin = 27;
const unsigned int irqPin = 25;

boolean touchStates[12] = {0};

volatile bool isInterrupt = false;
void newInterrupt() { isInterrupt = true; }

void setup() {
    Serial.begin(115200);
    Serial.setDebugOutput(true);
    Serial.println("Badge MPR121 Test");

    if (!initializeMPR()) {
        Serial.println("MPR121 Initialization failed!");
    }

}

void loop() {
    if (isInterrupt) {
        byte state = readRegister(MPR121_TOUCHSTATUS_L);
        for (int i = 0; i < 8; i++) {
            boolean pinState = (state & (1 << i));
            if (touchStates[i] != pinState) {
                // Pin state changed
                if (pinState) {
                    Serial.print("Pin ");
                    Serial.print(i);
                    Serial.println(" touched!");
                } else {
                    Serial.print("Pin ");
                    Serial.print(i);
                    Serial.println(" released!");
                }
                touchStates[i] = pinState;
            }
        }

        isInterrupt = false;
    }
}

boolean initializeMPR() {
    pinMode(irqPin, INPUT);

    Wire.begin(sdaPin, sclPin);

    // Soft reset MPR121
    setRegister(MPR121_SRST, 0x63);

    // Disable touch pins, put MPR121 into stop mode
    byte result = setRegister(MPR121_ECR, 0x0);
    if (result) {
        Serial.print("I2C Error, result ");
        char resultChar[3];
        sprintf(resultChar, "%02d", result);
        Serial.println(resultChar);
    }

    // Set baseline filters
    setRegister(MPR121_MHDR, 0x01);
    setRegister(MPR121_NHDR, 0x01);
    setRegister(MPR121_NCLR, 0x0E);
    setRegister(MPR121_FDLR, 0x00);

    setRegister(MPR121_MHDF, 0x01);
    setRegister(MPR121_NHDF, 0x05);
    setRegister(MPR121_NCLF, 0x01);
    setRegister(MPR121_FDLF, 0x00);

    setRegister(MPR121_NHDT, 0x00);
    setRegister(MPR121_NCLT, 0x00);
    setRegister(MPR121_FDLT, 0x00);

    setRegister(MPR121_DEBOUNCE, 0x00);
    setRegister(MPR121_CONFIG1, 0x10); // default, 16µA charge current
    setRegister(MPR121_CONFIG2, 0x20); // 0x5µs encoding, 1ms period


    // Set touch & release thresholds
    setThresholds(0x30, 6);

    // Enable touch 0-7, put MPR into run mode
    setRegister(MPR121_ECR, 0x88); // baseline tracking & initialize enable


    attachInterrupt(irqPin, newInterrupt, FALLING);
    return true;
}

void setThresholds(uint8_t touch, uint8_t release) {
    for (uint8_t i = 0; i < 12; i++) {
        setRegister(MPR121_TOUCHTH_0 + 2*i, touch);
        setRegister(MPR121_RELEASETH_0 + 2 * i, release);
    }
}

uint16_t filteredData(uint8_t pin) {
    if (pin > 12) return 0;
    return readRegister2b(MPR121_FILTDATA_0L + pin * 2);
}

uint16_t baselineData(uint8_t pin) {
    if (pin > 12) return 0;
    uint16_t baseline = readRegister(MPR121_BASELINE_0 + pin);
    return (baseline << 2);
}

byte setRegister(unsigned int reg, unsigned int val) {
    Wire.beginTransmission(MPR121_ADDR);
    Wire.write(reg);
    Wire.write(val);
    return Wire.endTransmission();
}

byte readRegister(unsigned int reg) {
    Wire.beginTransmission(MPR121_ADDR);
    Wire.write(reg);
    Wire.endTransmission(false);

    Wire.requestFrom(MPR121_ADDR, 1);
    byte content = Wire.read();
    byte result = Wire.endTransmission();

    if (!result)
        return content;
    else
        return 0x0;
}

// Read 2-byte register
uint16_t readRegister2b(unsigned int reg) {
    Wire.beginTransmission(MPR121_ADDR);
    Wire.write(reg);
    Wire.endTransmission(false);

    while (Wire.requestFrom(MPR121_ADDR, 2) != 2);
    uint16_t v = Wire.read();
    v |= ((uint16_t) Wire.read()) << 8;
    return v;
}

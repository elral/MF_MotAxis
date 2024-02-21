#pragma once

#include "Arduino.h"

class MotAxis
{
public:
    MotAxis();
    void begin(uint8_t Pin1, uint8_t Pin2);
    void attach(uint16_t Pin3, char *init);
    void detach();
    void set(int16_t messageID, char *setPoint);
    void update();

private:
    bool    _initialised;
    uint8_t _pin1, _pin2, _pin3;
};

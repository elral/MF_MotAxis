#pragma once

#include "Arduino.h"

#define OUTOFSYNC_RANGE 10  // 6   // if delta is below this value, then it's synced
#define OUTOFSYNC_TIME  200 // 100 // min. time for out of sync detection, in ms

extern "C" {
// callback functions
typedef void (*CustomEvent)(uint8_t, const char *);
};

enum {
    btnOnPress,
    btnOnRelease,
};

class MotAxis
{
public:
    MotAxis();
    void        attach(uint8_t analogIn, const char *syncName, uint8_t stepper, uint8_t startPosition, uint16_t movingTime, uint16_t maxSteps, uint8_t enablePin, uint8_t enableStatus);
    void        begin();
    void        detach();
    void        set(int16_t messageID, char *setPoint);
    void        update();
    void        setSetpoint(int16_t newPosition);
    int16_t     getSetpoint();
    static void attachHandler(CustomEvent handler);

private:
    void               setPowerSave(bool state);
    bool               _initialized;
    static CustomEvent _handler;
    int16_t            _setPoint;
    uint8_t            _analogIn;
    char               _syncName[20] = {0};
    uint8_t            _stepper;
    int16_t            _actualValue;
    int16_t            _deltaSteps;
    int16_t            _oldsetPoint;
    bool               _inMove;
    uint32_t           _time2move;
    bool               _synchronized;
    uint32_t           _lastSync;
    uint16_t           _movingTime;
    uint16_t           _maxSteps;
    uint8_t            _enablePin;
    uint8_t            _enableStatus;
};

#include "MotAxis.h"
#include "allocateMem.h"
#include "commandmessenger.h"
#include "Analog.h"
#include "Stepper.h"

/* **********************************************************************************
    This is just the basic code to set up your custom device.
    Change/add your code as needed.
********************************************************************************** */
CustomEvent MotAxis::_handler = NULL;

MotAxis::MotAxis()
{
    _initialized = false;
}

void MotAxis::attach(uint8_t analogIn, const char *syncName, uint8_t stepper, uint8_t startPosition,
                     uint16_t movingTime, uint16_t maxSteps, uint8_t enablePin, uint8_t enableStatus)
{
    _initialized  = true;
    _setPoint     = map(startPosition, 0, 100, -512, 511); // define start position, comes in 0...100%, must be -512...511
    _synchronized = true;                                  // on startup we will move to start position
    _lastSync     = 0;                                     // for calculation of out of sync, this is time dependent
    _analogIn     = analogIn;                              // where to get the actual value from
    _stepper      = stepper;                               // which stepper has to be moved
    _movingTime   = movingTime;                            // time for complete stroke in 1ms (0s to 25.5s)
    _maxSteps     = maxSteps;                              // number of steps for the complete stroke
    _enablePin    = enablePin;                             // output to en-/dis-able the stepper
    _enableStatus = enableStatus;                          // HIGH or LOW to enable the stepper
    strncpy(_syncName, syncName, 19);                      // button name on which out of sync is reported
}

// this must be called after reading the config as it can not be ensured that all device are initialized when the constructor is called
void MotAxis::begin()
{
    if (!_initialized)
        return;

    digitalWrite(_enablePin, _enableStatus); // enable stepper for moving to center position
    uint32_t startCentering = millis();
    do {
        Analog::readAverage();                                  // read analog and calculate floating average
        _actualValue = Analog::getActualValue(_analogIn) - 512; // range is -512 ... 511 for 270°
        _deltaSteps  = _setPoint - _actualValue;                // Stepper: 800 steps for 360° -> 600 steps for 270°
        Stepper::SetRelative(_stepper, _deltaSteps / 2);        // Accellib has it's own PID controller, so handles acceleration and max. speed by itself
        Stepper::update();                                      // ensure stepper is moving
        if (millis() - startCentering > _movingTime)            // do not move to center position forever, must be within max. moving time
        {
            _synchronized = false; // in this case we are not synchronized
            break;                 // centering must be within 3 sec in case one analog in is not connected
        }
    } while (abs(_deltaSteps) > 5);           // on startup center Axis
    digitalWrite(_enablePin, !_enableStatus); // disable stepper on startup
}

void MotAxis::detach()
{
    _initialized = false;
}

void MotAxis::set(int16_t messageID, char *setPoint)
{
    if (!_initialized)
        return;

    /* **********************************************************************************
        Each messageID has it's own value
        check for the messageID and define what to do.
        Important Remark!
        MessageID == -1 will be send from the connector when Mobiflight is closed
        Put in your code to shut down your custom device (e.g. clear a display)
        MessageID == -2 will be send from the connector when PowerSavingMode is entered
        Put in your code to enter this mode (e.g. clear a display)

    ********************************************************************************** */
    int32_t data = atoi(setPoint);

    switch (messageID) {
    case -1:
        // get's called when Mobiflight shuts down
        setPowerSave((bool)data);
    case -2:
        // get's called when PowerSavingMode is entered
        setPowerSave((bool)data);
    case 0:
        setSetpoint(data);
        break;
    case 1:
        /* code */
        break;
    case 2:
        /* code */
        break;
    default:
        break;
    }
}

void MotAxis::update()
{
    if (!_initialized)
        return;

    _actualValue = Analog::getActualValue(_analogIn) - 512; // range is -512 ... 511 for 270°
    _deltaSteps  = _setPoint - _actualValue;                // Stepper: 800 steps for 360° -> 600 steps for 270° -> with gear 1:2 = 900 steps
    Stepper::SetRelative(_stepper, _deltaSteps / 2);        // Accellib has it's own PID controller, so handles acceleration and max. speed by itself

    if (_oldsetPoint != _setPoint) // stepper must move
    {
        _oldsetPoint   = _setPoint;
        _inMove        = true;
        uint16_t accel = 1 + ((1000 - abs(_deltaSteps)) / 250); // consider longer time for small steps due to acceleration, might require optimization
        _time2move     = millis() + ((uint32_t)abs(_deltaSteps) * _movingTime * accel) / _maxSteps;
    }

    if (_time2move < millis() && _inMove) {
        _inMove = false;
    }

    if (abs(_deltaSteps) < OUTOFSYNC_RANGE) // if actual value is near setpoint
    {                                       // do I have to check for AutoTrim mode??? What happens if manual mode selected and actual value is setpoint -> synchronized = true,
                                            // next movement could be out of range, button press will be initiated
        _synchronized = true;               // we are synchronized
        _lastSync     = millis();           // save the time of last synchronization for detecting out of sync for more than specified time
    } else if (millis() - _lastSync >= OUTOFSYNC_TIME && _synchronized == true && !_inMove) {
        _synchronized = false;
        if (digitalRead(_enablePin) == _enableStatus) {
            //            digitalWrite(_enablePin, !_enableStatus);
            if (_handler)
                (*_handler)(btnOnPress, _syncName);
        }
    }
}

void MotAxis::setSetpoint(int16_t newPosition)
{
    if (!_initialized)
        return;

    // range is -500 ... 500
    // from UI setpoint must be in +/-0.1% so -1000 ... 1000
    if (newPosition < -1000)
        newPosition = -1000;
    if (newPosition > 1000)
        newPosition = 1000;
    _setPoint = newPosition / 2;
}

int16_t MotAxis::getSetpoint()
{
    if (!_initialized)
        return 0;
    return _setPoint;
}

void MotAxis::setPowerSave(bool state)
{
    if (!_initialized)
        return;

    if (state)
        digitalWrite(_enablePin, !_enableStatus); // disable stepper for moving to center position
    else
        digitalWrite(_enablePin, _enableStatus); // enable stepper for moving to center position
}

void MotAxis::attachHandler(CustomEvent newHandler)
{
    _handler = newHandler;
}

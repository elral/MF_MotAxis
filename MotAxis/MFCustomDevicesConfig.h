#pragma once

#include <Arduino.h>

// If no input custom deivece is defined, uncomment the following line
// const char CustomDeviceConfig[] PROGMEM = {""};

// Otherwise define your input custom devices
const char CustomDeviceConfig[] PROGMEM = 
{
    "3.4.TrimEnable:"
    "3.7.ThrottleEnable:"
    "1.10.SyncLostTrim:"
    "1.11.SyncLostThrottle:"
    "11.14.5.TrimWheel:"
    "11.15.5.Throttle:"
    "15.2.3.5.6.0.2.0.0.3.Stepper TrimWheel:"
    "17.ELRAL_MOTAXIS.21..Elral's Mot Axis:"
};

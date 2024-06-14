#include "MFCustomDevice.h"
#include "commandmessenger.h"
#include "allocateMem.h"
#include "MFEEPROM.h"
#ifdef HAS_CONFIG_IN_FLASH
#include "MFCustomDevicesConfig.h"
#else
const char CustomDeviceConfig[] PROGMEM = {};
#endif

extern MFEEPROM MFeeprom;

/* **********************************************************************************
    The custom device pins, type and configuration is stored in the EEPROM
    While loading the config the adresses in the EEPROM are transferred to the constructor
    Within the constructor you have to copy the EEPROM content to a buffer
    and evaluate him. The buffer is used for all 3 types (pins, type configuration),
    so do it step by step.
    The max size of the buffer is defined here. It must be the size of the
    expected max length of these strings.

    E.g. 6 pins are required, each pin could have two characters (two digits),
    each pins are delimited by "|" and the string is NULL terminated.
    -> (6 * 2) + 5 + 1 = 18 bytes is the maximum.
    The custom type is "MyCustomClass", which means 14 characters plus NULL = 15
    The configuration is "myConfig", which means 8 characters plus NULL = 9
    The maximum characters to be expected is 18, so MEMLEN_STRING_BUFFER has to be at least 18
********************************************************************************** */
#define MEMLEN_STRING_BUFFER 40

// ************************************************************
// Simulate a button press if manual move is detected
// ************************************************************
void handlerOnCustomDevice(uint8_t eventId, const char *name)
{
    cmdMessenger.sendCmdStart(kButtonChange);
    cmdMessenger.sendCmdArg(name);
    cmdMessenger.sendCmdArg(eventId);
    cmdMessenger.sendCmdEnd();
}

// reads a string from EEPROM or Flash at given address which is '.' terminated and saves it to the buffer
bool MFCustomDevice::getStringFromMem(uint16_t addrMem, char *buffer, bool configFromFlash)
{
    char     temp     = 0;
    uint8_t  counter  = 0;
    uint16_t length   = MFeeprom.get_length();
    do {
        if (configFromFlash) {
            temp = pgm_read_byte_near(CustomDeviceConfig + addrMem++);
            if (addrMem > sizeof(CustomDeviceConfig))
                return false;
        } else {
            temp = MFeeprom.read_byte(addrMem++);
            if (addrMem > length)
                return false;
        }
        buffer[counter++] = temp;              // save character and locate next buffer position
        if (counter >= MEMLEN_STRING_BUFFER) { // nameBuffer will be exceeded
            return false;                      // abort copying to buffer
        }
    } while (temp != '.'); // reads until limiter '.' and locates the next free buffer position
    buffer[counter - 1] = 0x00; // replace '.' by NULL, terminates the string
    return true;
}

MFCustomDevice::MFCustomDevice()
{
    _initialized = false;
}

/* **********************************************************************************
    Within the connector pins, a device name and a config string can be defined
    These informations are stored in the EEPROM like for the other devices.
    While reading the config from the EEPROM this function is called.
    It is the first function which will be called for the custom device.
    If it fits into the memory buffer, the constructor for the customer device
    will be called
********************************************************************************** */

void MFCustomDevice::attach(uint16_t adrPin, uint16_t adrType, uint16_t adrConfig, bool configFromFlash)
{
    if (adrPin == 0) return;

    /* **********************************************************************************
        Do something which is required to setup your custom device
    ********************************************************************************** */

    char    *syncName, *params, *p = NULL;
    char     parameter[MEMLEN_STRING_BUFFER];
    uint8_t  analogIn, enablePin, enableStatus, startPosition, stepperNumber;
    uint16_t movingTime, maxSteps;

    /* **********************************************************************************
        Read the Type from the EEPROM, copy it into a buffer and evaluate it
        The string get's NOT stored as this would need a lot of RAM, instead a variable
        is used to store the type
    ********************************************************************************** */
    getStringFromMem(adrType, parameter, configFromFlash);
    if (strcmp(parameter, "MOBIFLIGHT_MOTAXIS") == 0)
        _customType = MY_MOTAXIS;

    if (_customType == MY_MOTAXIS) {
        /* **********************************************************************************
            Check if the device fits into the device buffer
        ********************************************************************************** */
        if (!FitInMemory(sizeof(MotAxis))) {
            // Error Message to Connector
            cmdMessenger.sendCmd(kStatus, F("Custom Device does not fit in Memory"));
            return;
        }
        /* **********************************************************************************************
            Read the pins from the EEPROM, copy them into a buffer
            If you have set '"isI2C": true' in the device.json file, the first value is the I2C address
        ********************************************************************************************** */
        getStringFromMem(adrPin, parameter, configFromFlash);
        /* **********************************************************************************************
            Split the pins up into single pins. As the number of pins could be different between
            multiple devices, it is done here.
        ********************************************************************************************** */
        /* The stepper is defined in the connector, no pins required for this device                   */

        /* **********************************************************************************
            Read the configuration from the EEPROM, copy it into a buffer.
            stored in the eeprom like:
            "0|SyncLostTrim|4|0|0|50|4000|900|600|800"
            "AnalogInNumber|NameButton|enablePin|enableStatus|startPosition|movingTime|maxSteps|maxSpeed|maxAccel"
            AnalogInNumber      x.th device for AnalogIn configured in the connector
            NameButton          name of the Button which reports a sync loss, must be the same as configured in the connector
            enablePin           pin number of output which dis/enbles the stepper driver
            enableStatus        0 or 1 for enabling the stepper driver
            stepperNumber       x.th device for Stepper configured in the connector
            startPosition       on start up the axis will go to this position, in 0% ... 100%
            movingTime          measure the time for a complete stroke and define here, required for calculating sync loss
            maxSteps            maximum steps for a complete stroke, required for calculating sync loss
        ********************************************************************************** */
        // getStringFromEEPROM(adrConfig, parameter);    As long as no config string from the connector is available, define it here.
        /* **********************************************************************************
            Split the config up into single parameter. As the number of parameters could be
            different between multiple devices, it is done here.
            This is just an example how to process the init string. Do NOT use
            "," or ";" as delimiter for multiple parameters but e.g. "|"
            For most customer devices it is not required.
            In this case just delete the following
        ********************************************************************************** */
        char configTrim[] = "0|SyncLostTrim|4|0|0|50|4000|900";
        params            = strtok_r(configTrim, "|", &p); // change configTrim back to parameter once the config string from the connector is available
        analogIn          = atoi(params);

        params   = strtok_r(NULL, "|", &p);
        syncName = params;

        params    = strtok_r(NULL, "|", &p);
        enablePin = atoi(params);

        params       = strtok_r(NULL, "|", &p);
        enableStatus = atoi(params);

        params        = strtok_r(NULL, "|", &p);
        stepperNumber = atoi(params);

        params        = strtok_r(NULL, "|", &p);
        startPosition = atoi(params);

        params     = strtok_r(NULL, "|", &p);
        movingTime = atoi(params);

        params   = strtok_r(NULL, "|", &p);
        maxSteps = atoi(params);

        /* **********************************************************************************
            Next call the constructor of your custom device
            adapt it to the needs of your constructor
        ********************************************************************************** */
        // In most cases you need only one of the following functions
        // depending on if the constuctor takes the variables or a separate function is required
        _myMotAxis = new (allocateMemory(sizeof(MotAxis))) MotAxis();
        _myMotAxis->attach(analogIn, syncName, stepperNumber, startPosition, movingTime, maxSteps, enablePin, enableStatus);
        _myMotAxis->attachHandler(handlerOnCustomDevice);
        _myMotAxis->begin();
        _initialized = true;
    } else {
        cmdMessenger.sendCmd(kStatus, F("Custom Device is not supported by this firmware version"));
    }
}

/* **********************************************************************************
    The custom devives gets unregistered if a new config gets uploaded.
    Keep it as it is, mostly nothing must be changed.
    It gets called from CustomerDevice::Clear()
********************************************************************************** */
void MFCustomDevice::detach()
{
    _initialized = false;
    if (_customType == MY_MOTAXIS) {
        _myMotAxis->detach();
    }
}

/* **********************************************************************************
    Within in loop() the update() function is called regularly
    Within the loop() you can define a time delay where this function gets called
    or as fast as possible. See comments in loop().
    It is only needed if you have to update your custom device without getting
    new values from the connector.
    Otherwise just make a return; in the calling function.
    It gets called from CustomerDevice::update()
********************************************************************************** */
void MFCustomDevice::update()
{
    if (!_initialized) return;

    if (_customType == MY_MOTAXIS) {
        _myMotAxis->update();
    }
}

/* **********************************************************************************
    If an output for the custom device is defined in the connector,
    this function gets called when a new value is available.
    It gets called from CustomerDevice::OnSet()
********************************************************************************** */
void MFCustomDevice::set(int16_t messageID, char *setPoint)
{
    if (!_initialized) return;

    if (_customType == MY_MOTAXIS) {
        _myMotAxis->set(messageID, setPoint);
    }
}

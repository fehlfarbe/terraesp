#ifndef ACTUATOR_H
#define ACTUATOR_H
#include <Arduino.h>
#include <Debug.h>

enum ActuatorType
{
    ACTUATOR_TOGGLE = 0,
    ACTUATOR_SLIDER = 1,
    ACTUATOR_INPUT = 2
};

class Actuator
{
public:
    Actuator(String name, uint8_t gpio);
    ~Actuator();

    String getName();
    uint16_t getGPIO();

    virtual uint16_t getValue() = 0;
    virtual void setValue(uint16_t value) = 0;

    virtual void turnOn() = 0;
    virtual void turnOff() = 0;

    virtual void initGPIO() = 0;
    virtual void deinitGPIO() = 0;

    virtual String toString() = 0;
    virtual ActuatorType getType() = 0;

protected:
    String name;
    uint8_t gpio;

    bool initialized = false;

    Debug debug;
};

#endif
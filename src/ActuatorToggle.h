#ifndef ACTUATOR_TOGGLE_H
#define ACTUATOR_TOGGLE_H
#include <Arduino.h>
#include <Debug.h>
#include <Actuator.h>

class ActuatorToggle : public Actuator
{
public:
    ActuatorToggle(String name, uint8_t gpio, bool inverted = false);
    ~ActuatorToggle();

    bool isInverted();

    uint16_t getValue();
    void setValue(uint16_t value);

    void turnOn();
    void turnOff();

    void initGPIO();
    void deinitGPIO();

    String toString();
    ActuatorType getType();

protected:
    bool inverted;
};

#endif
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
    Actuator(String name, uint8_t gpio, ActuatorType type, bool inverted = false, uint16_t min = 0, uint16_t max = 255, uint8_t channel = 0);
    ~Actuator();

    String getName();
    uint16_t getGPIO();
    ActuatorType getType();
    bool isInverted();
    uint16_t getMin();
    uint16_t getMax();
    uint8_t getChannel();

    uint16_t getValue();
    void setValue(uint16_t value);

    String toString();

protected:
    String name;
    uint8_t gpio;
    ActuatorType type;
    bool inverted;
    uint16_t min;
    uint16_t max;
    uint8_t channel;

    Debug debug;
};

#endif
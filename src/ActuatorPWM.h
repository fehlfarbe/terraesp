#ifndef ACTUATOR_PWM_H
#define ACTUATOR_PWM_H
#include <Arduino.h>
#include <Debug.h>
#include <Actuator.h>

class ActuatorPWM : public Actuator
{
public:
    ActuatorPWM(String name, uint8_t gpio, uint16_t minVal = 0, uint16_t maxVal = 1023);
    ~ActuatorPWM();

    uint16_t getMin();
    uint16_t getMax();

    uint16_t getValue();
    void setValue(uint16_t value);

    void turnOn();
    void turnOff();

    void initGPIO();
    void deinitGPIO();

    String toString();
    ActuatorType getType();

protected:
    uint16_t minVal;
    uint16_t maxVal;
};

#endif
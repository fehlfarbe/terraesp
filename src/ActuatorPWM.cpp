#include "ActuatorPWM.h"

ActuatorPWM::ActuatorPWM(String name, uint8_t gpio, uint16_t minVal, uint16_t maxVal) : Actuator(name, gpio), minVal(minVal), maxVal(maxVal)
{
    initGPIO();
}

ActuatorPWM::~ActuatorPWM()
{
}


uint16_t ActuatorPWM::getMin()
{
    return minVal;
}

uint16_t ActuatorPWM::getMax()
{
    return maxVal;
}

uint16_t ActuatorPWM::getValue()
{
    return ledcRead(gpio);
}

void ActuatorPWM::setValue(uint16_t value)
{
    // if (value == LOW && initialized)
    // {
    //     turnOff();
    // }
    // else
    // {
    //     if (!initialized)
    //     {
    //         initGPIO();
    //     }
        if (value == HIGH)
            value = maxVal;
        value = min(value, maxVal);
        debug.printf("Change PWM on GPIO %d to value: %d\n", gpio, value);
        ledcWrite(gpio, value);
    // }
}

void ActuatorPWM::turnOn()
{
    debug.printf("Change PWM on GPIO %d to value: %d\n", gpio, maxVal);
    ledcWrite(gpio, maxVal);
}

void ActuatorPWM::turnOff()
{
    debug.printf("Turn off PWM on GPIO %d\n", gpio);
    deinitGPIO();
    pinMode(gpio, OUTPUT);
    digitalWrite(gpio, LOW);
}

void ActuatorPWM::initGPIO()
{
    ledcAttach(gpio, 24000, 10);  // 24 kHz, 10 bit
    initialized = true;
}

void ActuatorPWM::deinitGPIO()
{
    ledcDetach(gpio);
    initialized = false;
}

String ActuatorPWM::toString()
{
    return "ActuatorPWM " + name + " GPIO=" + String(gpio) +
           " min=" + String(minVal) + " max=" + String(maxVal);
}

ActuatorType ActuatorPWM::getType(){
    return ActuatorType::ACTUATOR_SLIDER;
}
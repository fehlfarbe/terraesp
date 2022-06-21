#include "ActuatorPWM.h"

ActuatorPWM::ActuatorPWM(String name, uint8_t gpio, uint16_t minVal, uint16_t maxVal, uint8_t channel) : Actuator(name, gpio), minVal(minVal), maxVal(maxVal), channel(channel)
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

uint8_t ActuatorPWM::getChannel()
{
    return channel;
}

uint16_t ActuatorPWM::getValue()
{
    return ledcRead(channel);
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
        debug.printf("Change PWM on channel %d to value: %d\n", channel, value);
        ledcWrite(channel, value);
    // }
}

void ActuatorPWM::turnOn()
{
    debug.printf("Change PWM on channel %d to value: %d\n", channel, maxVal);
    ledcWrite(channel, maxVal);
}

void ActuatorPWM::turnOff()
{
    debug.printf("Turn off PWM on gpio %d\n", gpio);
    deinitGPIO();
    pinMode(gpio, OUTPUT);
    digitalWrite(gpio, LOW);
}

void ActuatorPWM::initGPIO()
{
    ledcAttachPin(gpio, channel);  // assign RGB led pins to channels
    ledcSetup(channel, 12000, 10); // 12 kHz PWM, 10-bit resolution
    initialized = true;
}

void ActuatorPWM::deinitGPIO()
{
    ledcDetachPin(gpio);
    initialized = false;
}

String ActuatorPWM::toString()
{
    return "ActuatorPWM " + name + " GPIO=" + String(gpio) +
           " min=" + String(minVal) + " max=" + String(maxVal) + " channel=" +
           String(channel);
}

ActuatorType ActuatorPWM::getType(){
    return ActuatorType::ACTUATOR_SLIDER;
}
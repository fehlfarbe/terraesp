#include "ActuatorToggle.h"

ActuatorToggle::ActuatorToggle(String name, uint8_t gpio, bool inverted) : Actuator(name, gpio), inverted(inverted)
{
    initGPIO();
}

ActuatorToggle::~ActuatorToggle()
{
}

bool ActuatorToggle::isInverted()
{
    return inverted;
}

uint16_t ActuatorToggle::getValue()
{
    return (digitalRead(gpio) ^ inverted) != 0;
}

void ActuatorToggle::setValue(uint16_t value)
{
    debug.printf("Set GPIO %d to %d\n", gpio, value ^ inverted);
    digitalWrite(gpio, value ^ inverted);
}

void ActuatorToggle::turnOn()
{
    setValue(HIGH);
}

void ActuatorToggle::turnOff()
{
    setValue(LOW);
}

void ActuatorToggle::initGPIO()
{
    pinMode(gpio, OUTPUT); // ToDo: pullup/-down?
    digitalWrite(gpio, (uint8_t)inverted);
    initialized = true;
}

void ActuatorToggle::deinitGPIO(){

}

String ActuatorToggle::toString()
{
    return "ActuatorToggle " + name + " GPIO=" + String(gpio) + " inverted=" + String(inverted);
}

ActuatorType ActuatorToggle::getType(){
    return ActuatorType::ACTUATOR_TOGGLE;
}
#include "Actuator.h"

Actuator::Actuator(String name, uint8_t gpio) : name(name), gpio(gpio)
{
}

Actuator::~Actuator()
{
}

String Actuator::getName()
{
    return name;
}

uint16_t Actuator::getGPIO()
{
    return gpio;
}
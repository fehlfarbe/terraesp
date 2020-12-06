#include "Actuator.h"

Actuator::Actuator(String name, uint8_t gpio, ActuatorType type, bool inverted, uint16_t min, uint16_t max, uint8_t channel) : name(name), gpio(gpio), type(type), inverted(inverted), min(min), max(max), channel(channel)
{
    // set pin mode
    switch (type)
    {
    case ActuatorType::ACTUATOR_TOGGLE:
        pinMode(gpio, OUTPUT); // ToDo: pullup/-down?
        digitalWrite(gpio, (uint8_t)inverted);
        break;
    case ActuatorType::ACTUATOR_SLIDER:
        // Initialize channels
        // channels 0-15, resolution 1-16 bits, freq limits depend on resolution
        // ledcSetup(uint8_t channel, uint32_t freq, uint8_t resolution_bits);
        ledcAttachPin(gpio, channel); // assign RGB led pins to channels
        ledcSetup(channel, 12000, 10);     // 12 kHz PWM, 8-bit resolution
        break;
    default:
        break;
    }
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

ActuatorType Actuator::getType()
{
    return type;
}

bool Actuator::isInverted(){
    return inverted;
}

uint16_t Actuator::getMin(){
    return min;
}

uint16_t Actuator::getMax(){
    return max;
}

uint8_t Actuator::getChannel(){
    return channel;
}

uint16_t Actuator::getValue()
{
    switch (type)
    {
    case ACTUATOR_TOGGLE:
        return (digitalRead(gpio) ^ inverted) != 0;
        break;
    case ACTUATOR_SLIDER:
        return ledcRead(channel);
        break;
    case ACTUATOR_INPUT:
        debug.println("BTN_INPUT not supported!");
        break;
    }
    return 0;
}

void Actuator::setValue(uint16_t value)
{
    switch (type)
    {
    case ACTUATOR_TOGGLE:
        debug.printf("Set GPIO %d to %d\n", gpio, value ^ inverted);
        digitalWrite(gpio, value ^ inverted);
        break;
    case ACTUATOR_SLIDER:
        if (value <= max && value >= min)
        {
            //analogWrite(b.pin, value);
            debug.printf("Change PWM on channel %d to value: %d\n", channel, value);
            ledcWrite(channel, value);
        }
        break;
    default:
        debug.printf("Unknown actor type %d\n", type);
        break;
    }
}

String Actuator::toString(){
    return "Actuator " + name + " GPIO=" + String(gpio) + 
    " type=" + String(type) + " inverted=" + String(inverted) + 
    " min=" + String(min) + " max=" + String(max) + " channel=" + 
    String(channel);
}
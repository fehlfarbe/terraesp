#include <Threshold.h>

/**
 * @brief Construct a new Threshold:: Threshold object
 *
 * @param name Threshold name
 * @param sensor THSensor for data input
 * @param actuator Actuator to switch on/off
 * @param duration Duration for Actuator switch
 * @param threshold Actuator activation Threshold for Sensor value
 * @param greater_than true = Actuator active if Sensor value > threshold, false = Actuator active if Sensor value < threshold
 * @param inverted Is Actuator pin inverted?
 * @param gap Timespan between two activations
 * @param sensor_type Compare the threshold with temperature or humidity value
 */
Threshold::Threshold(String name, THSensor *sensor, Actuator *actuator,
                     float duration, float threshold, bool greater_than,
                     bool inverted, float gap, Threshold::SensorType sensor_type) : name(name), sensor(sensor), actuator(actuator), duration(duration), threshold(threshold), greater_than(greater_than), inverted(inverted), gap(gap), type(sensor_type)
{
}

Threshold::~Threshold() {

};

/**
 * @brief Compares current sensor value with threshold and activates Actuator
 *
 */
void Threshold::checkThreshold()
{
    if (!sensor)
    {
        debug.printf("No sensor configured for threshold %s\n", getName().c_str());
        return;
    }

    // get value
    float value = NAN;
    switch (type)
    {
    case SensorType::HUMIDITY:
        value = sensor->readLastHumidity();
        debug.printf("Threshold %s: sensor %s humidity %.2f\n", name.c_str(), sensor->getName().c_str(), value);
        break;
    case SensorType::TEMPERATURE:
        value = sensor->readLastTemperature();
        debug.printf("Threshold %s: sensor %s temperature %.2f\n", name.c_str(), sensor->getName().c_str(), value);
        break;
    default:
        debug.printf("Threshold %s: type %d UNKNOWN\n", name.c_str(), type);
        break;
    }
    if (isnan(value))
    {
        debug.printf("Threshold %s: sensor %s value is nan\n", name.c_str(), sensor->getName().c_str());
        return;
    }
    // compare value
    bool activate = false;
    if (greater_than)
    {
        activate = value > threshold;
    }
    else
    {
        activate = value < threshold;
    }
    debug.printf("Threshold %s: check if val %c thresh -> %.2f %c %.2f = %d\n",
                 name.c_str(),
                 greater_than ? '>' : '<',
                 value,
                 greater_than ? '>' : '<',
                 threshold,
                 activate);
    // check it
    if (!activate)
    {
        return;
    }
    debug.printf("Threshold %s: val %c thresh -> %.2f %c %.2f\n",
                 name.c_str(),
                 greater_than ? '>' : '<',
                 value,
                 greater_than ? '>' : '<',
                 threshold);

    // check gap
    float time_diff = abs((long)(millis() - last_activated)) / 1000;
    if (time_diff < gap)
    {
        debug.printf("Threshold %s: only %.2fs of %.2fs time gap reached\n", name.c_str(), time_diff, gap);
        return;
    }

    if (events)
    {
        String description = "Threshold " + name + " reached: " + value;
        description += greater_than ? " > " : " < ";
        description += threshold;
        events->add(Event(now(), Event::Type::THRESHOLD_REACHED, description), true);
    }
    this->activate();
}

/**
 * @brief Activate the Actuator
 *
 */
void Threshold::activate()
{
    if (!actuator)
    {
        return;
    }
    debug.printf("Threshold activate %s for %.2f seconds...\n", name.c_str(), duration);
    is_active = true;
    activated = millis();
    // digitalWrite(actuator->getGPIO(), inverted ? LOW : HIGH);
    actuator->setValue(HIGH);
}

/**
 * @brief Deactivate the Actuator
 *
 */
void Threshold::deactivate()
{
    if (is_active && actuator)
    {
        debug.printf("Threshold %s deactivate after %.2f seconds...\n", name.c_str(), duration);
        // digitalWrite(actuator->getGPIO(), inverted ? HIGH : LOW);
        actuator->setValue(LOW);
        is_active = false;
        last_activated = millis();
    }
}

/**
 * @brief Run this function in the main loop. It checks if a actuator is active and deactivates if the duration is over
 *
 */
void Threshold::update()
{
    if (is_active && abs((long)(millis() - activated)) / 1000.0 > duration)
    {
        this->deactivate();
    }
}

/**
 * @brief
 *
 * @return String Threshold name
 */
String Threshold::getName()
{
    return name;
}

/**
 * @brief
 *
 * @return float Threshold value
 */
float Threshold::getThreshold()
{
    return threshold;
}

/**
 * @brief
 *
 * @return true Actuator pins is inverted
 * @return false Actuator pin is not inverted
 */
bool Threshold::isInverted()
{
    return inverted;
}

/**
 * @brief
 *
 * @return true value > threshold -> activate Actuator
 * @return false value < threshold -> activate Actuator
 */
bool Threshold::isGreaterThan()
{
    return greater_than;
}

void Threshold::setEventList(RingBufCPP<Event, 100> *events)
{
    this->events = events;
}
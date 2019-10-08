#include <Threshold.h>

/**
 * @brief Construct a new Threshold:: Threshold object
 * 
 * @param name Threshold name
 * @param sensor THSensor for data input
 * @param button Button to switch on/off
 * @param duration Duration for Button switch
 * @param threshold Button activation Threshold for Sensor value
 * @param greater_than true = Button active if Sensor value > threshold, false = Button active if Sensor value < threshold
 * @param inverted Is Button pin inverted?
 * @param gap Timespan between two activations
 * @param sensor_type Compare the threshold with temperature or humidity value
 */
Threshold::Threshold(String name, THSensor* sensor, Button* button,
                     float duration, float threshold, bool greater_than,
                     bool inverted, float gap, Threshold::SensorType sensor_type) :
 name(name), sensor(sensor), button(button), duration(duration), threshold(threshold), greater_than(greater_than), inverted(inverted), gap(gap), type(sensor_type) {

}

Threshold::~Threshold(){

};

/**
 * @brief Compares current sensor value with threshold and activates Button
 * 
 */
void Threshold::checkThreshold(){
    if(!sensor){
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
    if(isnan(value)){
        debug.printf("Threshold %s: sensor %s value is nan\n", name.c_str(), sensor->getName().c_str());
        return;
    }
    // compare value
    bool activate = false;
    if(greater_than){
        activate = value > threshold;
    } else {
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
    if(!activate){
        return;
    }
    debug.printf("Threshold %s: val %c thresh -> %.2f %c %.2f\n",
                    name.c_str(),
                    greater_than ? '>' : '<',
                    value,
                    greater_than ? '>' : '<',
                    threshold);
    
    // check gap
    float time_diff = abs(millis()-last_activated) / 1000;
    if(time_diff < gap){
        debug.printf("Threshold %s: only %.2fs of %.2fs time gap reached", name.c_str(), time_diff, gap);
        return;
    }

    if(events){
        String description = "Threshold " + name + " reached: " + value;
        description += greater_than ? " > " : " < ";
        description += threshold;
        events->add(Event(now(), Event::Type::THRESHOLD_REACHED, description), true);
    }
    this->activate();
}

/**
 * @brief Activate the Button
 * 
 */
void Threshold::activate(){
    if(!button){
        return;
    }
    debug.printf("Threshold activate %s for %.2f seconds...\n", name.c_str(), duration);
    is_active = true;
    activated = millis();
    digitalWrite(button->pin, inverted ? LOW : HIGH);
}

/**
 * @brief Deactivate the Button
 * 
 */
void Threshold::deactivate(){
    if(is_active && button){
        debug.printf("Threshold %s deactivate after %.2f seconds...\n", name.c_str(), duration);
        digitalWrite(button->pin, inverted ? HIGH : LOW);
        is_active = false;
        last_activated = millis();
    }
}

/**
 * @brief Run this function in the main loop. It checks if a button is active and deactivates if the duration is over
 * 
 */
void Threshold::update(){
    if(is_active && abs(millis()-activated)/1000.0 > duration){
        this->deactivate();
    }
}

/**
 * @brief 
 * 
 * @return String Threshold name
 */
String Threshold::getName(){
    return name;
}

/**
 * @brief 
 * 
 * @return float Threshold value
 */
float Threshold::getThreshold(){
    return threshold;
}

/**
 * @brief
 * 
 * @return true Button pins is inverted
 * @return false Button pin is not inverted
 */
bool Threshold::isInverted(){
    return inverted;
}

/**
 * @brief 
 * 
 * @return true value > threshold -> activate Button
 * @return false value < threshold -> activate Button
 */
bool Threshold::isGreaterThan(){
    return greater_than;
}

void Threshold::setEventList(RingBufCPP<Event, 100> *events){
    this->events = events;
}
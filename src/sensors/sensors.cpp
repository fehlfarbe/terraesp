#include "sensors.h"

THSensor::THSensor() {
}

THSensor::THSensor(String name) : m_name(name) {
}

THSensor::~THSensor(){

}

float THSensor::readLastTemperature(){
    if(buffer_t.isEmpty())
        return 0.0;
    return *buffer_t.peek(buffer_t.numElements()-1);
}

float THSensor::readLastHumidity(){
    if(buffer_h.isEmpty())
        return 0.0;
    return *buffer_h.peek(buffer_h.numElements()-1);
}

bool THSensor::updateTH(){
    if(hasTemperature()){
        buffer_t.add(readTemperature(), true);
    }
    if(hasHumidity()){
        buffer_h.add(readHumidity(), true);
    }
    return true;
}

String THSensor::getName(){
    return this->m_name;
}

void THSensor::setEnabled(bool enabled){
    m_enabled = enabled;
}

bool THSensor::isEnabled(){
    return m_enabled;
}
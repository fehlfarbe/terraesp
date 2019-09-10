#include "sensors.h"

THSensor::THSensor() {
}

THSensor::THSensor(String name) : m_name(name) {
}

THSensor::~THSensor(){

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
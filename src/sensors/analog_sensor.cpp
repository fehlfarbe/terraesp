#include "analog_sensor.h"

AnalogSensor::AnalogSensor(String name, uint8_t pin, bool is_temp, bool is_humid){
    m_name = name;
    m_pin = pin;
    m_temperature = is_temp;
    m_humidity = is_humid;
}

float AnalogSensor::readTemperature(){
    if(hasTemperature()){
        return analogRead(m_pin);
    }
    return 0;
}

float AnalogSensor::readHumidity(){
    if(hasHumidity()){
        return analogRead(m_pin);
    }
    return 0;
}

bool AnalogSensor::hasTemperature(){
    return m_temperature;
}

bool AnalogSensor::hasHumidity(){
    return m_humidity;
}

bool AnalogSensor::running(){
    return true;
}
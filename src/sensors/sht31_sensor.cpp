#include "sht31_sensor.h"

SHT31Sensor::SHT31Sensor(String name, uint8_t address){
    m_name = name;
    m_running = sensor.begin(address);
}

float SHT31Sensor::readTemperature(){
    return sensor.readTemperature();
}

float SHT31Sensor::readHumidity(){
    return sensor.readHumidity();
}

bool SHT31Sensor::hasTemperature(){
    return true;
}

bool SHT31Sensor::hasHumidity(){
    return true;
}

bool SHT31Sensor::running(){
    return m_running;
}
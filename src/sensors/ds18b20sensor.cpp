#include "ds18b20_sensor.h"

DS18B20Sensor::DS18B20Sensor(String name, uint8_t pin, uint8_t idx){
    m_name = name;
    m_pin = pin;
    m_idx = idx;
    m_wire.begin(m_pin);
    m_sensors.setOneWire(&m_wire);
    m_sensors.begin();
}

float DS18B20Sensor::readTemperature(){
    updateValues();
    return m_temperature;
}

float DS18B20Sensor::readHumidity(){
    return 0;
}

bool DS18B20Sensor::hasTemperature(){
    return true;
}

bool DS18B20Sensor::hasHumidity(){
    return false;
}

bool DS18B20Sensor::running(){
    return true;
}

bool DS18B20Sensor::updateValues(){
    if(m_sensors.getDeviceCount() < m_idx+1){
        return false;
    }
    m_sensors.requestTemperatures(); 
    m_temperature = m_sensors.getTempCByIndex(m_idx);
    return true;
}
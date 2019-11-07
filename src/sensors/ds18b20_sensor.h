#ifndef DS18B20_SENSOR_H
#define DS18B20_SENSOR_H

#include "sensors.h"
#include "datatypes.h"
//#include <DHT.h>
// use Rob Tillaart's DHTlib because of NaN errors in Adafruit lib
#include <OneWire.h>
#include <DallasTemperature.h>

class DS18B20Sensor : public THSensor {
    public:
        DS18B20Sensor(String name, uint8_t pin, uint8_t idx=0);

        float readTemperature();
        float readHumidity();
        bool hasTemperature();
        bool hasHumidity();
        bool running();
    
    protected:
        bool updateValues();

        OneWire m_wire;
        DallasTemperature m_sensors;

        uint8_t m_pin;
        uint8_t m_idx;
        float m_temperature = 0;
        float m_humidity = 0;
};

#endif
#ifndef DHT_SENSOR_H
#define DHT_SENSOR_H

#include "sensors.h"
#include "datatypes.h"
//#include <DHT.h>
// use Rob Tillaart's DHTlib because of NaN errors in Adafruit lib
#include <dht.h>

class DHT11Sensor : public THSensor {
    public:
        DHT11Sensor(String name, uint8_t pin, uint8_t type);

        float readTemperature();
        float readHumidity();
        bool hasTemperature();
        bool hasHumidity();
        bool running();
    
    protected:
        bool updateValues();

        dht sensor;
        uint8_t m_pin;
        uint8_t m_type = UNKNOWN;
        float m_temperature = 0;
        float m_humidity = 0;
};

#endif
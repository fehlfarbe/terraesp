#ifndef ANALOG_SENSOR_H
#define ANALOG_SENSOR_H

#include "sensors.h"


class AnalogSensor : public THSensor {
    public:
        AnalogSensor(String name, uint8_t pin, bool is_temp, bool is_humid);

        float readTemperature();
        float readHumidity();
        bool hasTemperature();
        bool hasHumidity();
        bool updateTH();
        bool running();

    protected:
        uint8_t m_pin;
        bool m_humidity = false;
        bool m_temperature = false;
};

#endif
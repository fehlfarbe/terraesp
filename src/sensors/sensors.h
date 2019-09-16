#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>
#include <RingBufCPP.h>

#define SENSORS_BUFSIZE 288

class THSensor {
    public:
        THSensor();
        THSensor(String name);
        ~THSensor();

        virtual float readTemperature() = 0;
        virtual float readHumidity() = 0;
        float readLastTemperature();
        float readLastHumidity();
        virtual bool hasTemperature() = 0;
        virtual bool hasHumidity() = 0;
        bool updateTH();
        virtual bool running() = 0;

        String getName();
        void setEnabled(bool enabled);
        bool isEnabled();

        RingBufCPP<float, SENSORS_BUFSIZE> buffer_h;
        RingBufCPP<float, SENSORS_BUFSIZE> buffer_t;

    protected:
        String m_name = "null";
        bool m_running = false;
        bool m_enabled = true;
};

#endif
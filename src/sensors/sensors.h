#include <Arduino.h>
#include <RingBufCPP.h>

// Buffer size for 24h / 5 minutes
#define BUFSIZE 288

class THSensor {
    public:
        THSensor();
        THSensor(String name);
        ~THSensor();

        virtual float readTemperature();
        virtual float readHumidity();
        virtual bool hasTemperature();
        virtual bool hasHumidity();
        bool updateTH();
        virtual bool running();

        String getName();

        RingBufCPP<float, BUFSIZE> buffer_h;
        RingBufCPP<float, BUFSIZE> buffer_t;

    protected:
        String m_name = "null";
        bool m_running = false;
};
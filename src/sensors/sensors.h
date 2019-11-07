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

        enum class SENSOR_TYPE {
            UNKNOWN,
            DHT,
            SHT31,
            ANALOG_TEMPERATURE,
            ANALOG_HUMIDITY,
            ANALOG_TEMPERATURE_K,
            DS18B20
            // change last value in stringToSensor and getSensorTypes if you add an additional sensor
        };

        #define SENSOR_TYPE_SIZE 7

        static String sensorToString(SENSOR_TYPE t){
            switch(t){
                case SENSOR_TYPE::DHT:
                    return "DHT";
                case SENSOR_TYPE::SHT31:
                    return "SHT31";
                case SENSOR_TYPE::ANALOG_TEMPERATURE:
                    return "analog_t";
                case SENSOR_TYPE::ANALOG_HUMIDITY:
                    return "analog_h";
                case SENSOR_TYPE::ANALOG_TEMPERATURE_K:
                    return "analog_t_k";
                case SENSOR_TYPE::DS18B20:
                    return "DS18B20";
            }
            return "unknown";
        }

        static SENSOR_TYPE stringToSensor(String s) {
            int num_sensor_types = ((int)SENSOR_TYPE::DS18B20+1);
            // iterate over sensors and return type if name equal
            for(int i=(int)SENSOR_TYPE::UNKNOWN; i<num_sensor_types; i++){
                SENSOR_TYPE type = static_cast<SENSOR_TYPE>(i);
                if(s.equals(sensorToString(type)) == 0){
                    return type;
                }
            }
            return SENSOR_TYPE::UNKNOWN;
        }

    protected:
        String m_name = "null";
        bool m_running = false;
        bool m_enabled = true;
};

#endif
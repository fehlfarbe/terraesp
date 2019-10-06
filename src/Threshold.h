#ifndef THRESHOLD_H
#define THRESHOLD_H
#include <Arduino.h>
#include <datatypes.h>
#include <Debug.h>
#include <sensors/sensors.h>

class Threshold {
    public:
        enum SensorType {
            UNKNOWN,
            TEMPERATURE,
            HUMIDITY
        };

        Threshold(String name, THSensor* sensor, Button* button, 
                  float duration, float threshold, bool greater_than,
                  bool inverted, float gap, Threshold::SensorType sensor_type);
        ~Threshold();

        void checkThreshold();
        void update();

        String getName();
        float getThreshold();
        bool isInverted();
        bool isGreaterThan();

    private:
        void activate();
        void deactivate();

        Debug debug;

        String name;
        Button* button;
        THSensor* sensor;
        SensorType type;
        float duration;
        float threshold;
        bool greater_than = true;
        bool inverted = false;
        float gap;

        // save last activation time for gap
        long last_activated = 0;
        long activated = 0;
        bool is_active = false;
};

#endif

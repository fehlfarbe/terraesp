#ifndef TIMER_H
#define TIMER_H
#include <Arduino.h>
#include <datatypes.h>
#include <Debug.h>

class Timer {
    public:
        Timer(String name, Button *b, int onHour, int onMinute, int offHour, int offMinute, bool inverted);
        ~Timer();
        bool init(unsigned int time_min, OnTick_t timerCallback);
        bool isInitialized();
        bool checkAlarmId(AlarmId);

        // AlarmId getAlarmOnId();
        // AlarmId getAlarmOffId();

    private:
        void activate();
        void deactivate();

        Debug debug;
        String name;
        AlarmId alarmOnId = -1;
        AlarmId alarmOffId = -1;
        uint8_t pin;
        Button* button = nullptr;
        int alarmOnHour;
        int alarmOnMinute;
        int alarmOffHour;
        int alarmOffMinute;
        bool initialized = false;
        bool inverted = false;
};

#endif
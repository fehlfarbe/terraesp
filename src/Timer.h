#ifndef TIMER_H
#define TIMER_H
#include <Arduino.h>
#include <RingBufCPP.h>
#include <datatypes.h>
#include <Actuator.h>
#include <Event.h>
#include <Debug.h>

class Timer {
    public:
        Timer(String name, Actuator *a, int onHour, int onMinute, int offHour, int offMinute, bool inverted);
        ~Timer();
        bool init(unsigned int time_min, OnTick_t timerCallback);
        bool isInitialized();
        bool checkAlarmId(AlarmId);
        void setEventList(RingBufCPP<Event, 100> *events);

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
        Actuator* actuator = nullptr;
        int alarmOnHour;
        int alarmOnMinute;
        int alarmOffHour;
        int alarmOffMinute;
        bool initialized = false;
        bool inverted = false;

        RingBufCPP<Event, 100> *events = nullptr;
};

#endif
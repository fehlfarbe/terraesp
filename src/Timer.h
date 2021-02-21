#ifndef TIMER_H
#define TIMER_H
#include <Arduino.h>
#include <time.h>
#include <TimeLib.h>
#include <RingBufCPP.h>
#include <datatypes.h>
#include <Actuator.h>
#include <Event.h>
#include <Debug.h>

class Timer {
    public:
        Timer(String name, Actuator *a, uint8_t onHour, uint8_t onMinute, uint8_t offHour, uint8_t offMinute);
        ~Timer();
        bool init(const struct tm timeinfo);
        bool isInitialized();
        bool isTurnedOn();
        // bool checkAlarmId(AlarmId);
        bool update(const struct tm timeinfo);
        bool operator >(const Timer &b);
        bool operator >(const struct tm timeinfo);
        bool operator <(const struct tm timeinfo);
        int tmToSec(const struct tm t);
        int startToSec();
        int endToSec();
        

        void setEventList(RingBufCPP<Event, 100> *events);
        void setDebug(Debug& d);
        String toString();

    private:
        void activate();
        void deactivate();

        Debug debug;
        String name;
        uint8_t pin;
        Actuator* actuator = nullptr;
        uint8_t alarmOnHour;
        uint8_t alarmOnMinute;
        uint8_t alarmOffHour;
        uint8_t alarmOffMinute;
        unsigned int alarmOnInSeconds = 0;
        unsigned int alarmOffInSeconds = 0;
        bool initialized = false;
        bool turned_on = false;

        RingBufCPP<Event, 100> *events = nullptr;
};

#endif
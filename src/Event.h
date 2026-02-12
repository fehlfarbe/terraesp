#ifndef EVENT_H
#define EVENT_H
#include <Arduino.h>
#include <time.h>
#include <TimeLib.h>
#include <datatypes.h>
#include <Debug.h>

class Event
{

public:
    enum Type
    {
        UNKNOWN,
        BUTTON_TOGGLE,
        THRESHOLD_REACHED,
        TIMER_ON,
        TIMER_OFF
    };

    Event();
    Event(time_t time, Event::Type type, String description = "");
    String getEventString();

    time_t getTime();
    Event::Type getType();
    String getDescription();

private:
    time_t time;
    Event::Type type = Event::Type::UNKNOWN;
    String description;
};

#endif
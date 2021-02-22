#include <Event.h>

Event::Event() : time(now()), type(Event::Type::UNKNOWN){

};

Event::Event(time_t time, Event::Type type, String description) : time(time), type(type), description(description){

};

String Event::getEventString(){
    switch(type){
        case Event::Type::UNKNOWN:
            return String("UNKNOWN");
        case Event::Type::BUTTON_TOGGLE:
            return String("Button toggle");
        case Event::Type::THRESHOLD_REACHED:
            return String("Threshold reached toggle");
        case Event::Type::TIMER_ON:
            return String("Timer on");
        case Event::Type::TIMER_OFF:
            return String("Timer off");
            
    }

    return String("Unknown value: ") + type;
}


time_t Event::getTime(){
    return time;
}

Event::Type Event::getType(){
    return type;
}

String Event::getDescription(){
    return description;
}
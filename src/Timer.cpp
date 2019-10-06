#include <Timer.h>

/**
 * @brief Construct a new Timer:: Timer object
 * 
 * @param name Timer name
 * @param b Button object
 * @param onHour Timer start hour
 * @param onMinute Timer start minute
 * @param offHour Timer stop hour
 * @param offMinute Timer stop minute
 * @param inverted Invert Button pin
 */
Timer::Timer(String name, Button *b, int onHour, int onMinute, int offHour, int offMinute, bool inverted) : 
name(name), button(b), alarmOnHour(onHour), alarmOnMinute(onMinute), alarmOffHour(offHour), alarmOffMinute(offMinute), inverted(inverted) {

}

Timer::~Timer(){

}

/**
 * @brief Initializes the Timer
 * 
 * @param time_min current time of day in minutes
 * @param timerCallback callback function if Alarm/Timer gets triggered
 * @return true success
 * @return false error (button not available, ...)
 */
bool Timer::init(unsigned int time_min, OnTick_t timerCallback){
    debug.printf("Timer %s init on: %02d:%02d, off: %02d:%02d\n",
                 name.c_str(), alarmOnHour, alarmOnMinute, alarmOffHour, alarmOffMinute);
    if(!button){
        debug.printf("Timer %s: button is null\n", name.c_str());
        return false;
    }

    // setup timer
    alarmOnId = Alarm.alarmRepeat(alarmOnHour, alarmOnMinute, 0, timerCallback);
    alarmOffId = Alarm.alarmRepeat(alarmOffHour, alarmOffMinute, 0, timerCallback);

    // activate pin if we are in the timespan
    unsigned int alarm_on = alarmOnHour * 60 + alarmOnMinute;
    unsigned int alarm_off = alarmOffHour * 60 + alarmOffMinute;
    debug.printf("Timer %s: time: %d, alarm on: %d, alarm_off: %d\n", name.c_str(), time_min, alarm_on, alarm_off);
    if(alarm_on < alarm_off && alarm_on < time_min && time_min < alarm_off){
        this->activate();
    }
    else if(alarm_off < alarm_on && time_min < alarm_off){
        this->activate();
    }

    initialized = true;
    return initialized;
}

bool Timer::isInitialized(){
    return initialized;
}

/**
 * @brief Checks if the given AlarmId matches the on/off AlarmId and (de-)activates the given button
 * 
 * @param id AlarmId of the triggered Alarm (Alarm.getTriggeredAlarmId())
 * @return true AlarmId matches on/off AlarmId
 * @return false AlarmId does not match on/off AlarmId
 */
bool Timer::checkAlarmId(AlarmId id){
    if (alarmOnId == id) {
        this->activate();
        return true;
    }
    if (alarmOffId == id) {
        this->deactivate();
        return true;
    }
    return false;
}

/**
 * @brief Activates the button pin
 * 
 */
void Timer::activate(){
    debug.printf("Timer %s triggered button %s on\n", name.c_str(), button->name.c_str());
    digitalWrite(button->pin, inverted ? LOW : HIGH);
}

/**
 * @brief Deactivates the button pin
 * 
 */
void Timer::deactivate(){
    debug.printf("Timer %s triggered button %s off\n", name.c_str(), button->name.c_str());
    digitalWrite(button->pin, inverted ? HIGH : LOW);
}
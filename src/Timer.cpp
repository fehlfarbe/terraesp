#include <Timer.h>

/**
 * @brief Construct a new Timer:: Timer object
 * 
 * @param name Timer name
 * @param b Actuator object
 * @param onHour Timer start hour
 * @param onMinute Timer start minute
 * @param offHour Timer stop hour
 * @param offMinute Timer stop minute
 */
Timer::Timer(String name, Actuator *a, uint8_t onHour, uint8_t onMinute, uint8_t offHour, uint8_t offMinute) : name(name), actuator(a), alarmOnHour(onHour), alarmOnMinute(onMinute), alarmOffHour(offHour), alarmOffMinute(offMinute)
{
    alarmOnInSeconds = startToSec();
    alarmOffInSeconds = endToSec();
}

Timer::~Timer()
{
}

/**
 * @brief Initializes the Timer
 * 
 * @param time_min current time of day in minutes
 * @param timerCallback callback function if Alarm/Timer gets triggered
 * @return true success
 * @return false error (actuator not available, ...)
 */
bool Timer::init(const struct tm timeinfo)
{
    auto current = tmToSec(timeinfo);
    debug.printf("Timer %s init on: %02d:%02d, off: %02d:%02d\n",
                 name.c_str(), alarmOnHour, alarmOnMinute, alarmOffHour, alarmOffMinute);
    if (!actuator)
    {
        debug.printf("Timer %s: actuator is null\n", name.c_str());
        return false;
    }

    // setup timer
    // alarmOnId = Alarm.alarmRepeat(alarmOnHour, alarmOnMinute, 0, timerCallback);
    // alarmOffId = Alarm.alarmRepeat(alarmOffHour, alarmOffMinute, 0, timerCallback);

    // activate pin if we are in the timespan
    // unsigned int alarm_on = alarmOnHour * 60 + alarmOnMinute;
    // unsigned int alarm_off = alarmOffHour * 60 + alarmOffMinute;
    debug.printf("Timer %s: time: %d, alarm on: %d, alarm_off: %d\n", name.c_str(), current, alarmOnInSeconds, alarmOffInSeconds);
    // if (alarmOnInSeconds < alarmOffInSeconds && alarmOnInSeconds < current && current < alarmOnInSeconds)
    // {
    //     this->activate();
    // }
    // else if (alarm_off < alarm_on && (time_min < alarm_off || time_min > alarm_on))
    // {
    //     this->activate();
    // }
    initialized = true;

    // turn off on init
    deactivate();

    // check if actor should be turned on at start
    update(timeinfo);

    return initialized;
}

bool Timer::isInitialized()
{
    return initialized;
}

bool Timer::isTurnedOn(){
    return turned_on;
}

// /**
//  * @brief Checks if the given AlarmId matches the on/off AlarmId and (de-)activates the given actuator
//  * 
//  * @param id AlarmId of the triggered Alarm (Alarm.getTriggeredAlarmId())
//  * @return true AlarmId matches on/off AlarmId
//  * @return false AlarmId does not match on/off AlarmId
//  */
// bool Timer::checkAlarmId(AlarmId id)
// {
//     if (alarmOnId == id)
//     {
//         this->activate();
//         return true;
//     }
//     if (alarmOffId == id)
//     {
//         this->deactivate();
//         return true;
//     }
//     return false;
// }

/**
 * @brief Check if timer should be turned on/off
 * 
 * @return true if actuator state changed
 * @return false if nothing happened
 */
bool Timer::update(struct tm timeinfo){
    auto current = tmToSec(timeinfo);
    auto start = startToSec();
    auto end = endToSec();
    bool reversed = end < start;
    bool in_range = (!reversed && (start <= current && current < end)) || (reversed && (current >= start || current < end));

    // debug.printf("%s: current=%d start=%d end=%d in_range=%d\n", name.c_str(), current, start, end, in_range);
    // debug.println();

    // check if timer on should be triggered
    if(!isTurnedOn() && in_range){
        activate();
        return true;
    } else if(isTurnedOn() && !in_range){
        deactivate();
        return true;
    }

    return false;
}

/**
 * @brief Activates the actuator pin
 * 
 */
void Timer::activate()
{
    if(!initialized){
        debug.printf("Timer %s isnt initalized, cannot turn on actor", name.c_str());
    }

    if (this->events)
    {
        String description = "Timer " + name + " ON at ";
        description += alarmOnHour + String(":") + alarmOnMinute;
        this->events->add(Event(now(), Event::Type::TIMER_ON, description), true);
    }
    debug.printf("Timer %s triggered actuator %s on\n", name.c_str(), actuator->getName().c_str());
    // digitalWrite(actuator->getGPIO(), inverted ? LOW : HIGH);
    actuator->setValue(HIGH);
    turned_on = true;
}

/**
 * @brief Deactivates the actuator pin
 * 
 */
void Timer::deactivate()
{
    if(!initialized){
        debug.printf("Timer %s isnt initalized, cannot turn off actor", name.c_str());
    }

    // if(!turned_on){
    //     debug.printf("Timer %s isnt activated, cannot turn off actor", name.c_str());
    //     return;
    // }

    if (this->events)
    {
        String description = "Timer " + name + " OFF at ";
        description += alarmOffHour + String(":") + alarmOffMinute;
        this->events->add(Event(now(), Event::Type::TIMER_OFF, description), true);
    }
    debug.printf("Timer %s triggered actuator %s off\n", name.c_str(), actuator->getName().c_str());
    // digitalWrite(actuator->getGPIO(), inverted ? HIGH : LOW);
    actuator->setValue(LOW);
    turned_on = false;
}

int Timer::tmToSec(const struct tm t){
    return (t.tm_hour * 3600) + (t.tm_min * 60) + t.tm_sec;
}

int Timer::startToSec(){
    return alarmOnHour*3600 + alarmOnMinute*60;
}

int Timer::endToSec(){
    return alarmOffHour*3600 + alarmOffMinute*60;
}

void Timer::setEventList(RingBufCPP<Event, 100> *events)
{
    this->events = events;
}

void Timer::setDebug(Debug &d)
{
    debug = d;
}

String Timer::toString()
{
    String s = "Timer " + name + " on: " + alarmOnHour + ":" + alarmOnMinute + ", off: " + alarmOffHour + ":" + alarmOffMinute +
           " initialized: " + initialized + " activated: " + isTurnedOn();
    if(actuator){
        s += " actuator: " + actuator->toString();
    }
    return s;
}
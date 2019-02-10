#include <TimeAlarms.h>

struct AlarmSettings {
  String name;
  AlarmId alarmOnId = -1;
  AlarmId alarmOffId = -1;
  uint8_t pin;
  int alarmOnHour;
  int alarmOnMinute;
  int alarmOffHour;
  int alarmOffMinute;
  bool init = false;
  bool inverted = false;
};

struct TimeSettings {
    bool dst = false;
    int gmt_offset_sec = 0;
};

enum DHTType {
    UNKNOWN,
    DHT11,
    DHT21,
    DHT22,
    AM2302,
    AM2301
};

//
//enum ThreshAction {
//  ON,
//  OFF
//};
//
//enum ThreshType {
//  MIN,
//  MAX
//};
//
//struct ThreshSettings {
//  String name;
//  int pin;
//  ThreshAction action;
//  int duration;
//  float value;
//  ThreshType type;
//};

struct RainSettings {
    uint8_t pin;
    int duration;
    float threshold;
    bool initialized = false;
    bool inverted = false;
};

enum ButtonType {
    BTN_TOGGLE=0,
    BTN_SLIDER=1,
    BTN_INPUT=2
};

struct Button {
    String name;
    uint8_t pin;
    ButtonType type;
    bool inverted = false;
    int min = 0;
    int max = 255;
    uint8_t channel = 0;
};
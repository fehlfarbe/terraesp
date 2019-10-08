#ifndef DATATYPES_H
#define DATATYPES_H
#include <TimeAlarms.h>


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

#endif
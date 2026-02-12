#ifndef DATATYPES_H
#define DATATYPES_H

struct TimeSettings
{
    bool dst = false;
    int gmt_offset = 0;
};

enum class DHTType
{
    UNKNOWN,
    DHT11,
    DHT21,
    DHT22,
    AM2302,
    AM2301
};

enum LEDState
{
    NONE,
    ERROR,
    WIFI_STATE_CONNECTING,
    WIFI_STATE_CONNECTED,
    WIFI_STATE_DISCONNECTED,
    WIFI_STATE_AP,
    OTA_ACTIVE,
    READ_SENSORS
};

#endif
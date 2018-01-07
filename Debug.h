//
// Created by kolbe on 07.01.18.
//

#ifndef TERRAESP_DEBUG_H
#define TERRAESP_DEBUG_H

#if ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include <WiFi.h>


class Debug : public HardwareSerial {

public:
    Debug(int uart_nr, WiFiServer& server, WiFiClient& client);

#if ARDUINO >= 100
    size_t write(uint8_t byte);
#else
    void write(uint8_t byte);
#endif

private:
    WiFiServer telnetServer;
    WiFiClient telnetClient;
};


#endif //TERRAESP_DEBUG_H

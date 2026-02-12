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

class Debug : public Stream
{

public:
    Debug();
    Debug(WiFiServer *server, WiFiClient *client);

    void setTelnet(WiFiServer *server, WiFiClient *client);

    virtual int available(void)
    {
        return Serial.available();
    };
    virtual int peek(void)
    {
        return Serial.peek();
    };
    virtual int read(void)
    {
        return Serial.read();
    };
    virtual int availableForWrite(void)
    {
        return 0;
    };
    virtual void flush(void)
    {
        return;
    };
    virtual size_t write(uint8_t);
    using Print::write; // pull in write(str) and write(buf, size) from Print
    // #if ARDUINO >= 100
    //     size_t write(uint8_t byte);
    // #else
    //     void write(uint8_t byte);
    // #endif

private:
    WiFiServer *telnetServer = nullptr;
    WiFiClient *telnetClient = nullptr;
};

#endif // TERRAESP_DEBUG_H

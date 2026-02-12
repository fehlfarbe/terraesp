//
// Created by kolbe on 07.01.18.
//

#include "Debug.h"

Debug::Debug()
{
}

Debug::Debug(WiFiServer *server, WiFiClient *client)
{
    setTelnet(server, client);
}

void Debug::setTelnet(WiFiServer *server, WiFiClient *client)
{
    telnetServer = server;
    telnetClient = client;
}

size_t Debug::write(uint8_t c)
{
    Serial.write(c);
    if (telnetClient && telnetClient->connected())
    {
        telnetClient->write(c);
    }
    return 1;
}

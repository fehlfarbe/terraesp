//
// Created by kolbe on 07.01.18.
//

#include "Debug.h"

Debug::Debug(int uart_nr, WiFiServer& server, WiFiClient& client) : HardwareSerial(uart_nr) {
    telnetServer = server;
    telnetClient = client;
    telnetServer.begin();
    telnetServer.setNoDelay(true);
}


#if ARDUINO >= 100
size_t Debug::write(uint8_t c){
    HardwareSerial::write(c);
    if (telnetClient && telnetClient.connected()){
        telnetServer.write(c);
    }
    return 1;
}
#else
void Debug::write(uint8_t c){
    HardwareSerial.write(c);
    if (telnetClient && telnetClient.connected()){
        telnetServer.write(c);
    }
}
#endif

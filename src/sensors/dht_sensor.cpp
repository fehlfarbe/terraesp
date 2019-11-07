#include "dht_sensor.h"

DHT11Sensor::DHT11Sensor(String name, uint8_t pin, DHTType type){
    m_name = name;
    m_pin = pin;
    m_type = type;
}

float DHT11Sensor::readTemperature(){
    updateValues();
    return m_temperature;
}

float DHT11Sensor::readHumidity(){
    updateValues();
    return m_humidity;
}

bool DHT11Sensor::hasTemperature(){
    return true;
}

bool DHT11Sensor::hasHumidity(){
    return true;
}

bool DHT11Sensor::running(){
    return true;
}

bool DHT11Sensor::updateValues(){
    if (m_type != DHTType::UNKNOWN){
        // get new values (try up to 10 times if NaN or incorrect values)
        for(size_t i=0; i<10; i++){
            int chk = -1;
            switch (m_type){
                case DHTType::DHT11:
                    chk = sensor.read11(m_pin);
                    break;
                case DHTType::DHT21:
                    chk = sensor.read21(m_pin);
                    break;
                case DHTType::DHT22:
                    chk = sensor.read22(m_pin);
                    break;
                case DHTType::AM2301:
                    chk = sensor.read2301(m_pin);
                    break;
                case DHTType::AM2302:
                    chk = sensor.read2302(m_pin);
                    break;
                default:
                    chk = -1;
            }
            // switch (chk)
            // {
            //     case DHTLIB_OK:
            //         debug.print("OK,\t");
            //         break;
            //     case DHTLIB_ERROR_CHECKSUM:
            //         debug.print("Checksum error,\t");
            //         break;
            //     case DHTLIB_ERROR_TIMEOUT:
            //         debug.print("Time out error,\t");
            //         break;
            //     case DHTLIB_ERROR_CONNECT:
            //         debug.print("Connect error,\t");
            //         break;
            //     case DHTLIB_ERROR_ACK_L:
            //         debug.print("Ack Low error,\t");
            //         break;
            //     case DHTLIB_ERROR_ACK_H:
            //         debug.print("Ack High error,\t");
            //         break;
            //     default:
            //         debug.print("Unknown error,\t");
            //         break;
            // }

            // test for NaN values and filter incorrect values
            if (chk == DHTLIB_OK) {
                m_temperature = sensor.temperature;
                m_humidity = sensor.humidity;
                return true;
            }
        }
    }
    return false;
}
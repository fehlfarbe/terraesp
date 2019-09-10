#include <Adafruit_SHT31.h>
#include "sensors.h"


class SHT31Sensor : public THSensor {
    public:
        SHT31Sensor(String name, uint8_t address);

        float readTemperature();
        float readHumidity();
        bool hasTemperature();
        bool hasHumidity();
        bool running();
    
    protected:
        Adafruit_SHT31 sensor;
};
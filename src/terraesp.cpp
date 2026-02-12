#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <HTTPClient.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncWiFiManager.h>
#include <time.h>
#include <TimeLib.h>
#include <FS.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <Array.h>
#include <RingBufHelpers.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <NeoPixelBus.h>
#include <RingBufCPP.h>
#include "Debug.h"

// #define MOCKUP_DATA
// #define LED_BUILTIN 2

// NeoPixel status
#define LED_STATUS_NEO 13

// Buffer size for 24h / 5 minutes
#define MAX_ELEMENTS 50
#define SENSORS_PAUSE_S 120

#include "sensors/sensors.h"
#include "sensors/analog_sensor.h"
#include "sensors/dht_sensor.h"
#include "sensors/sht31_sensor.h"
#include "sensors/ds18b20_sensor.h"
#include "Threshold.h"
#include "ActuatorToggle.h"
#include "ActuatorPWM.h"
#include "Timer.h"
#include "Event.h"
#include "datatypes.h"
#include "utils.h"

// status LED
// Adafruit_NeoPixel statusLED(1, LED_STATUS_NEO, NEO_GRB + NEO_KHZ800);
NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> statusLED(1, LED_STATUS_NEO);
LEDState statusLEDState = LEDState::NONE;

//WebServer
AsyncWebServer server(80);
DNSServer dns;
File fsUploadFile;

// WiFi
AsyncWiFiManager wifiManager(&server, &dns);

// settings
// String ssid = "";
// String password = "";
String host = "terraesp";
// String ap_ssid = "terraesp";
// bool ap_mode = false;
TimeSettings settings_time;
unsigned long last_sensors = 0;
Array<Threshold *, MAX_ELEMENTS> thresholds;
Array<Actuator *, MAX_ELEMENTS> actuators;
Array<Timer *, MAX_ELEMENTS> timers;
RingBufCPP<Event, 100> events;

// sensors
Array<THSensor *, MAX_ELEMENTS> sensors;
RingBufCPP<time_t, SENSORS_BUFSIZE> buffer_time;

// parallel task
TaskHandle_t pTask;

// Telnet
Debug debug;
WiFiServer telnetServer(23);
WiFiClient telnetClient;

/**************************
 * DECLARATIONS
 *************************/
THSensor *getSensor(String name);
Actuator *getActuator(String name);
void readSensors();
time_t getNtpTime();
bool initTimers();

// www
void handleActuratorChange(AsyncWebServerRequest *request);
void handleActurators(AsyncWebServerRequest *request);
void handleSensordata(AsyncWebServerRequest *request);
void handleSensordataCurrent(AsyncWebServerRequest *request);
void handleSensors(AsyncWebServerRequest *request);
void handleSensorTypes(AsyncWebServerRequest *request);
void handleConfig(AsyncWebServerRequest *request);

void initOTA();
void loadSettings(fs::FS &fs);
void setLEDState(LEDState state);
void updateLEDState();

/**************************
 * DEFINITIONS
 *************************/
void setup()
{

    // start debug output
    Serial.begin(115200);

    // setup pins
    // pinMode(LED_BUILTIN, OUTPUT);
    // digitalWrite(LED_BUILTIN, HIGH);
    // digitalWrite(LED_BUILTIN, LOW);

    // status LED
    statusLED.Begin();
    setLEDState(LEDState::WIFI_STATE_CONNECTING);

    // WiFi and HTTP
    wifiManager.setConnectTimeout(10);
    if (!wifiManager.autoConnect())
    {
        Serial.println("failed to connect, we should reset as see if it connects");
        setLEDState(LEDState::ERROR);
    }

    // setup mDNS
    char hostname[20];
    uint64_t chipid = ESP.getEfuseMac();
    sprintf(hostname, "esp32-%04X%08X", (uint16_t)(chipid >> 32), (uint32_t)chipid);
    if (!MDNS.begin(hostname))
    {
        Serial.println("Error setting up MDNS responder!");
    }
    else
    {
        Serial.println("mDNS responder started");
    }

    // init OTA update
    // @todo: activate only if jumper is set
    initOTA();

    // init File system and show content
    if (!LittleFS.begin(false))
    {
        setLEDState(LEDState::ERROR);
        while (true)
        {
            debug.println("Failed to mount file system");
            delay(1000);
        }
    }
    listDir(LittleFS, "/", 0);

    // settings
    loadSettings(LittleFS);

    // sync time
    setSyncProvider(getNtpTime);

    // init timers
    while (!initTimers())
    {
        delay(100);
    }

    // timer for sensors, read new sensor values every 120s
    // Alarm.timerRepeat(120, readSensors);
    readSensors();

    // HTTP handles
    server.begin();
    server.serveStatic("/", LittleFS, "/www/");

    // API
    server.on("/api/actuators", HTTP_GET, handleActurators);
    server.on("/api/actuator/change", HTTP_POST, handleActuratorChange);
    server.on("/api/sensors/all", HTTP_GET, handleSensordata);
    server.on("/api/sensors/current", HTTP_GET, handleSensordataCurrent);
    server.on("/api/config", handleConfig);
    server.on("/reboot", [](AsyncWebServerRequest* r) { // return current device time
        String time = String(now());
        r->send(200, "text/json", "{\"answer\": \"ok\"}");
        delay(100);
        ESP.restart();
    });
    server.on("/api/time", [](AsyncWebServerRequest* r) { // return current device time
        String time = String(now());
        r->send(200, "text/json", "{\"time\": \"" + time + "\"}");
    });
    server.on("/api/stats", [](AsyncWebServerRequest* r) { // return current device time
        r->send(200, "text/json", "{\"free_heap\": \"" + String(ESP.getFreeHeap()) + "\", "
                                      "\"chip_rev\" : \"" + ESP.getChipRevision() + "\", "
                                      "\"sdk\" : \"" + ESP.getSdkVersion() + "\", "
                                      "\"wifi\" : \"" + WiFi.RSSI() + "\", "
                                      "\"LITTLEFS_used\" : \"" + LittleFS.usedBytes() + "\", "
                                      "\"cpu_freq\" : \"" + ESP.getCpuFreqMHz() + "\"}"
                                      );
    });

    // setup parallel task
    // xTaskCreatePinnedToCore(
    //   parallelTask, /* Function to implement the task */
    //   "task2", /* Name of the task */
    //   10000,  /* Stack size in words */
    //   NULL,  /* Task input parameter */
    //   1,  /* Priority of the task */
    //   &pTask,  /* Task handle. */
    //   0); /* Core where the task should run */

    // init telnetServer
    debug.setTelnet(&telnetServer, &telnetClient);
    telnetServer.begin();
    telnetServer.setNoDelay(true);

    setLEDState(LEDState::NONE);

#ifdef MOCKUP_DATA
    debug.print("Creating mockup data...");
    time_t t = now();
    for (size_t n = 0; n < SENSORS_BUFSIZE; n++)
    {
        buffer_time.add(t++, true);
        for (size_t i = 0; i < sensors.size(); i++)
        {
            THSensor *s = sensors.get(i);
            if (s->isEnabled())
            {
                sensors.get(i)->updateTH();
                digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
            }
        }
    }
    debug.println("done!");
#endif
}

void loop()
{
    // check WiFi an reconnect if necessary
    if (WiFi.status() != WL_CONNECTED)
    {
        debug.println(WiFi.status());
        setLEDState(LEDState::WIFI_STATE_DISCONNECTED);
        wifiManager.autoConnect();
    }

    //debug.printf("WiFi state: %d\n", WiFi.status());

    // handle OTA updates
    ArduinoOTA.handle();

    // sync time
    if (timeStatus() == timeNotSet)
    {
        debug.println("Waiting for synced time");
    }

    // add new telnet Clients
    if (telnetServer.hasClient())
    {
        if (!telnetClient || !telnetClient.connected())
        {
            if (telnetClient)
            {
                telnetClient.stop();
                debug.println("Telnet Client Stop");
            }
            telnetClient = telnetServer.accept();
            debug.println("New Telnet client");
            telnetClient.clear();
            telnetClient.println("Welcome to TerraESP");

            // print config
            File dataFile = LittleFS.open("/config.json", FILE_READ);
            String json = dataFile.readString();
            dataFile.close();
            debug.println(json);

            // print NTP
            getNtpTime();

            // debug.println("Timers:");
            // for (size_t i = 0; i < timers.size(); i++)
            // {
            //     debug.println(timers[i]->toString());
            // }
            
            // debug.println("Thresholds:");
            // for (size_t i = 0; i < thresholds.size(); i++)
            // {
            //     debug.println(thresholds[i]->getName());
            // }
        }
    }

    // read current sensor values
    auto current = millis();
    if( (current - last_sensors) / 1000. >= SENSORS_PAUSE_S){
        readSensors();
        last_sensors = current;
        debug.printf("Free heap %d min free heap %d stack %d\n", ESP.getFreeHeap(), ESP.getMinFreeHeap(), uxTaskGetStackHighWaterMark(NULL));
    }

    // update thresholds -> deactivates thresholds after duration
    for (size_t i = 0; i < thresholds.size(); i++)
    {
        thresholds[i]->update();
    }

    // delay, handle alarms/sensors and so on
    // Alarm.delay(10);
    
    // handle timers
    struct tm timeinfo;
    if (getLocalTime(&timeinfo))
    {
        for(size_t i=0; i<timers.size(); i++){
            timers[i]->update(timeinfo);            
        }
    } else {
        debug.println("Failed to obtain time");
    }

    delay(100);
}

// void parallelTask(void *parameter)
// {
//     while (true)
//     {
//         // Serial.println(ESP.getFreeHeap());
//         delay(100);
//     }
// }

/**
 * @brief load settings an initialize sensors, buttons, timers, thresholds, ...
 * 
 * @param fs FileSystem
 */
void loadSettings(fs::FS &fs)
{
    File dataFile = fs.open("/config.json", FILE_READ);
    String json = dataFile.readString();
    dataFile.close();
    debug.println(json);
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, json);
    if (error)
    {
        debug.println("----- parseObject() for config.json failed -----");
        debug.println(error.c_str());
        return;
    }
    else
    {
        // load sensor settings
        JsonArray sensorsJSON = doc["sensors"];
        for (auto s : sensorsJSON)
        {
            debug.print("Got new sensor ");
            debug.print(s["type"].as<String>());
            debug.print(" on pin/address ");
            debug.print(s["gpio"].as<String>());
            debug.println();
            String type((const char *)s["type"]);
            type.toLowerCase();
            THSensor *thsensor = nullptr;
            if (type.startsWith("dht") || type.startsWith("am23"))
            {
                // DHTType dht_type = DHTType::UNKNOWN;
                uint8_t dht_type = 0;
                if (type == "dht22" || type == "am2302" || type == "am2321"){
                    dht_type = DHT22;
                }
                else if(type == "dht11") {
                    dht_type = DHT11;
                }
                else if(type == "dht21" || type == "am2301"){
                    dht_type = DHT21;
                }
                // if (type == "dht22")
                // {
                //     dht_type = DHTType::DHT22;
                // }
                // else if (type == "am2302")
                // {
                //     dht_type = DHTType::AM2302;
                // }
                // else if (type == "dht21")
                // {
                //     dht_type = DHTType::DHT21;
                // }
                // else if (type == "dht11")
                // {
                //     dht_type = DHTType::DHT11;
                // }
                // else if (type == "dht2301")
                // {
                //     dht_type = DHTType::AM2301;
                // }
                // if (dht_type != DHTType::UNKNOWN)
                // {
                //     thsensor = new DHT11Sensor(s["name"], s["gpio"], dht_type);
                // }
                if (dht_type != 0)
                {
                    thsensor = new DHT11Sensor(s["name"], s["gpio"], dht_type);
                }
            }
            else if (type == "sht31")
            {
                thsensor = new SHT31Sensor(s["name"], s["gpio"]);
            }
            else if (type == "ds18b20")
            {
                thsensor = new DS18B20Sensor(s["name"], s["gpio"], s["idx"].is<JsonVariant>() ? s["idx"] : 0);
            }
            else if (type == "analog_t")
            {
                thsensor = new AnalogSensor(s["name"], s["gpio"], true, false);
            }
            else if (type == "analog_h")
            {
                thsensor = new AnalogSensor(s["name"], s["gpio"], false, true);
            }
            else
            {
                debug.println(" unknown");
            }

            if (thsensor)
            {
                if (!s["enabled"].isNull())
                {
                    thsensor->setEnabled(s["enabled"]);
                }
                sensors.push_back(thsensor);
            }
        }

        // load actuators
        JsonArray actuatorsJSON = doc["actuators"];
        for (auto a : actuatorsJSON)
        {
            String name = a["name"];
            uint16_t gpio = (uint16_t)a["gpio"];
            bool inverted = a["inverted"];
            uint16_t minVal = a["min"];
            uint16_t maxVal = a["max"];

            if(strcmp(a["type"], "toggle") == 0){
                actuators.push_back((Actuator*)new ActuatorToggle(name, gpio, inverted));
            } else {
                actuators.push_back((Actuator*)new ActuatorPWM(name, gpio, minVal, maxVal));
            }
            debug.println("Got new " + actuators[actuators.size()-1]->toString());

        }

        // load timer settings
        JsonArray timerJSON = doc["timers"];
        for (auto t : timerJSON)
        {
            Actuator *actuator = getActuator(t["actuator"].as<String>());
            if (actuator)
            {
                Timer *timer = new Timer(t["name"].as<String>(),
                                         actuator,
                                         split(String((const char *)t["on"]), ':', 0).toInt(),
                                         split(String((const char *)t["on"]), ':', 1).toInt(),
                                         split(String((const char *)t["off"]), ':', 0).toInt(),
                                         split(String((const char *)t["off"]), ':', 1).toInt());
                timer->setEventList(&events);
                timer->setDebug(debug);
                timers.push_back(timer);
            }
        };

        // load thresholds
        JsonArray thresholdsJSON = doc["thresholds"];
        for (auto s : thresholdsJSON)
        {
            THSensor *sensor = getSensor(s["sensor"].as<String>());
            Actuator *actuator = getActuator(s["actuator"].as<String>());
            // if sensor and actuator exist create threshold
            if (sensor && actuator)
            {
                Threshold::SensorType sensor_type = Threshold::SensorType::UNKNOWN;
                if (s["sensor_type"].as<String>() == "humidity")
                {
                    sensor_type = Threshold::SensorType::HUMIDITY;
                }
                else if (s["sensor_type"].as<String>() == "temperature")
                {
                    sensor_type = Threshold::SensorType::TEMPERATURE;
                }
                else
                {
                    debug.printf("Threshold %s sensor type %s is unknown\n", s["name"].as<String>().c_str(), s["sensor_type"].as<String>().c_str());
                }

                bool greater_than = false;
                if (s["comparator"].as<String>() == ">")
                {
                    greater_than = true;
                }

                Threshold *thresh = new Threshold(s["name"].as<String>(),
                                                  sensor,
                                                  actuator,
                                                  s["duration"],
                                                  s["threshold"],
                                                  greater_than,
                                                  s["inverted"],
                                                  s["gap"],
                                                  sensor_type);
                thresh->setEventList(&events);
                thresholds.push_back(thresh);
            }
        }

        // general settings
        JsonObject generalJSON = doc["general"];
        settings_time.dst = (bool)generalJSON["dst"];                  // daylight saving time
        settings_time.gmt_offset = (int)generalJSON["gmt_offset"]; // gmt offset
    }
}

/**************************************
 * 
 * OTA Update
 * 
 *************************************/

/**
 * @brief Inits the OTA Update
 * 
 */
void initOTA()
{
    // Port defaults to 3232
    // ArduinoOTA.setPort(3232);

    // Hostname defaults to esp3232-[MAC]
    ArduinoOTA.setHostname(host.c_str());

    // Password can be set with it's md5 value as well
    // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
    ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

    ArduinoOTA.onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH)
        {
            type = "sketch";
        }
        else
        {
            // U_LITTLEFS
            type = "filesystem";
        }

        // NOTE: if updating LITTLEFS this would be the place to unmount LITTLEFS using LITTLEFS.end()
        debug.println("Start updating " + type);
    });
    ArduinoOTA.onEnd([]() {
        debug.println("\nEnd");
        ESP.restart();
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        // setLEDState(LEDState::OTA_ACTIVE);
        RgbColor updatedColor = RgbColor::LinearBlend(RgbColor(50, 0, 0), RgbColor(0, 50, 0), (progress / (float)total));
        statusLED.SetPixelColor(0, updatedColor);
        statusLED.Show();
        debug.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
        debug.printf("Error[%u]: ", error);
        setLEDState(LEDState::ERROR);
        if (error == OTA_AUTH_ERROR)
            debug.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR)
            debug.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR)
            debug.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR)
            debug.println("Receive Failed");
        else if (error == OTA_END_ERROR)
            debug.println("End Failed");
    });
    ArduinoOTA.begin();
    debug.println("OTA update ready");
}

/**************************************
 * 
 * HTTP handler
 * 
 *************************************/
/**
 * @brief Checks if path exists
 * 
 * @param path URL
 * @return true exists
 * @return false does not exist
 */
bool exists(String path)
{
    bool exists = false;
    File file = LittleFS.open(path, FILE_READ);
    if (!file.isDirectory())
    {
        exists = true;
    }
    file.close();
    return exists;
}

/**
 * @brief Sends buffered timestamps and sensor value for all sensors as JSON object for charts, ...
 * 
 */
void handleSensordata(AsyncWebServerRequest *request)
{
    JsonDocument doc;
    JsonArray sensor_array = doc["sensors"].to<JsonArray>();
    JsonArray thresholds_array = doc["thresholds"].to<JsonArray>();
    JsonArray events_array = doc["events"].to<JsonArray>();
    JsonArray time_array = doc["time"].to<JsonArray>();

    for (size_t i = 0; i < buffer_time.numElements(); i++)
    {
        time_array.add(*buffer_time.peek(i));
    }
    for (size_t i = 0; i < sensors.size(); i++)
    {
        THSensor *s = sensors[i];
        if (s->isEnabled())
        {
            JsonObject sensor = sensor_array.add<JsonObject>();
            sensor["name"] = s->getName();
            if (s->hasTemperature())
            {
                JsonArray t = sensor["temperature"].to<JsonArray>();
                for (size_t j = 0; j < s->buffer_t.numElements(); j++)
                {
                    t.add(*s->buffer_t.peek(j));
                }
            }
            if (s->hasHumidity())
            {
                JsonArray h = sensor["humidity"].to<JsonArray>();
                for (size_t j = 0; j < s->buffer_h.numElements(); j++)
                {
                    h.add(*s->buffer_h.peek(j));
                }
            }
        }
    }
    for (size_t i = 0; i < thresholds.size(); i++)
    {
        Threshold *thresh = thresholds[i];
        JsonObject t = thresholds_array.add<JsonObject>();
        t["name"] = thresh->getName();
        t["threshold"] = thresh->getThreshold();
        t["greater_than"] = thresh->isGreaterThan();
    }
    for (size_t i = 0; i < events.numElements(); i++)
    {
        Event *event = events.peek(i);
        JsonObject e = events_array.add<JsonObject>();
        e["time"] = event->getTime();
        e["type"] = event->getType();
        e["name"] = event->getEventString();
        e["description"] = event->getDescription();
    }

    debug.printf("Free heap %d min free heap %d stack %d\n", ESP.getFreeHeap(), ESP.getMinFreeHeap(), uxTaskGetStackHighWaterMark(NULL));
    debug.printf("JSON doc size: %d overflowed: %d\n", doc.size(), doc.overflowed());
    debug.flush();

    // send JSON response
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    // serializeJson(doc, debug);
    serializeJson(doc, *response);
    request->send(response);
}

void handleSensordataCurrent(AsyncWebServerRequest *request)
{
    JsonDocument doc;
    JsonArray sensor_array = doc["sensors"].to<JsonArray>();
    
    for (size_t i = 0; i < sensors.size(); i++)
    {
        THSensor *s = sensors[i];
        if (s->isEnabled())
        {
            JsonObject sensor = doc.add<JsonObject>();;
            sensor["name"] = s->getName();
            if (s->hasTemperature())
            {
                sensor["temperature"] = s->readTemperature();
            }
            if (s->hasHumidity())
            {
                sensor["humidity"] = s->readHumidity();
            }
        }
    }

    // send JSON response
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    // serializeJson(doc, debug);
    serializeJson(doc, *response);
    request->send(response);
}

/**
 * @brief Reads current sensor values and returns sensor name, values and current time as JSON object
 * 
 */
void handleSensors(AsyncWebServerRequest *request)
{
    JsonDocument doc;
    JsonArray sensor_array = doc["sensors"].to<JsonArray>();

    for (size_t i = 0; i < sensors.size(); i++)
    {
        THSensor *s = sensors[i];
        if (s->isEnabled())
        {
            JsonObject sensor = sensor_array.add<JsonObject>();
            sensor["name"] = s->getName();
            if (s->hasTemperature())
            {
                sensor["temperature"] = s->readTemperature();
            }
            if (s->hasHumidity())
            {
                sensor["humidity"] = s->readHumidity();
            }
        }
    }

    doc["time"] = now();

    // send JSON response
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    serializeJson(doc, debug);
    serializeJson(doc, *response);
    request->send(response);
}

/**
 * @brief returns all supported sensor types as JSON object
 * 
 */
void handleSensorTypes(AsyncWebServerRequest *request)
{
    JsonDocument doc;
    JsonArray sensor_types = doc["sensor_types"].to<JsonArray>();
    String types[SENSOR_TYPE_SIZE];
    for (int i = (int)THSensor::SENSOR_TYPE::UNKNOWN + 1; i < SENSOR_TYPE_SIZE; i++)
    {
        THSensor::SENSOR_TYPE type = static_cast<THSensor::SENSOR_TYPE>(i);
        String s = THSensor::sensorToString(type);
        // debug.printf("\th%d: %s\n", i, s.c_str());
        JsonObject st_json;
        st_json["name"] = s;
        sensor_types.add(st_json);
    }

    // send JSON response
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    serializeJson(doc, debug);
    serializeJson(doc, *response);
    request->send(response);
}

/**
 * @brief Returns JSON array of available buttons and PWM sliders
 * 
 */
void handleActurators(AsyncWebServerRequest *request)
{
    JsonDocument doc;
    for (const auto &a : actuators)
    {
        JsonObject actuator = doc.add<JsonObject>();
        actuator["name"] = a->getName();
        actuator["type"] = (uint8_t)a->getType();
        actuator["gpio"] = a->getGPIO();
        actuator["value"] = a->getType() == ActuatorType::ACTUATOR_TOGGLE ? (bool)a->getValue() : a->getValue();
        if(a->getType() == ActuatorType::ACTUATOR_SLIDER){
            actuator["min"] = ((ActuatorPWM*)a)->getMin();
            actuator["max"] = ((ActuatorPWM*)a)->getMax();
        }
    }

    // send JSON response
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    // serializeJson(doc, debug);
    serializeJson(doc, *response);
    request->send(response);
}

/**
 * @brief Handles actuator click and slider change
 * 
 */
void handleActuratorChange(AsyncWebServerRequest *request)
{
    if (request->method() == HTTP_POST)
    {
        debug.printf("Change actuator %s to %ld\n", request->arg("name").c_str(), request->arg("value").toInt());
        Actuator *actuator = getActuator(request->arg("name"));
        if (actuator)
        {
            actuator->setValue(request->arg("value").toInt());
        }
        request->send(200);
    }
    else
    {
        debug.println("Actuator change wrong method");
    }
}

void handleConfig(AsyncWebServerRequest *request)
{
    // if method is POST and param exists
    if (request->method() == HTTP_POST && request->params())
    {

        // ToDo
        debug.println("Config Update");
        debug.println(request->args());
        for (size_t i = 0; i < request->args(); i++)
        {
            debug.printf("Arg %s: %s\n", request->argName(i).c_str(), request->arg(request->argName(i)).c_str());
        }
        JsonDocument doc;
        //JsonObject &rootNew = jsonBufferNew.parseObject(server.arg("data"));
        DeserializationError error = deserializeJson(doc, request->arg(request->argName(0)));

        if (error)
        {
            debug.println("JSON config not valid");
            debug.println(error.c_str());
            request->send(500, "text/html", error.c_str());
            return;
        }

        // check for fields
        String fields[] = {"sensors", "actuators", "timers", "thresholds", "general", "network"};
        for (size_t i = 0; i < 5; i++)
        {
            if (!doc[fields[i]].is<JsonVariant>())
            {
                debug.printf("JSON config misses key %s!", fields[i].c_str());
                request->send(500, "text/html", "Invalid JSON data! Key " + fields[i] + " missing.");
                return;
            }
        }

        // if(!doc.containsKey("network") || !doc.containsKey("sensors") || !doc.containsKey("sensors"))

        // don't overwrite WiFi password if empty
        // replace empty string with old password
        // if(doc["wlan"]["pass"].as<String>().length() == 0){
        //     File dataFile = LITTLEFS.open("/config.json", "r");
        //     DynamicJsonDocument doc_old(1024);
        //     DeserializationError error_old = deserializeJson(doc_old, dataFile);
        //     if(!error_old){
        //         doc["wlan"]["pass"] = doc_old["wlan"]["pass"].as<String>();
        //         dataFile.close();
        //     }
        // }

        // replace config file
        File dataFile = LittleFS.open("/config.json", FILE_WRITE);
        serializeJson(doc, dataFile);
        dataFile.close();
        // send success
        request->send(200, "text/plain", "ok, will reboot now");
        delay(1000);
        // restart to read clean values
        ESP.restart();
    }

    // load config
    File config = LittleFS.open("/config.json", FILE_READ);
    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", config.readString());
    request->send(response);
    config.close();
}

/**
 * @brief gets called when an Alarm was triggered. Checks all Timers for the right alarm and activates actuator
 * 
 */
// void timerCallback()
// {
//     AlarmId id = Alarm.getTriggeredAlarmId();
//     debug.printf("Timer callback for Id %d\n", id);
//     // find the triggered alarm by ID
//     // for (size_t i = 0; i < timers.size(); i++)
//     // {
//     //     Timer *t = timers[i];
//     //     if (t->checkAlarmId(id))
//     //     {
//     //         return;
//     //     }
//     // }
// }

/**
 * @brief Initialize timers, return false if time is not synced
 * 
 * @return true time is synced
 * @return false time is not synced
 */
bool initTimers()
{
    if (timeStatus() != timeStatus_t::timeSet)
    {
        debug.println("Cannot init timers. Time not set");
        return false;
    }

    struct tm ti;
    getLocalTime(&ti);
    // time_t t = mktime(&ti) + settings_time.gmt_offset*3600;
    // time_t t = mktime(&ti);
    // ti = *localtime(&t);
    // unsigned int time_min = ti.tm_hour * 60 + ti.tm_min;

    for (size_t i = 0; i < timers.size(); i++)
    {
        Timer *t = timers[i];
        if (!t->isInitialized())
        {
            t->init(ti);
        }
    }

    return true;
}

/**************************************
 *
 * NTP time sync provider
 *
 *************************************/
time_t getNtpTime()
{
    //configTime(-3600, -3600, "69.10.161.7");

    debug.printf("Update time with GMT+%02d and DST: %d\n",
                 settings_time.gmt_offset, settings_time.dst);

    configTime(settings_time.gmt_offset * 3600, settings_time.dst ? 3600 : 0, "0.de.pool.ntp.org", "1.de.pool.ntp.org");
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        debug.println("Failed to obtain time");
    }
    else
    {
        debug.print("Synced time to: ");
        debug.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
    }
    return mktime(&timeinfo);
}

/**************************************
 * 
 * Read new sensor values
 * 
 *************************************/
void readSensors()
{
    setLEDState(LEDState::READ_SENSORS);
    time_t t = now();
    buffer_time.add(t, true);
    for (size_t i = 0; i < sensors.size(); i++)
    {
        THSensor *s = sensors[i];
        if (s->isEnabled())
        {
            s->updateTH();
            debug.printf("Sensor %s t=%.2fC h=%.2f%%\n", s->getName().c_str(), s->readLastTemperature(), s->readLastHumidity());
        }
    }
    setLEDState(LEDState::NONE);

    // check for thresholds and activate buttons
    for (size_t i = 0; i < thresholds.size(); i++)
    {
        Threshold *t = thresholds[i];
        t->checkThreshold();
    }
}

// /**************************************
//  *
//  * Start rain / whatever for a given duration
//  *
//  *************************************/
// void startThresholdAction(Threshold *t, Actuator *b) {
//     debug.printf("Activate threshold %s for %.2f seconds...\n", t->name.c_str(), t->duration);
//     digitalWrite(b->pin, t->inverted ? LOW : HIGH);
//     Alarm.delay(t->duration*1000);
//     stopThresholdAction(t, b);
//     // Alarm.timerOnce(t->duration, [t, b](){
//     //     digitalWrite(b->pin, t->inverted ? HIGH : LOW);
//     //     t->last_activated = millis();
//     // });
//     // if (settings_rain.initialized) {
//     //     debug.printf("Let it rain for %d seconds\n", settings_rain.duration);
//     //     digitalWrite(settings_rain.pin, settings_rain.inverted ? LOW : HIGH);
//     //     Alarm.timerOnce(settings_rain.duration, stopThresholdAction);
//     // }
// }

// void stopThresholdAction(Threshold *t, Actuator *b) {
//     debug.printf("Deactivate threshold %s after %.2f seconds...\n", t->name.c_str(), t->duration);
//     digitalWrite(b->pin, t->inverted ? HIGH : LOW);
//     t->last_activated = millis();
// }

THSensor *getSensor(String name)
{
    // get Sensor by name
    for (size_t i = 0; i < sensors.size(); i++)
    {
        if (sensors[i]->getName() == name)
        {
            return sensors[i];
        }
    }
    debug.printf("Cannot find sensor with name %s\n", name.c_str());
    return nullptr;
}

Actuator *getActuator(String name)
{
    // get actuator by name
    for (const auto &a : actuators)
    {
        if (a->getName() == name)
        {
            return a;
        }
    }
    debug.printf("Cannot find actuator with name %s\n", name.c_str());
    return nullptr;
}

void setLEDState(LEDState state)
{
    statusLEDState = state;
    updateLEDState();
}

void updateLEDState()
{
    debug.printf("Set LED state %d\n", statusLEDState);
    RgbColor color;
    switch (statusLEDState)
    {
    case LEDState::NONE:
        color = RgbColor(0, 0, 0);
        break;
    case LEDState::ERROR:
        color = RgbColor(255, 0, 0);
        break;
    case LEDState::WIFI_STATE_AP:
        color = RgbColor(255, 255, 0);
        break;
    case LEDState::WIFI_STATE_CONNECTING:
        color = RgbColor(0, 255, 255);
        break;
    case LEDState::WIFI_STATE_CONNECTED:
        color = RgbColor(0, 255, 0);
        break;
    case LEDState::WIFI_STATE_DISCONNECTED:
        color = RgbColor(255, 0, 0);
        break;
    case LEDState::OTA_ACTIVE:
        color = RgbColor(0, 0, 255);
        break;
    case LEDState::READ_SENSORS:
        color = RgbColor(0, 255, 255);
        break;
    default:
        break;
    }

    statusLED.SetPixelColor(0, color.Dim(30));

    statusLED.Show();
    debug.printf("LED brightness: %d\n", statusLED.GetPixelColor(0).CalculateBrightness());
}
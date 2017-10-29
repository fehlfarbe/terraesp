#include <WiFi.h>
#include <ESPmDNS.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include "ESPTemplateProcessor.h"
#include <time.h>
#include <Time.h>
#include <TimeAlarms.h>
#include "FS.h"
#include "SPIFFS.h"
#include <ArduinoJson.h>
#include <LinkedList.h>
#include <DHT.h>
#include <RingBufHelpers.h>
#include <RingBufCPP.h>

#include "datatypes.h"
#include "utils.h"

// settings
String ssid = "";
String password = "";
String host = "terraesp";
bool dst = false; // DaylightSavingTime
int DHT22_pin;
RainSettings rain;
LinkedList<Button*> buttons;

// sensors
DHT *dht = nullptr;
RingBufCPP<time_t, 720> buffer_time;
RingBufCPP<float, 720> buffer_temp;
RingBufCPP<float, 720> buffer_humid;


//WebServer
WebServer server(80);
File fsUploadFile;

// timer
hw_timer_t *timer = NULL;
LinkedList<AlarmSettings*> alarms;
//LinkedList<ThreshSettings*> threshs;


void setup() {

    // start serial output
    Serial.begin(115200);

    // File system
    if (!SPIFFS.begin(true)) {
        Serial.println("Failed to mount file system");
        return;
    }
    listDir(SPIFFS, "/", 0);

    // settings
    loadSettings(SPIFFS);

    // WiFi and HTTP
    connectWiFi();
    server.begin();

    // sync time
    setSyncProvider(getNtpTime);
    // init timers
    while (!initTimers()) {
        Alarm.delay(100);
    }

    // timer for sensors, read new sensor values every 120s
    Alarm.timerRepeat(120, readSensors);
    readSensors();

    // HTTP handles
    server.on("/edit", HTTP_POST, []() {
        server.send(200, "text/plain", "");
    }, handleFileUpload);
    server.on("/", handleRoot);
    server.on("/config", handleConfig);
    server.on("/config/update", HTTP_POST, handleConfigUpdate);
    server.on("/config.json", []() {
        File dataFile = SPIFFS.open("/config.json");
        server.send(200, "text/json", dataFile.readString());
    });
    server.on("/sensordata.json", handleSensordata);
    server.on("/buttons.json", handleButtons);
    server.on("/button/change", HTTP_POST, handleButtonChange);

    // DEBUG: ADD RANDOM SENSOR VALUES
    //randomValues(720);
}

void loop() {
    // check WiFi an reconnect if necessary
    if (WiFi.status() != WL_CONNECTED) {
        connectWiFi();
    }

    // sync time
    if (timeStatus() == timeNotSet) {
        Serial.println("Waiting for synced time");
    }

    // serve HTTP content
    server.handleClient();

    // delay and handle alarms/sensors and so on
    Alarm.delay(10);
}

/**************************************
 * 
 * Load Settings and initializte stuff
 * 
 *************************************/
void loadSettings(fs::FS &fs) {
    File dataFile = fs.open("/config.json");
    String json = dataFile.readString();
    dataFile.close();
    Serial.println(json);
    DynamicJsonBuffer jsonBuffer;
    JsonObject &root = jsonBuffer.parseObject(json);
    if (!root.success()) {
        Serial.println("----- parseObject() failed -----");
        return;   //if parsing failed, this will exit the function and not change the values to 00

    } else {
        // load wlan settings
        JsonObject &wlanJSON = root["wlan"];
        if (wlanJSON.success()) {
            ssid = (const char *) wlanJSON["ssid"];
            password = (const char *) wlanJSON["pass"];
            host = (const char *) wlanJSON["host"];
        } else {
            // @ToDo: start access point
            Serial.println("Cannon read WiFi config, ToDo: start access point");
        }

        // load timer settings
        JsonArray &timerJSON = root["timer"];
        for (auto &t : timerJSON) {
            //Serial.println((const char*)t["name"]);
            //pinMode((int)t["pin"], OUTPUT);
            struct AlarmSettings *s = new struct AlarmSettings;
            s->name = (const char *) t["name"];
            s->pin = (uint8_t) t["pin"];
            s->alarmOnHour = (int) split(String((const char *) t["on"]), ':', 0).toInt();
            s->alarmOnMinute = (int) split(String((const char *) t["on"]), ':', 1).toInt();
            s->alarmOffHour = (int) split(String((const char *) t["off"]), ':', 0).toInt();
            s->alarmOffMinute = (int) split(String((const char *) t["off"]), ':', 1).toInt();
            alarms.add(s);
        };

        // load sensor settings
        JsonArray &sensorsJSON = root["sensors"];
        for (auto &s : sensorsJSON) {
            Serial.print("Got new sensor ");
            Serial.print((const char *) s["type"]);
            Serial.print(" on pin ");
            Serial.println((const char *) s["pin"]);
            if (strcmp((const char *) s["type"], "DHT22") == 0) {
                dht = new DHT((int) s["pin"], DHT22);
                dht->begin();
            } else if (strcmp((const char *) s["type"], "DHT21") == 0) {
                dht = new DHT((int) s["pin"], DHT21);
                dht->begin();
            } else if (strcmp((const char *) s["type"], "DHT11") == 0) {
                dht = new DHT((int) s["pin"], DHT11);
                dht->begin();
            } else if (strcmp((const char *) s["type"], "AM2301") == 0) {
                dht = new DHT((int) s["pin"], AM2301);
                dht->begin();
            }
        }

        // load rain
        JsonObject &rainJSON = root["rain"];
        if (rainJSON.success()) {
            rain.pin = (uint8_t) rainJSON["pin"];
            rain.duration = (int) rainJSON["duration"];
            rain.threshold = (int) rainJSON["threshold"];
            rain.initialized = true;
        } else {
            Serial.println("No config for rain");
        }

        // load buttons
        JsonArray &buttonsJSON = root["buttons"];
        uint8_t channel = 0;
        for (auto &b : buttonsJSON) {
            Button *btn = new Button;
            btn->name = (const char *) b["name"];
            btn->pin = (uint8_t) b["pin"];
            btn->type = strcmp(b["type"], "toggle") ? BTN_SLIDER : BTN_TOGGLE;
            btn->min = (int) b["min"];
            btn->max = (int) b["max"];
            buttons.add(btn);

            // set pin mode
            switch(btn->type){
                case BTN_TOGGLE:
                    pinMode(btn->pin, OUTPUT);
                    break;
                case BTN_SLIDER:
                    // Initialize channels
                    // channels 0-15, resolution 1-16 bits, freq limits depend on resolution
                    // ledcSetup(uint8_t channel, uint32_t freq, uint8_t resolution_bits);
                    btn->channel = channel;
                    ledcAttachPin(btn->pin, channel); // assign RGB led pins to channels
                    ledcSetup(channel++, 12000, 8); // 12 kHz PWM, 8-bit resolution
                    break;
                default:
                    break;
            }
        }

        // general
        JsonObject &generalJSON = root["general"];
        dst = (bool)generalJSON["dst"];
    }
}

/**************************************
 * 
 * (Re-)connect WiFi
 * 
 *************************************/
void connectWiFi() {
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid.c_str(), password.c_str());

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    MDNS.begin(host.c_str());
}

/**************************************
 * 
 * HTTP handling
 * 
 *************************************/
void handleRoot() {
    if (ESPTemplateProcessor(server).send(String("/base.html"), indexProcessor)) {
        Serial.println("SUCCESS");
    } else {
        Serial.println("FAIL");
        server.send(404, "text/plain", "page not found.");
    }
}

String indexProcessor(const String &key) {
    Serial.println(String("KEY IS ") + key);
    if (key == "CONTENT") {
        File dataFile = SPIFFS.open("/home.html");
        return dataFile.readString();
    }
    return "";
}

void handleConfig() {
    if (ESPTemplateProcessor(server).send(String("/base.html"), configProcessor)) {
    } else {
        server.send(404, "text/plain", "page not found.");
    }
}

void handleSensordata() {
    String json_temp = "[";
    String json_humid = "[";
    Serial.println(buffer_temp.numElements());
    for (size_t i = 0; i < buffer_temp.numElements(); i++) {
        json_temp += "[";
        json_temp += *buffer_time.peek(i);
        json_temp += ",";
        json_temp += *buffer_temp.peek(i);
        json_temp += "]";
        json_humid += "[";
        json_humid += *buffer_time.peek(i);
        json_humid += ",";
        json_humid += *buffer_humid.peek(i);
        json_humid += "]";
        if (i != buffer_temp.numElements() - 1) {
            json_temp += ",";
            json_humid += ",";
        }
    }
    json_temp += "]";
    json_humid += "]";
    server.send(200, "text/json",
                "{\"temperature\": " + json_temp + ",\"humid\": " + json_humid + "}");
}

void handleButtons(){
    DynamicJsonBuffer jsonBuffer;
    JsonArray& root = jsonBuffer.createArray();
    for(size_t i=0; i<buttons.size(); i++){
        Button b = *buttons.get(i);
        JsonObject& btn = root.createNestedObject();
        btn["name"] = b.name;
        btn["type"] = (uint8_t)b.type;
        btn["pin"] = b.pin;
        switch(b.type){
            case BTN_TOGGLE:
                btn["state"] = digitalRead(b.pin) != 0;
                break;
            case BTN_SLIDER:
                btn["min"] = b.min;
                btn["max"] = b.max;
                btn["state"] = ledcRead(b.pin);
                break;
        }
    }
    String s;
    root.printTo(s);
    server.send(200, "text/json", s);
}

void handleButtonChange(){
    Serial.println("Button changed");
    String name = server.arg("name"); //root["name"];
    uint8_t value = server.arg("value").toInt();
    Serial.println(name);
    Serial.println(value);
    for(size_t i=0; i<buttons.size(); i++) {
        Button b = *buttons.get(i);
        if(name == b.name){
            switch(b.type){
                case BTN_TOGGLE:
                    digitalWrite(b.pin, value);
                    break;
                case BTN_SLIDER:
                    if(value <= b.max && value >= b.min){
                        //analogWrite(b.pin, value);
                        ledcWrite(b.channel, value);
                    }
                    break;
                default:
                    break;
            }
        }
    }
    server.send(200, "text/plain", "ok");
}

String configProcessor(const String &key) {
    if (key == "CONTENT") {
        File dataFile = SPIFFS.open("/config.html");
        return dataFile.readString();
    }
    return "";
}


void handleConfigUpdate() {
    Serial.println("Config Update");
    Serial.println(server.args());
    Serial.println(server.arg("data"));
    File dataFile = SPIFFS.open("/config.json", "w");
    if (dataFile) {
        dataFile.print(server.arg("data"));
        dataFile.close();
        server.send(200, "text/plain", "ok");
        Alarm.delay(100);
        // restart to read clean values
        ESP.restart();
    } else {
        server.send(503, "text/plain", "file error");
    }

}

void handleFileUpload() {
    if (server.uri() != "/edit") return;
    HTTPUpload &upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
        String filename = upload.filename;
        if (!filename.startsWith("/")) filename = "/" + filename;
        Serial.print("handleFileUpload Name: ");
        Serial.println(filename);
        fsUploadFile = SPIFFS.open(filename, "w");
        filename = String();
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (fsUploadFile)
            fsUploadFile.write(upload.buf, upload.currentSize);
    } else if (upload.status == UPLOAD_FILE_END) {
        if (fsUploadFile)
            fsUploadFile.close();
        Serial.print("handleFileUpload Size: ");
        Serial.println(upload.totalSize);
    }
}

/**************************************
 * 
 * Timer callback
 * 
 *************************************/
void timerCallback() {
    AlarmId id = Alarm.getTriggeredAlarmId();
    // find the triggered alarm by ID
    for (size_t i = 0; i < alarms.size(); i++) {
        AlarmSettings *a = alarms.get(i);
        if (a->alarmOnId == id) {
            Serial.print("Timer ");
            Serial.print(a->name);
            Serial.println(" triggered");
            digitalWrite(a->pin, HIGH);
            break;
        }
        if (a->alarmOffId == id) {
            Serial.print("Timer ");
            Serial.print(a->name);
            Serial.println(" triggered");
            digitalWrite(a->pin, LOW);
            break;
        }
    }
}

bool initTimers() {
    if (timeStatus() != timeSet) {
        Serial.println("Cannot init timers. Time not set");
        return false;
    }

    for (size_t i = 0; i < alarms.size(); i++) {
        AlarmSettings *a = alarms.get(i);
        Serial.print("Init timer ");
        Serial.print(a->name);
        Serial.print(" on: ");
        Serial.print(a->alarmOnHour);
        Serial.print(":");
        Serial.print(a->alarmOnMinute);
        Serial.print(" off: ");
        Serial.print(a->alarmOffHour);
        Serial.print(":");
        Serial.println(a->alarmOffMinute);
        pinMode(a->pin, OUTPUT);
        // test timer
//        if (i == 0) {
//            a->alarmOnId = Alarm.alarmRepeat(hour(), minute() + 1, 0, timerCallback);
//            a->alarmOffId = Alarm.alarmRepeat(hour(), minute() + 1, 10, timerCallback);
//        } else {
            a->alarmOnId = Alarm.alarmRepeat(a->alarmOnHour, a->alarmOnMinute, 0, timerCallback);
            a->alarmOffId = Alarm.alarmRepeat(a->alarmOffHour, a->alarmOffMinute, 0, timerCallback);
//        }
        a->init = true;
    }

    return true;
}

time_t getNtpTime() {
    //configTime(-3600, -3600, "69.10.161.7");
    configTime(-3600, dst ? 3600 : 0, "69.10.161.7");
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        Serial.println("Failed to obtain time");
    } else {
        Serial.print("Synced time to: ");
        Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
    }
    return mktime(&timeinfo) + 7200;
}

/**************************************
 * 
 * Read new sensor values
 * 
 *************************************/
void readSensors() {
    if (dht) {
        time_t t;
        float temp;
        float humid;

        // remove first buffer element if full
        if (buffer_time.isFull()) {
            buffer_time.pull(&t);
            buffer_temp.pull(&temp);
            buffer_humid.pull(&humid);
        }

        // get new values
        t = now();
        temp = dht->readTemperature();
        humid = dht->readHumidity();

        // test for NaN values
        if (!isnan(t) && !isnan(temp) && !isnan(humid)) {
            // add values to buffer
            buffer_time.add(t);
            buffer_temp.add(temp);
            buffer_humid.add(humid);
        } else {
            Serial.println("NaN value detected");
            return;
        }

        Serial.print(t);
        Serial.print(" ");
        Serial.print("Temperature: ");
        Serial.print(temp);
        Serial.print(" Humidity: ");
        Serial.println(humid);

        // activate rain
        if (rain.initialized && humid < rain.threshold) {
            String s = "Humidity is under threshold " + String(humid) + "<" + String(rain.threshold);
            s += "\nstart rain for " + String(rain.duration) + " seconds...";
            Serial.println(s);
            letItRain();
        }
    }
}

void letItRain() {
    if (rain.initialized) {
        digitalWrite(rain.pin, HIGH);
        Alarm.timerOnce(rain.duration, stopRain);
    }
}

void stopRain() {
    if (rain.initialized) {
        digitalWrite(rain.pin, LOW);
    }
}

void randomValues(size_t count) {
    time_t t;
    float temp;
    float humid;

    for (size_t i = 0; i < count; i++) {
        // remove first buffer element if full
        if (buffer_time.isFull()) {
            buffer_time.pull(&t);
            buffer_temp.pull(&temp);
            buffer_humid.pull(&humid);
        }

        t = now() + i;
        temp = humid = (float) i;
        // add values to buffer
        buffer_time.add(t);
        buffer_temp.add(temp);
        buffer_humid.add(humid);
    }
}


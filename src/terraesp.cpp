#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
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
//#include <DHT.h>
// use Rob Tillaart's DHTlib because of NaN errors in Adafruit lib
#include <dht.h>
#include <RingBufHelpers.h>
#include <RingBufCPP.h>
#include <WiFiManager.h>
#include "Debug.h"

#include "datatypes.h"
#include "utils.h"

// WiFi
WiFiManager wifiManager;

// settings
String ssid = "";
String password = "";
String host = "terraesp";
String ap_ssid = "terraesp";
bool ap_mode = false;
TimeSettings settings_time;
RainSettings settings_rain;
LinkedList<Button*> buttons;
LinkedList<AlarmSettings*> alarms;
// String basic_auth_user = "";
// String basic_auth_pass = "";

// sensors
//DHT *dht = nullptr;
dht dht_sensor;
DHTType dht_type = UNKNOWN;
uint8_t dht_pin = 0;
RingBufCPP<time_t, 720> buffer_time;
RingBufCPP<float, 720> buffer_temp;
RingBufCPP<float, 720> buffer_humid;

//WebServer
WebServer server(80);
File fsUploadFile;

// Telnet
Debug debug;
WiFiServer telnetServer(23);
WiFiClient telnetClient;

/**************************
 * DECLARATIONS
 *************************/
void letItRain();
void stopRain();
void randomValues(size_t count);
void readSensors();
time_t getNtpTime();
bool initTimers();
void timerCallback();
void handleFileUpload();
void handleConfigUpdate();
void handleButtonChange();
void handleButtons();
void handleSensordata();
void handleConfig();
String configProcessor(const String &key);
String indexProcessor(const String &key);
void handleRoot();
void initOTA();
void createAP();
bool connectWiFi();
void loadSettings(fs::FS &fs);

/**************************
 * DEFINITIONS
 *************************/
void setup() {

    // start debug output
    Serial.begin(115200);

    // init File system and show content
    if (!SPIFFS.begin(true)) {
        debug.println("Failed to mount file system");
        return;
    }
    listDir(SPIFFS, "/", 0);

    // settings
    loadSettings(SPIFFS);

    // WiFi and HTTP
    // if(!connectWiFi()){
    //     createAP();
    // }
    wifiManager.autoConnect();
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
    // server.on("/edit", HTTP_POST, []() {
    //     server.send(200, "text/plain", "");
    // }, handleFileUpload);
    server.on("/", handleRoot);
    server.on("/config", handleConfig);
    server.on("/config/update", HTTP_POST, handleConfigUpdate);
    server.on("/config.json", []() {
        File dataFile = SPIFFS.open("/config.json");
        // remove WiFi password for security reasons
        DynamicJsonBuffer jsonBuffer;
        JsonObject &root = jsonBuffer.parseObject(dataFile);
        JsonObject &wlanJSON = root["wlan"];
        wlanJSON["pass"] = "";
        String json;
        root.printTo(json);
        debug.println(json);
        server.send(200, "text/json", json);
    });
    server.on("/sensordata.json", handleSensordata);
    server.on("/buttons.json", handleButtons);
    server.on("/button/change", HTTP_POST, handleButtonChange);
    server.on("/reboot", []() {
        server.send(200, "text/json", "{\"answer\": \"ok\"}");
        Alarm.delay(100);
        ESP.restart();
    });
    server.on("/time", []() { // return current device time
        String time = String(now());
        server.send(200, "text/json", "{\"time\": \"" + time + "\"}");
    });
    server.on("/stats.json", []() { // return current device time
        server.send(200, "text/json", "{\"free_heap\": \"" + String(ESP.getFreeHeap()) + "\", "
                                      "\"chip_rev\" : \"" + ESP.getChipRevision() + "\", "
                                      "\"sdk\" : \"" + ESP.getSdkVersion() + "\", "
                                      "\"wifi\" : \"" + WiFi.RSSI() + "\", "
                                      "\"cpu_freq\" : \"" + ESP.getCpuFreqMHz() + "\"}");
    });

    server.serveStatic("/bootstrap.min.js", SPIFFS, "/bootstrap.min.js");
    server.serveStatic("/bootstrap.min.css", SPIFFS, "/bootstrap.min.css");
    server.serveStatic("/highcharts.js", SPIFFS, "/highcharts.js");
    server.serveStatic("/highslide.config.js", SPIFFS, "/highslide.config.js");
    server.serveStatic("/highslide.css", SPIFFS, "/highslide.css");
    server.serveStatic("/highslide-full.min.js", SPIFFS, "/highslide-full.min.js");
    server.serveStatic("/jquery-3.1.1.min.js", SPIFFS, "/jquery-3.1.1.min.js");
    server.serveStatic("/popper.min.js", SPIFFS, "/popper.min.js");

//    server.on("/bootstrap.min.js", []() {
//        debug.println("BOOTSTRAP JS");
//        File dataFile = SPIFFS.open("/bootstrap.min.js");
//        server.send(200, "text/json", dataFile.readString());
//    });
//    server.on("/bootstrap.min.css", []() {
//        debug.println("BOOTSTRAP CSS START");
//        File dataFile = SPIFFS.open("/bootstrap.min.css");
//        server.send(200, "text/json", dataFile.readString());
//        debug.println("BOOTSTRAP CSS END");
//    });

    // init OTA update
    initOTA();

    // init telnetServer
    debug.setTelnet(&telnetServer, &telnetClient);
    telnetServer.begin();
    telnetServer.setNoDelay(true);

    // DEBUG: ADD RANDOM SENSOR VALUES
    //randomValues(720);
}

void loop() {
    // check WiFi an reconnect if necessary
    if (WiFi.status() != WL_CONNECTED and !ap_mode) {
        connectWiFi();
    }

    // handle OTA updates
    ArduinoOTA.handle();

    // sync time
    if (timeStatus() == timeNotSet) {
        debug.println("Waiting for synced time");
    }

    // serve HTTP content
    server.handleClient();

    // add new telnet Clients
    if (telnetServer.hasClient()) {
        if (!telnetClient || !telnetClient.connected()) {
            if (telnetClient) {
                telnetClient.stop();
                debug.println("Telnet Client Stop");
            }
            telnetClient = telnetServer.available();
            debug.println("New Telnet client");
            telnetClient.flush();
            telnetClient.println("Welcome to TerraESP");
        }
    }

    // delay, handle alarms/sensors and so on
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
    debug.println(json);
    DynamicJsonBuffer jsonBuffer;
    JsonObject &root = jsonBuffer.parseObject(json);
    if (!root.success()) {
        debug.println("----- parseObject() for config.json failed -----");
        return;
    } else {
        // load wlan settings
        JsonObject &wlanJSON = root["wlan"];
        if (wlanJSON.success()) {
            ssid = (const char *) wlanJSON["ssid"];
            password = (const char *) wlanJSON["pass"];
            host = (const char *) wlanJSON["host"];
        } else {
            debug.println("Cannot read WiFi config. Start access point");
            createAP();
        }

        // load basic auth settings
        // JsonObject &basicAuthJSON = root["basic_auth"];
        // if (basicAuthJSON.success()) {
        //     basic_auth_user = (const char *) basicAuthJSON["user"];
        //     basic_auth_pass = (const char *) basicAuthJSON["pass"];
        // } else {
        //     debug.println("Cannot read basic auth config");
        //     createAP();
        // }
        // load timer settings
        JsonArray &timerJSON = root["timer"];
        for (auto &t : timerJSON) {
            struct AlarmSettings *s = new struct AlarmSettings;
            s->name = (const char *) t["name"];
            s->pin = (uint8_t) t["pin"];
            s->alarmOnHour = (int) split(String((const char *) t["on"]), ':', 0).toInt();
            s->alarmOnMinute = (int) split(String((const char *) t["on"]), ':', 1).toInt();
            s->alarmOffHour = (int) split(String((const char *) t["off"]), ':', 0).toInt();
            s->alarmOffMinute = (int) split(String((const char *) t["off"]), ':', 1).toInt();
            s->inverted = (bool) t["inverted"];
            alarms.add(s);
        };

        // load sensor settings
        JsonArray &sensorsJSON = root["sensors"];
        for (auto &s : sensorsJSON) {
            debug.print("Got new sensor ");
            debug.print((const char *) s["type"]);
            debug.print(" on pin ");
            debug.println((const char *) s["pin"]);
            dht_pin = (uint8_t) s["pin"];
            if (strcmp((const char *) s["type"], "DHT22") == 0) {
                dht_type = DHT22;
            } else if (strcmp((const char *) s["type"], "AM2302") == 0) {
                dht_type = AM2302;
            } else if (strcmp((const char *) s["type"], "DHT21") == 0) {
                dht_type = DHT21;
            } else if (strcmp((const char *) s["type"], "DHT11") == 0) {
                dht_type = DHT11;
            } else if (strcmp((const char *) s["type"], "AM2301") == 0) {
                dht_type = AM2301;
            }
        }

        // load rain
        JsonObject &rainJSON = root["rain"];
        if (rainJSON.success()) {
            settings_rain.pin = (uint8_t) rainJSON["pin"];
            settings_rain.duration = (int) rainJSON["duration"];
            settings_rain.minGap = (int) rainJSON["min_gap"];
            settings_rain.threshold = (int) rainJSON["threshold"];
            settings_rain.initialized = true;
            settings_rain.inverted = (bool) rainJSON["inverted"];
            pinMode(settings_rain.pin, OUTPUT);
            digitalWrite(settings_rain.pin, (uint8_t) settings_rain.inverted);
        } else {
            debug.println("No (valid) config for rain");
        }

        // load buttons
        JsonArray &buttonsJSON = root["buttons"];
        uint8_t channel = 0;
        for (auto &b : buttonsJSON) {
            Button *btn = new Button;
            btn->name = (const char *) b["name"];
            btn->pin = (uint8_t) b["pin"];
            btn->type = strcmp(b["type"], "toggle") ? BTN_SLIDER : BTN_TOGGLE;
            btn->inverted = (bool) b["inverted"];
            //btn->min = (int) b["min"];
            //btn->max = (int) b["max"];
            buttons.add(btn);

            // set pin mode
            switch(btn->type){
                case BTN_TOGGLE:
                    pinMode(btn->pin, OUTPUT);
                    digitalWrite(btn->pin, (uint8_t) btn->inverted);
                    break;
                case BTN_SLIDER:
                    // Initialize channels
                    // channels 0-15, resolution 1-16 bits, freq limits depend on resolution
                    // ledcSetup(uint8_t channel, uint32_t freq, uint8_t resolution_bits);
                    btn->channel = channel;
                    ledcAttachPin(btn->pin, channel); // assign RGB led pins to channels
                    ledcSetup(channel, 12000, 8); // 12 kHz PWM, 8-bit resolution
                    channel++;
                    break;
                default:
                    break;
            }
        }

        // general settings
        JsonObject &generalJSON = root["general"];
        settings_time.dst = (bool)generalJSON["dst"]; // daylight saving time
        settings_time.gmt_offset_sec = (int)generalJSON["gmt_offset"]; // daylight saving time
    }
}

/**************************************
 * 
 * (Re-)connect WiFi
 * 
 *************************************/
bool connectWiFi() {
    debug.print("Connecting to ");
    debug.println(ssid);
    WiFi.begin(ssid.c_str(), password.c_str());

    int retries = 0;
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        debug.print(".");
        if(retries++ > 10){
            return false;
        }
    }

    debug.println("");
    debug.println("WiFi connected");
    debug.println("IP address: ");
    debug.println(WiFi.localIP());

    MDNS.begin(host.c_str());
    return true;
}

void createAP(){
    debug.println("Cannot connect to WiFi. Create Access Point");
    WiFi.softAP(ap_ssid.c_str());
    ap_mode = true;
    IPAddress myIP = WiFi.softAPIP();
    debug.print("AP IP address: ");
    debug.println(myIP);
}

void initOTA(){
    // Port defaults to 3232
    // ArduinoOTA.setPort(3232);

    // Hostname defaults to esp3232-[MAC]
    ArduinoOTA.setHostname(host.c_str());

    // No authentication by default
    // ArduinoOTA.setPassword("admin");

    // Password can be set with it's md5 value as well
    // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
    ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

    ArduinoOTA.onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH){
            type = "sketch";
        }
        else {
            // U_SPIFFS
            type = "filesystem";
        }

        // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
        debug.println("Start updating " + type);
    });
    ArduinoOTA.onEnd([]() {
        debug.println("\nEnd");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        debug.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
        debug.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) debug.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR) debug.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR) debug.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) debug.println("Receive Failed");
        else if (error == OTA_END_ERROR) debug.println("End Failed");
    });
    ArduinoOTA.begin();
    debug.println("OTA update ready");
}

/**************************************
 * 
 * HTTP handler
 * 
 *************************************/
void handleRoot() {
    if (ESPTemplateProcessor(server).send(String("/base.html"), indexProcessor)) {
        //debug.println("SUCCESS");
    } else {
        debug.println("FAIL");
        server.send(404, "text/plain", "page not found.");
    }
}

String indexProcessor(const String &key) {
    //debug.println(String("KEY IS ") + key);
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
    //debug.println(buffer_temp.numElements());
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
                "{\"temperature\": " + json_temp + ",\"humid\": " + json_humid +
                        ",\"humid_level\" : " + String(settings_rain.threshold) + "}");
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
                btn["state"] = digitalRead(b.pin) ^ b.inverted != 0;
                break;
            case BTN_SLIDER:
                btn["min"] = b.min;
                btn["max"] = b.max;
                btn["state"] = ledcRead(b.channel);
                break;
        }
    }
    String s;
    root.printTo(s);
    server.send(200, "text/json", s);
}

void handleButtonChange(){
    String name = server.arg("name"); //root["name"];
    uint8_t value = server.arg("value").toInt();
    debug.printf("Button changed: %s -> %d\n", name.c_str(), value);
    for(size_t i=0; i<buttons.size(); i++) {
        Button b = *buttons.get(i);
        if(name == b.name){
            switch(b.type){
                case BTN_TOGGLE:
                    debug.println(value ^ b.inverted);
                    digitalWrite(b.pin, value ^ b.inverted);
                    break;
                case BTN_SLIDER:
                    if(value <= b.max && value >= b.min){
                        //analogWrite(b.pin, value);
                        debug.printf("Change PWM on channel %d to value: %d\n", b.channel, value);
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
    debug.println("Config Update");
    debug.println(server.args());
    debug.println(server.arg("data"));
    DynamicJsonBuffer jsonBufferNew;
    JsonObject &rootNew = jsonBufferNew.parseObject(server.arg("data"));
    if(rootNew.success()){
        // don't overwrite WiFi password if empty
        // replace empty string with old password
        if(rootNew["wlan"]["pass"].as<String>().length() == 0){
            File dataFile = SPIFFS.open("/config.json", "r");
            DynamicJsonBuffer jsonBufferOld;
            JsonObject &rootOld = jsonBufferOld.parseObject(dataFile);
            rootNew["wlan"]["pass"] = rootOld["wlan"]["pass"].as<String>();
            dataFile.close();
        }
        // replace config file
        File dataFile = SPIFFS.open("/config.json", "w");
        rootNew.printTo(dataFile);
        dataFile.close();
        // send success
        server.send(200, "text/plain", "ok");
        Alarm.delay(100);
        // restart to read clean values
        ESP.restart();
    } else {
        debug.println("JSON config not valid");
        server.send(503, "text/plain", "invalid JSON data");
    }

}

// void handleFileUpload() {
//     if (server.uri() != "/edit") return;
//     HTTPUpload &upload = server.upload();
//     if (upload.status == UPLOAD_FILE_START) {
//         String filename = upload.filename;
//         if (!filename.startsWith("/")) filename = "/" + filename;
//         debug.print("handleFileUpload Name: ");
//         debug.println(filename);
//         fsUploadFile = SPIFFS.open(filename, "w");
//         filename = String();
//     } else if (upload.status == UPLOAD_FILE_WRITE) {
//         if (fsUploadFile)
//             fsUploadFile.write(upload.buf, upload.currentSize);
//     } else if (upload.status == UPLOAD_FILE_END) {
//         if (fsUploadFile)
//             fsUploadFile.close();
//         debug.print("handleFileUpload Size: ");
//         debug.println(upload.totalSize);
//     }
// }

/**************************************
 * 
 * check basic auth if user and password is set
 * 
 *************************************/
// bool checkAuth(){
//     debug.println("Checking basic auth...");
//     if(basic_auth_user.length() == 0 || basic_auth_user.length() == 0){
//         debug.println("No user:pass set");
//         return true;
//     }
//     char* user = new char[basic_auth_user.length()];
//     char* pass = new char[basic_auth_pass.length()];
//     basic_auth_user.toCharArray(user, basic_auth_user.length());
//     basic_auth_pass.toCharArray(pass, basic_auth_pass.length());
//     debug.printf("Checking %s%s\n", user, pass);
//     bool auth = server.authenticate(user, pass);
//     delete user;
//     delete pass;
//     return auth;
// }

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
            debug.print("Timer ");
            debug.print(a->name);
            debug.println(" triggered");
            digitalWrite(a->pin, a->inverted ? LOW : HIGH);
            break;
        }
        if (a->alarmOffId == id) {
            debug.print("Timer ");
            debug.print(a->name);
            debug.println(" triggered");
            digitalWrite(a->pin, a->inverted ? HIGH : LOW);
            break;
        }
    }
}

/**************************************
 *
 * Initialize timers, return false if
 * time is not synced
 *
 *************************************/
bool initTimers() {
    if (timeStatus() != timeSet) {
        debug.println("Cannot init timers. Time not set");
        return false;
    }

    struct tm ti;
    getLocalTime(&ti);
    time_t t = mktime(&ti) + settings_time.gmt_offset_sec;
    ti = *localtime(&t);
    unsigned int time_min = ti.tm_hour * 60 + ti.tm_min;

    for (size_t i = 0; i < alarms.size(); i++) {
        AlarmSettings *a = alarms.get(i);
        debug.printf("Init timer %s on: %02d:%02d, off: %02d:%02d\n",
        a->name.c_str(), a->alarmOnHour, a->alarmOnMinute, a->alarmOffHour, a->alarmOffMinute);
        pinMode(a->pin, OUTPUT);
        a->alarmOnId = Alarm.alarmRepeat(a->alarmOnHour, a->alarmOnMinute, 0, timerCallback);
        a->alarmOffId = Alarm.alarmRepeat(a->alarmOffHour, a->alarmOffMinute, 0, timerCallback);
        a->init = true;

        // activate pin if we are in the timespan
        unsigned int alarm_on = a->alarmOnHour * 60 + a->alarmOnMinute;
        unsigned int alarm_off = a->alarmOffHour * 60 + a->alarmOffMinute;
        if(alarm_on < time_min && time_min < alarm_off){
            digitalWrite(a->pin, a->inverted ? LOW : HIGH);
        }
    }

    return true;
}

/**************************************
 *
 * NTP time sync provider
 *
 *************************************/
time_t getNtpTime() {
    //configTime(-3600, -3600, "69.10.161.7");

    debug.printf("Update time with GMT+%02d and DST: %d\n",
                 settings_time.gmt_offset_sec/3600, settings_time.dst);

    configTime(0, settings_time.dst ? 3600 : 0, "69.10.161.7");
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        debug.println("Failed to obtain time");
    } else {
        debug.print("Synced time to: ");
        debug.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
    }
    return mktime(&timeinfo) + settings_time.gmt_offset_sec;
}

/**************************************
 * 
 * Read new sensor values
 * 
 *************************************/
void readSensors() {
    if (dht_type != UNKNOWN) {
        time_t t;
        float temp = 0;
        float humid = 0;

        // get new values (try up to 10 times if NaN or incorrect values)
        size_t i = 0;
        for(; i<10; i++){
            t = now();
            int chk = -1;
            switch (dht_type){
                case DHT11:
                    chk = dht_sensor.read11(dht_pin);
                    break;
                case DHT21:
                    chk = dht_sensor.read21(dht_pin);
                    break;
                case DHT22:
                    chk = dht_sensor.read22(dht_pin);
                    break;
                case AM2301:
                    chk = dht_sensor.read2301(dht_pin);
                    break;
                case AM2302:
                    chk = dht_sensor.read2302(dht_pin);
                    break;
                default:
                    chk = -1;
            }
            switch (chk)
            {
                case DHTLIB_OK:
                    debug.print("OK,\t");
                    break;
                case DHTLIB_ERROR_CHECKSUM:
                    debug.print("Checksum error,\t");
                    break;
                case DHTLIB_ERROR_TIMEOUT:
                    debug.print("Time out error,\t");
                    break;
                case DHTLIB_ERROR_CONNECT:
                    debug.print("Connect error,\t");
                    break;
                case DHTLIB_ERROR_ACK_L:
                    debug.print("Ack Low error,\t");
                    break;
                case DHTLIB_ERROR_ACK_H:
                    debug.print("Ack High error,\t");
                    break;
                default:
                    debug.print("Unknown error,\t");
                    break;
            }

            // test for NaN values and filter incorrect values
            if (chk != DHTLIB_OK) {
                // delay time between reads > 2s
                temp = humid = 0;
                Alarm.delay(2100);
                continue;
            } else {
                temp = dht_sensor.temperature;
                humid = dht_sensor.humidity;
                // both 0 == wrong sensor?
                if(temp == 0.f && humid == 0.f){
                    debug.println("Temperature and humidity = 0. Wrong sensor?");
                    Alarm.delay(2100);
                    continue;
                } else {
                    break;
                }
            }
        }

        if(temp == 0 && humid == 0){
            debug.println("Too many NaN/0.0 values detected");
            return;
        }

        debug.printf("%d: temperature: %.2f, humidity: %.2f, try: %d\n",
        t, temp, humid, i);

        // remove first buffer element if full
        time_t tmp_t;
        float  tmp_temp, tmp_humid;
        if (buffer_time.isFull()) {
            buffer_time.pull(&tmp_t);
            buffer_temp.pull(&tmp_temp);
            buffer_humid.pull(&tmp_humid);
        }

        // add values to buffer
        buffer_time.add(t);
        buffer_temp.add(temp);
        buffer_humid.add(humid);

        // activate rain
        if (settings_rain.initialized && humid < settings_rain.threshold) {
            debug.printf("Humidity is under threshold of (%.2f < %.2f). Start rain for %d seconds\n",
                          humid, settings_rain.threshold, settings_rain.duration);
            if((millis() - settings_rain.last_rain) / 1000.0f > settings_rain.minGap){
                letItRain();
                settings_rain.last_rain = millis();
            } else {
                debug.printf("Rain gap of %ds prevents rain (have to wait for %ds)",
                settings_rain.minGap, settings_rain.minGap - (millis() - settings_rain.last_rain));
            }
        }
    }
}

/**************************************
 *
 * Start rain / whatever for a given duration
 *
 *************************************/
void letItRain() {
    if (settings_rain.initialized) {
        debug.printf("Let it rain for %d seconds\n", settings_rain.duration);
        digitalWrite(settings_rain.pin, settings_rain.inverted ? LOW : HIGH);
        Alarm.timerOnce(settings_rain.duration, stopRain);
    }
}

void stopRain() {
    if (settings_rain.initialized) {
        debug.println("Stopping rain...");
        digitalWrite(settings_rain.pin, settings_rain.inverted ? HIGH : LOW);
    }
}

/**************************************
 *
 * Create mock values for chart test
 *
 *************************************/
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


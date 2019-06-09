## ESP32 terrarium controller

With this project you can control your terrarium's temperature, light, sprinkler and so on via webinterface.

### Features

* Sensors for temperature and humidity
* Timers for day/night light switch
* Starts sprinkler if humidity is under threshold
* 24h log of temperature and humidity via Webinterface
* OTA update via WiFi

![webinterface](doc/webinterface01.png "Screenshot of webinterface")

### Parts needed

* ESP32 controller
* DHT22 or similar DHT temperature/humidity sensor
* Relay board to control lights/pump
* MOSFET board to control LED brightness, fan speed etc. via PWM
* WiFi connection

The device creates an access point on first start. Then you can configure your WiFi and pin settings.

### Caution

The WiFi password is saved without encryption on the device. 
Also OTA-update has default password "admin" at the moment.
Change it if you want to secure your device for unauthorized updates.

**Make the webinterface accessible from internet is a very bad idea!**

### Installation

* Download / Clone this repository
* Install PlatformIO: https://docs.platformio.org/en/latest/ide/pioide.html
* Open this project in PlatformIO
* Connect your ESP32 via USB cable and upload sketch and [filesystem](https://docs.platformio.org/en/latest/platforms/espressif32.html#uploading-files-to-file-system-spiffs) (Run task -> Upload file system image)
* Reboot ESP32, it will create an AccessPoint with name ESP_xxxx
* connect to the AP and open http://192.168.4.1 in Browser
* Connect to your WiFi network

### Problems
* If PlatformIO can't find the libraries, open a terminal and type `pio lib install`. The command should download all necessary libraries.

### ToDo

* make more dynamic config site
* webinterface with authorization
* better instructions
* create PCB with relays, MOSFETS, power supply...

![terrarium](doc/terrarium01.jpg "Terrarium")

![terrarium](doc/terrarium02.jpg "Terrarium")

![terrarium](doc/terrarium03.jpg "Terrarium")

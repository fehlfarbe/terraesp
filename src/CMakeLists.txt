cmake_minimum_required(VERSION 2.8.4)
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/cmake/ArduinoToolchain.cmake)
#set(CMAKE_CXX_SYSROOT_FLAG_CODE "list(APPEND CMAKE_CXX_SOURCE_FILE_EXTENSIONS ino)")

set(PROJECT_NAME terraesp)
project(${PROJECT_NAME})

set(SOURCE ${SOURCE} terraesp.ino)
set_source_files_properties(terraesp.ino PROPERTIES LANGUAGE CXX)

#target_sources(${PROJECT_NAME} ${SOURCE})

#### Uncomment below additional settings as needed.
#set(${CMAKE_PROJECT_NAME}_BOARD esp32)
#set(${CMAKE_PROJECT_NAME}_PORT /dev/ttyACM0)
#set(esp32.build.mcu esp32)
#set(esp32.upload.protocol wiring)
#set(esp32.upload.speed 921600)

include_directories(.
        /home/kolbe/Arduino/hardware/espressif/esp32/libraries
        /home/kolbe/Arduino/libraries
        /home/kolbe/Arduino/libraries/DHT)

#generate_arduino_firmware(${CMAKE_PROJECT_NAME})

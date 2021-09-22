## ESP-IDF PSRamFS vfs example


1) cd to the example folder

    cd examples/esp-idf-psramvfs


2) create the components folder

    mkdir components

3) pull the library

    git clone https://github.com/tobozo/ESP32-PsRamFS --branch=1.0.3-beta components/ESP32-PsRamFS

4) setup the project

    idf.py set-target esp32

5) make sure psram is enabled

    idf.py menuconfig
    # Component config -> ESP32-specific -> Support for external, SPI-connected RAM = yes

6) build and flash

    idf.py build flash monitor

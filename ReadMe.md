# ESP32-PsRamFS

Coding Horror
-------------

This is a very early version of ESP32-PsRamFS: a wannabee RamDisk library.

It provides a fs::FS style filesystem using the available psram.

At the time of committing the code, only basic tests have been performed, few
things are still unimplemented (directory support is ... meh) and the API is
still under development.


Hardware Requirements
--------

ESP32 with psram


Credits:

- [espressif](https://github.com/espressif)
- [Ivan Grokhotkov](https://github.com/igrr)
- [Bill Greiman](https://github.com/greiman/RamDisk)
- [lorolol](https://github.com/lorol)


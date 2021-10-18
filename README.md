notifier
========

![Notification](res/img/screenshot.png)

A program that fetches notifications from given sources and displays them on desktop.  
**Note: The program is currently in early stages of development, and breaking changes may occur frequently.**

## Availability
Currently the program works only on Linux.

## Building
Required dependencies:
- GCC
- CMake
- make
- iniparser
- libcurl

Optional dependencies:
- libnotify - for libnotify display
- json-c - for GitHub/ISOD/Twitch modules
- libxml2 - for RSS module

Linux:
```
git clone https://github.com/gardockt/notifier.git
cd notifier
mkdir build
cd build
cmake ..
make
```

## Configuration
Configuration guide is available on [wiki](https://github.com/gardockt/notifier/wiki/Configuration).

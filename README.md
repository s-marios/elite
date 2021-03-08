# ELITE 
An ECHONET Lite implementation for embedded devices. Uses FreeRTOS and
the lwip networking stack.

This library has been tested on two platforms so far, the ESP8266 from espressif
and the FRDM-K64F from NXP. Used with FreeRTOS 9.0/10.x and lwip 2.1/2.x.

# Using Elite in your project
This is a lightweight library implementing the core functionality necesasry to
build ECHONET Lite devices using embedded systems. As build environments for
embedded devices vary wildly, adapt this code to your needs. You may have to
adjust include paths, various headers and/or certain basic data types.

The esp8266 directory contains older code specific to that platform that may
still be relevant.

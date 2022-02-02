# ELITE

A library for implementing ECHONET Lite devices.

## Supported Environments

### Embedded Devices

This library can be used with embedded devices that support FreeRTOS and
the lwip networking stack. So far it has been tested on two platforms,
the ESP8266 from espressif and the FRDM-K64F from NXP (FreeRTOS 9.0/10.x and
lwip 2.1/2.x).

In principle, any embedded device that provides support for a standard library,
FreeRTOS and lwip should be sufficient.

### Linux

The library is usable in Linux, with the user responsible for initializing
a multicast socket and joining the corresponding ECHONET Lite multicast
group.

## Using Elite in your Project

This is a lightweight library implementing the core functionality necesasry to
build ECHONET Lite devices. As build environments for embedded devices vary,
adapt this code to your needs. You may have to adjust include paths, headers
and/or certain basic data types.

## Documentation

This project uses Doxygen for generating documentation. Generate using:

```
doxygen Doxyfile
```

Refer to that for an overview regarding the usage of the library as well as
detailed developer documentation.

## Deprecated and Older Functionality

The esp8266 directory contains older code specific to that platform that may
still be relevant.

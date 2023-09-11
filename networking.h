#ifndef NETWORKING_H
#define NETWORKING_H

//common definitions here
#define ELITE_PORT 3610
#define ELITE_MADDR "224.0.23.0"

#ifdef __unix
//unix/linux stuff here

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>

#else
//microcontroller stuff here
#include "lwipopts.h"

#include "lwip/sockets.h"

#endif

#endif

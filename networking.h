#ifndef NETWORKING_H
#define NETWORKING_H

//common definitions here
#define ELITE_PORT 3610


#ifdef __unix__
//unix/linux stuff here

#define _DEFAULT_SOURCE 1
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#define ELITE_MADDR "224.0.23.0"


#else
//microcontroller stuff here

#include "lwipopts.h"
#include "lwip/sockets.h"

#endif

#endif

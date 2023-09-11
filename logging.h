#ifndef LOGGING_H
#define LOGGING_H

#ifdef __unix
//just to get us working on linux
#include <stdio.h>

#elif ESP_PLATFORM
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#endif

#else
// Microcontroller environment, hard requirements
// is FreeRTOS with semaphores
#include "FreeRTOS.h"
#include "semphr.h"
#endif

/**
 * Debug printfs, default level 1. Debug level of 0 (zero) suppresses all output.
 * Debug level of 1 enables PRINTF, level 2 also enables PPRINTF.
 */

#ifndef ELITE_DEBUG
#define ELITE_DEBUG 1
#endif

#if ELITE_DEBUG == 0
#define PRINTF(...)
#define PPRINTF(...)

#endif

#if ELITE_DEBUG == 1
#define PRINTF printf
#define PPRINTF(...)
#endif

#if ELITE_DEBUG > 1
#define PRINTF printf
#define PPRINTF printf
#endif

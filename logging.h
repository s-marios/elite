#ifndef LOGGING_H
#define LOGGING_H

#ifdef __unix
//just to get us working on linux

#include <stdio.h>
#ifndef PRINTF
#define PRINTF printf

#endif

#ifndef PPRINTF
#define PPRINTF printf
#endif

#else
// Microcontroller environment, hard requirements
// is FreeRTOS with semaphores
#include "FreeRTOS.h"
#include "semphr.h"

/**
 * Debug printfs. Debug level of 0 (zero) suppresses all output. Debug level of
 * 1 enables PRINTF, 2 enables PPRINTF also.
 */
//#define ELITE_DEBUG 2
//#define PPRINTF printf
#define PPRINTF(...)
/**
 * if defined, web log functionality at port 6666 is enabled.
 */
//#define DOWEBLOG

#ifdef DOWEBLOG


extern uint8_t ndsize; /**< last weblog data size */
extern char ndbuf[256]; /**< buffer holding the data to be sent */
/** semaphore protecting simultaneous writes */
extern SemaphoreHandle_t debugdowrite;
/** semaphore used to signal  the performance of a write */
extern SemaphoreHandle_t debugsem;

/** the web logging macro, usually mapped to PPRINTF */
#define WEBLOG(...) do { \
	if (debugsem) { \
		xSemaphoreTake(debugdowrite, portMAX_DELAY); \
		ndsize = sprintf(ndbuf, __VA_ARGS__);	\
		xSemaphoreGive(debugsem); \
		xSemaphoreGive(debugdowrite); \
	}\
} while (0)

#else
#define WEBLOG
#endif //DOWEBLOG

#ifdef ELITE_DEBUG
#undef PRINTF
#ifdef DOWEBLOG
#define PRINTF WEBLOG
#else
#define PRINTF printf
#endif	//DOWEBLOG
#endif //ELITE_DEBUG
#if ELITE_DEBUG > 1
#undef PPRINTF
#ifdef DOWEBLOG
#define PPRINTF WEBLOG
#else
#define PPRINTF printf
#endif
#endif

#ifndef PRINTF
#define PRINTF printf
#endif

#endif
#endif

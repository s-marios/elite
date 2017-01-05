/**
 * \file
 *  elite FreeRTOSConfig overrides.
 *
 * This is intended as an example of overriding some of the default FreeRTOSConfig settings,
 * which are otherwise found in FreeRTOS/Source/include/FreeRTOSConfig.h
*/

/* We sleep a lot, so cooperative multitasking is fine. \todo or is it??? */
#define configUSE_PREEMPTION 0

/* Aggresively small stacks */
#define configMINIMAL_STACK_SIZE 128

/* Support for dynamic allocation, enables vSemaphoreCreateBinary */
#define configSUPPORT_DYNAMIC_ALLOCATION 1

/* sleep endlessly waiting for mutexes */
#define INCLUDE_vTaskSuspend 1

/* Use the defaults for everything else */
#include_next<FreeRTOSConfig.h>


/* The classic "blink" example
 *
 * This sample code is in the public domain.
 */
#include <stdlib.h>
#include <malloc.h>
#include "espressif/esp_common.h"
#include "esp/uart.h"
#include "FreeRTOS.h"
#include "task.h"
#include "esp8266.h"
#include "esp/gpio.h"
#include "elite.h"
#include "minunit.h"

int tests_run = 0;

static char * test_allocateFrame(){
	ECHOFRAME_PTR fptr = allocateFrame();
	mu_assert("frame allocation failed", fptr != NULL);
	mu_assert("allocated frame size wrong", fptr->allocated == ECHOFRAME_STDSIZE);
	free(fptr);
	return 0;
}

static char * test_malloc64(){
	void * malloced = malloc(64);
	mu_assert("malloc 64 bytes failed.", malloced != NULL );
	free(malloced);
	return 0;
}

static char * test_malloc32(){
	void * malloced = malloc(32);
	mu_assert("malloc 32 bytes failed.", malloced != NULL );
	free(malloced);
	return 0;
}

static char * test_malloc1(){
	void * malloced = malloc(1);
	mu_assert("malloc 1 bytes failed.", malloced != NULL );
	free(malloced);
	return 0;
}


static char * allTests(){
	mu_run_test(test_malloc1);
	mu_run_test(test_malloc32);
	mu_run_test(test_malloc64);
	mu_run_test(test_allocateFrame);
	return 0;
}

void runTestsTask(void * params){
	void * malloced = malloc(1);
	printf("is malloced? : %s ", malloced? "true" : "false");
	printf("Sizeof ECHOFRAME: %d\n", sizeof(ECHOFRAME));
	printf("running tests... \n");
	char * result = allTests();
	if (result != 0){
		printf("%s\n", result);
	} else {
		printf("ALL TESTS PASSED.");
	}
	while(1) {
	        vTaskDelay(1000 / portTICK_RATE_MS);
	        printf(".");
	    }
}

void user_init(void)
{
    uart_set_baud(0, 115200);
    xTaskCreate(runTestsTask, (signed char *)"runTestsTask", 256, NULL, 2, NULL);
}

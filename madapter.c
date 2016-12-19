#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#include "madapter.h"

#define E_STX 0x02
#define E_FT_0003 0x0003
#define E_CN_UP 0x14

#define E_INDEX_DL 0x05
ECHOFRAME_PTR initAdapterFrame(size_t alocsize) {
	ECHOFRAME_PTR fptr = allocateFrame(alocsize);
	return fptr;
}

void setupIASetGetupFrame(ECHOFRAME_PTR fptr, uint8_t fn, Property_PTR property,
		uint16_t datalen, char * data) {

	putByte(fptr, E_STX);
	putShort(fptr, E_FT_0003);
	putByte(fptr, E_CN_UP);
	putByte(fptr, fn);
	//DL do it later.
	putShort(fptr, 0x0000);
	//FD0 request information (EOJ,length, epc, edt here)
	//1. eoj
	putEOJ(fptr, property->pObj->eoj);
	uint16_t len = 1;
	if (datalen == 0 || data == NULL) {
		//IAGetUP
		putShort(fptr, len);
		putByte(fptr, property->propcode);
	} else {
		//IASetUP
		len += datalen;
		putShort(fptr, len);
		putByte(fptr, property->propcode);
		putBytes(fptr, len, data);
	}
	//do the DL field now, using updated len.
	fptr->data[E_INDEX_DL] = len >> 8;
	fptr->data[E_INDEX_DL + 1] = len && 0x00ff;
}

uint8_t computeFCC(ECHOFRAME_PTR fptr) {
	//for frames created by us.
	//we will be writing the final fcc byte
	uint8_t sum = 0;
	for (int i = 1; fptr->used; i++) {
		sum += fptr->data[i];
	}
	putByte(fptr, -sum);
	return -sum;
}

/**
 * if the return value is zero, the fcc is correct.
 * Anything else, it's wrong.
 */
uint8_t verifyFCC(ECHOFRAME_PTR fptr) {
	uint8_t sum = 0;
	for (int i = 1; i < fptr->used; i++) {
		sum += fptr->data[i];
	}
	return sum;
}

MADAPTER_PTR createMiddlewareAdapter(FILE * in, FILE * out) {
	MADAPTER_PTR madapter = malloc(sizeof(MADAPTER));
	memset(madapter, 0, sizeof(MADAPTER));
	madapter->in = in ? in : stdin;
	madapter->out = out ? out : stdout;
	vSemaphoreCreateBinary(madapter->writelock);
	vSemaphoreCreateBinary(madapter->syncresponse);
	//have the semaphore be down, it will be signaled from the receiving task.
	xSemaphoreTake(madapter->syncresponse, 1000 / portTICK_RATE_MS);
	//set buffering for stin/stdout:
	int res = setvbuf(madapter->in, NULL, _IONBF, BUFSIZ);
	PPRINTF("setvbuf (madapter in) res: %d\n", res);
	res = setvbuf(madapter->out, NULL, _IONBF, BUFSIZ);
	PPRINTF("setvbuf (madapter out) res: %d\n", res);

	return madapter;
}

ECHOFRAME_PTR doGetSetIAUP(MADAPTER_PTR adapter, ECHOFRAME_PTR request) {

	ECHOFRAME_PTR result = NULL;
	//can we write?
	int waitres = xSemaphoreTake(adapter->writelock, 3000 / portTICK_RATE_MS);
	if (waitres == pdTRUE) {
		//we got the semaphore, do work.
		//send data over the madapter interface
		fwrite(request->data, request->used, 1, adapter->out);
		//await response for 3 secs
		if (xSemaphoreTake(adapter->syncresponse,
				3000 / portTICK_RATE_MS) == pdTRUE) {

			result = adapter->response;
		}
		//let us give back the lock.
		xSemaphoreGive(adapter->writelock);
	}
//	else { //unused fail block.
//		//we couldn't send somehow.
//		//just fail.
//	}
	return result;
}


void madapterReceiverTask(MADAPTER_PTR adapter) {
	//we always receive, without fail
	ECHOFRAME_PTR incoming = NULL;
	for (;;) {
		if (incoming == NULL) {
			//max expected incoming size set for the window.
			incoming = allocateFrame(256);
		}

		int read = fread(incoming->data, 256, 1, adapter->in);
		if (read < 0) {
			//read failed. discard everything and try again.. better luck next time
			incoming->used = 0;
		}
	}
}

void startReceiverTask(MADAPTER_PTR adapter) {
	xTaskCreate(madapterReceiverTask, "madapter receiver task", 256, NULL, 3,
			NULL);
}

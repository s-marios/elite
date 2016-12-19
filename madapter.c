#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#include "madapter.h"

#define E_STX 0x02
#define E_FT_0003 0x0003
#define E_CN_UP 0x10
#define E_CN_UPRESP 0x90
#define E_CN_NREQ 0x11
#define E_CN_NRESP 0x91

#define E_INDEX_DL 0x05
ECHOFRAME_PTR initAdapterFrame(size_t alocsize, uint16_t ft, uint8_t cn,
		uint8_t fn) {
	ECHOFRAME_PTR fptr = allocateFrame(alocsize);
	putByte(fptr, E_STX);
	putShort(fptr, ft);
	putByte(fptr, cn);
	putByte(fptr, fn);
	return fptr;
}

ECHOFRAME_PTR setupIASetGetupFrame(uint8_t fn, Property_PTR property,
		uint16_t datalen, char * data) {

	ECHOFRAME_PTR fptr = initAdapterFrame(7 + datalen + 1, E_FT_0003, E_CN_UP,
			fn);
	if (fptr == NULL) {
		return NULL;
	}
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
	return fptr;
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
	madapter->FN = 1;
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

/**
 * This is the inner lock where we block during communication.
 * We calculate FCC!!
 * caller responsible for freeing request response
 */
ECHOFRAME_PTR doSendReceive1(MADAPTER_PTR adapter, ECHOFRAME_PTR request) {

	ECHOFRAME_PTR result = NULL;
	//can we write?
	int waitres = xSemaphoreTake(adapter->writelock, 3000 / portTICK_RATE_MS);
	if (waitres == pdTRUE) {
		//we got the semaphore, do work.
		computeFCC(request);
		//setup our request for matching purposes
		adapter->request = request;
		//send data over the madapter interface
		fwrite(request->data, request->used, 1, adapter->out);
		//await response for 3 secs
		if (xSemaphoreTake(adapter->syncresponse,
				3000 / portTICK_RATE_MS) == pdTRUE) {

			result = adapter->response;
		}
		//we're done, remove request/response here
		adapter->request = NULL;
		adapter->response = NULL;
		//let us give back the lock.
		xSemaphoreGive(adapter->writelock);
	}
//	else { //unused fail block.
//		//we couldn't send somehow.
//		//just fail.
//	}
	return result;
}

#define PR_OK 0
#define PR_EMPTY 1
#define PR_NOSTX 2
//header incomplete
#define PR_HEADER 4
//suspiciously long DL
#define PR_BADDL 8
//incomplete data
#define PR_FDINCMPLT 16
#define PR_FDLONG 32
#define PR_FCC 64

#define E_OFF_FT 1
#define E_OFF_CN 3
#define E_OFF_FN 4
#define E_OFF_DL 5
#define E_OFF_FDZERO 7

int parseAdapterFrame(ECHOFRAME_PTR frame) {
	//checks
	if (frame == NULL || frame->used == 0) {
		return PR_EMPTY;
	}
	if (frame->data[0] != 0x02) {
		return PR_NOSTX;
	}
	if (frame->used < 7) {
		return PR_HEADER;
	}
	//we have the first 7 bytes.. check length
	uint16_t dl = getShort(frame, E_OFF_DL);
	if (dl > 256) {
		//currently we don't do packets that big.
		return PR_BADDL;
	}
	uint16_t framesize = E_OFF_FDZERO + dl + 1; //header + data + fcc
	if (framesize > frame->used) {
		//incoming frame must be longer but it's short. data still missing
		return PR_FDINCMPLT;
	}
	if (framesize < frame->used) {
		//the data stored are more than what should have been
		//too long
		return PR_FDLONG;
	}
	//framesize == frame->used
	if (verifyFCC(frame)) {
		//FCC IS BAD
		return PR_FCC;
	}
	//looks pretty fine to me!
	return PR_OK;
}

void processFrame(MADAPTER_PTR adapter, ECHOFRAME_PTR frame);

void madapterReceiverTask(MADAPTER_PTR adapter) {
	//we always receive, without fail
#define MADAPTER_BUFSIZ 255
	ECHOFRAME_PTR incoming = NULL;
	for (;;) {
		if (incoming == NULL) {
			//max expected incoming size set for the window.
			incoming = allocateFrame(MADAPTER_BUFSIZ);
		}
		int read = fread(&ECHOFRAME_HEAD(incoming),
		MADAPTER_BUFSIZ - incoming->used, 1, adapter->in);
		if (read < 0) {
			//read failed. discard everything and try again.. better luck next time
			incoming->used = 0;
			continue;
		} else {
			//we read something.
			incoming->used += read;
			int parseres = parseAdapterFrame(incoming);
			PPRINTF("madapter parse result: %d\n", parseres);
			if (parseres == PR_OK) {
				//PROCESS THE DAMN THING!
				processFrame(adapter, incoming);
				//trigger frame creation in the next loop..
				incoming = NULL;
			} else if (parseres & (PR_HEADER | PR_FDINCMPLT)) {
				//short header or incomplete fd data.. wait
				continue;
			} else {
				//burn everything
				incoming->used = 0;
			}
		}

	}
}

void processNormalCommunicationFrame(MADAPTER_PTR adapter, ECHOFRAME_PTR frame);

void processFrame(MADAPTER_PTR adapter, ECHOFRAME_PTR frame) {
	//we have a valid frame, try and do things with it.

	//1. discover what type of frame we have.
	uint16_t ft = getShort(frame, E_OFF_FT);
	switch (ft) {
	case 0x0003:
		processNormalCommunicationFrame(adapter, frame);
		break;

	default:
		PPRINTF("madapter: can't handle frame with type: %d\n", ft);
		break;
	}
}

void matchIAResponse(MADAPTER_PTR adapter, ECHOFRAME_PTR frame);
void handleNotifyRequest(MADAPTER_PTR adapter, ECHOFRAME_PTR frame);
void processNormalCommunicationFrame(MADAPTER_PTR adapter, ECHOFRAME_PTR frame) {
	uint8_t cn = frame->data[E_OFF_CN];
	switch (cn) {
	case E_CN_UPRESP:
		matchIAResponse(adapter, frame);
		break;
	case E_CN_NREQ:
		handleNotifyRequest(adapter, frame);
		break;
	default:
		PPRINTF("processNCF: CN %d\n", cn);
		//frame handled. free it.
		freeFrame(frame);

	}
}

void matchIAResponse(MADAPTER_PTR adapter, ECHOFRAME_PTR frame) {
	//TODO: be more thorough with the matching. currently only fn
	if (adapter->request
			&& adapter->request->data[E_OFF_FN] == frame->data[E_OFF_FN]) {
		//we have a match.
		adapter->response = frame;
		xSemaphoreGive(adapter->syncresponse);
		//and we're done! client frees the memory.
	}
	//in all other cases...
	//no match/fail/whatever. Free the packet.
	freeFrame(frame);
}

void handleNotifyRequest(MADAPTER_PTR adapter, ECHOFRAME_PTR frame) {
	//TODO 1. send response back.
	//WE NEVER FAIL. NEVER.
	uint8_t fn = frame->data[E_OFF_FN];
	ECHOFRAME_PTR response = initAdapterFrame(13, E_FT_0003, E_CN_NRESP, fn);
	putShort(response, 0x0005);
	putEOJ(response, &frame->data[E_OFF_FDZERO]);
	computeFCC(response);
	//pray...
	fwrite(response->data, response->used, 1, adapter->out);

	//TODO 2. perform actual notification
	if (!adapter->context) {
		PPRINTF("handleNR: assert fails, init your context\n");
	}
	PPRINTF("TODO: perform the actual notifications from the adapter\n");

}

#define E_OFF_RES 10
#define E_OFF_RESLEN 12
#define E_OFF_RESEDT 14

/**
 * todo SEMANTICS?! what is this buff?! for read response?
 */
int doIAGetup(Property_PTR property, uint8_t size, char * buf) {

	MADAPTER_PTR adapter = (MADAPTER_PTR) property->opt;
	uint8_t fn;
	getFN(adapter, fn);

	//TODO: check size and len param (uint8_t vs uint16_t)
	ECHOFRAME_PTR request = setupIASetGetupFrame(fn, property, 0, NULL);
	//we got our request, send it
	ECHOFRAME_PTR response = doSendReceive1(adapter, request);
	int result = 0;
	if (response) {
		//we had a read response, disect it and extract data.
		uint16_t resresult = getShort(response, E_OFF_RES);
		if (resresult == 0) {
			//success, copy data to buffer.
			uint16_t reslen = getShort(response, E_OFF_RESLEN);
			//TODO handle insufficient buffer size
			memcpy(buf, &request->data[E_OFF_RESEDT], reslen);
			result = reslen;
		}
	}
	//free all
	freeFrame(response);
	freeFrame(request);
	return result;
}

int doIASetup(Property_PTR property, uint8_t size, char * buf) {
	MADAPTER_PTR adapter = (MADAPTER_PTR) property->opt;
	uint8_t fn;
	getFN(adapter, fn);

	ECHOFRAME_PTR request = setupIASetGetupFrame(fn, property, size, buf);
	ECHOFRAME_PTR response = doSendReceive1(adapter, request);

	int result = 0;
	if (response) {
		uint16_t resresult = getShort(response, E_OFF_RES);
		if (resresult == 0) {
			result = size;
		}
	}
	freeFrame(response);
	freeFrame(request);
	return result;
}

Property_PTR createIAupProperty(uint8_t propcode, uint8_t rwn,
		MADAPTER_PTR adapter) {
	Property_PTR property = createProperty(propcode, rwn);
	if (property == NULL) {
		return NULL;
	}
	if (property->rwn_mode & E_READ) {
		property->readf = doIAGetup;
	}
	if (property->rwn_mode & E_WRITE) {
		property->writef = doIAGetup;
	}
	property->opt = adapter;
	return property;
}

void startReceiverTask(MADAPTER_PTR adapter) {
	xTaskCreate(madapterReceiverTask, "madapter receiver task", 256, NULL, 3,
			NULL);
}

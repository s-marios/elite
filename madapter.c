#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include "espressif/esp_common.h"
#include "esp/uart.h"

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

uint8_t computeFCC(ECHOFRAME_PTR fptr);
int parseAdapterFrame(ECHOFRAME_PTR frame);

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

	//size: standard header + (6 + datalen) bytes request + 1 FCC
	ECHOFRAME_PTR fptr = initAdapterFrame(7 + 6 + datalen + 1,
	E_FT_0003,
	E_CN_UP,
			fn);
	if (fptr == NULL) {
		return NULL;
	}
	//DL do it NOW. 6 is fd header info (eoj, len, epc) + n data
	putShort(fptr, 6 + datalen);
	//FD0 request information (EOJ,length, epc, edt here)
	//1. eoj
	putEOJ(fptr, property->pObj->eoj);
	if (datalen == 0 || data == NULL) {
		//IAGetUP
		putShort(fptr, 1);
		putByte(fptr, property->propcode);
	} else {
		//IASetUP
		putShort(fptr, 1 + datalen);
		putByte(fptr, property->propcode);
		putBytes(fptr, datalen, data);
	}
	computeFCC(fptr);
	return fptr;
}

uint8_t computeFCC(ECHOFRAME_PTR fptr) {
	//for frames created by us.
	//we will be writing the final fcc byte
	uint8_t sum = 0;
	for (int i = 1; i < fptr->used; i++) {
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
	//set buffering for io:
	int res = setvbuf(madapter->in, NULL, _IONBF, BUFSIZ);
	//PPRINTF("setvbuf (madapter in) res: %d\n", res);
	res = setvbuf(madapter->out, NULL, _IONBF, BUFSIZ);
	//PPRINTF("setvbuf (madapter out) res: %d\n", res);
	PPRINTF("adapter created\n");
	return madapter;
}

void sendOverSerial(MADAPTER_PTR adapter, ECHOFRAME_PTR outgoing) {

	for (int i = 0; i < outgoing->used; i++) {
		//stdout for the time being
		putc(outgoing->data[i], stdout);
	}
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
		PPRINTF("dSR1: send\n");
		//we got the semaphore, do work.
		//setup our request for matching purposes
		adapter->request = request;
		//int pres = parseAdapterFrame(request);
		//PPRINTF("dsr1: request parse result: %d\n", pres);
		//send data over the madapter interface
		fwrite(request->data, 1, request->used, adapter->out);
		//await response for 3 secs
		if (xSemaphoreTake(adapter->syncresponse,
				1000 / portTICK_RATE_MS) == pdTRUE) {

			result = adapter->response;
			PPRINTF("dSR1: GOT RESPONSE!\n");
		} else {
			PPRINTF("dSR1: NO RESPONSE \n");
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

//parse results
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

//standard adapter header
#define E_OFF_FT 1
#define E_OFF_CN 3
#define E_OFF_FN 4
#define E_OFF_DL 5
#define E_OFF_FDZERO 7

//response offsets for... cn 90
#define E_OFF_RES 10
#define E_OFF_RESLEN 12
#define E_OFF_RESEPC 14
#define E_OFF_RESEDT 15

//request offsets for.... cn11 (NREQ)
#define E_OFF_NREQ_LEN 10
#define E_OFF_NREQ_EPC 12
#define E_OFF_NREQ_EDT 13

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
		PPRINTF("mad parse: badDL %d\n", dl);
		return PR_BADDL;
	}
	uint16_t framesize = E_OFF_FDZERO + dl + 1; //header + data + fcc
	if (frame->used < framesize) {
		//incoming frame must be longer but it's short. data still missing
		//PPRINTF("mad parse: incomplete frame, length: %d\n", frame->used);
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
		PPRINTF("mad parse: BAD FCC\n");
		return PR_FCC;
	}
	//looks pretty fine to me!
	return PR_OK;
}

void processFrame(MADAPTER_PTR adapter, ECHOFRAME_PTR frame);

void madapterReceiverTask(void * pvParameters) {
	MADAPTER_PTR adapter = (MADAPTER_PTR) pvParameters;
	portTickType prev;
	portTickType now;
	prev = xTaskGetTickCount() / portTICK_RATE_MS;

	//we always receive, without fail
#define MADAPTER_BUFSIZ 255
	PPRINTF("madapterReceiverTask started. adapter NULL? %s\n", adapter?"false":"true");
	ECHOFRAME_PTR incoming = NULL;
	for (;;) {
		if (incoming == NULL) {
			//max expected incoming size set for the window.
			incoming = allocateFrame(MADAPTER_BUFSIZ);
		}
		//PPRINTF("mad read\n");
		//everything commented out crashes the mcu
		//int readbytes = fread(
		//		cbuf, 1,
		//		&incoming->data[incoming->used], sizeof(char),
		//		MADAPTER_BUFSIZ - incoming->used, adapter->in);
		//int rbyte = getc(adapter->in);
		//int rbyte = getc(stdin);
		//int rbyte = fgetc_unlocked(stdin);
		int rbyte = getc(stdin);
		/* if we have mroe than 300 ms since last reception, reset everything*/
		now = xTaskGetTickCount() * portTICK_RATE_MS;
		if (now - prev > 100) {
			incoming->used = 0;
			PPRINTF("RESET BUFFER\n\n");
		}
		//PPRINTF("tick: %d %d \n", prev, now);
		prev = now;

		//process incoming byte
		if (rbyte < 0) {
			PPRINTF(" d ");
			//read failed. discard everything and try again.. better luck next time
			incoming->used = 0;
			uart_clear_rxfifo(0);
		} else {
			//PPRINTF("r");
			//we read something.
			//incoming->used += readbytes;
			putByte(incoming, rbyte);
			int parseres = parseAdapterFrame(incoming);
			if (parseres == PR_OK) {
				PPRINTF("madpr: parse ok!\n");
				//PROCESS THE DAMN THING!
				processFrame(adapter, incoming);
				//frame freed by the processors.. hopefully..
				//trigger frame creation in the next loop..
				incoming = NULL;
				continue;
			} else if (parseres & (PR_HEADER | PR_FDINCMPLT)) {
				//short header or incomplete fd data.. wait
				continue;
			}
			PPRINTF("madpr: %d bytes: %d\n", parseres, incoming->used);
			//burn everything
			incoming->used = 0;
			uart_clear_rxfifo(0);

		}
	}
}

void processNormalCommunicationFrame(MADAPTER_PTR adapter,
		ECHOFRAME_PTR incoming);

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

void matchIAResponse(MADAPTER_PTR adapter, ECHOFRAME_PTR incoming);
void handleNotifyRequest(MADAPTER_PTR adapter, ECHOFRAME_PTR request);
void processNormalCommunicationFrame(MADAPTER_PTR adapter,
		ECHOFRAME_PTR incoming) {
	uint8_t cn = incoming->data[E_OFF_CN];
	switch (cn) {
	case E_CN_UPRESP:
		matchIAResponse(adapter, incoming);
		break;
	case E_CN_NREQ:
		handleNotifyRequest(adapter, incoming);
		break;
	default:
		PPRINTF("processNCF: CN %d\n", cn);
		//frame handled. free it.
		freeFrame(incoming);
	}
}

void matchIAResponse(MADAPTER_PTR adapter, ECHOFRAME_PTR incoming) {
	//TODO: be more thorough with the matching. currently only fn
	PPRINTF("IN MATCH IA RESPONSE\n");

	if (adapter->request
			&& adapter->request->data[E_OFF_FN] == incoming->data[E_OFF_FN]) {
		//we have a match in terms of frame number.
		adapter->response = incoming;
		xSemaphoreGive(adapter->syncresponse);
		//and we're done! client frees the memory.
	}
	//in all other cases...
	//no match/fail/whatever. Free the packet.
	freeFrame(incoming);
}

void handleNotifyRequest(MADAPTER_PTR adapter, ECHOFRAME_PTR request) {

	//TODO 1. send response back.
	//PPRINTF("IN HNR\n\n");
	//WE NEVER FAIL. NEVER.
	uint8_t fn = request->data[E_OFF_FN];
	//PPRINTF("fn :%d\n", fn);
	ECHOFRAME_PTR response = initAdapterFrame(13, E_FT_0003, E_CN_NRESP, fn);

	//DL fixed to 5
	//PPRINTF("r");
	putShort(response, 0x0005);
	//FD0 Result: WE NEVER FAIL
	//PPRINTF("s");
	putShort(response, 0x0000);
	//FD1 copy eoj from request
	//PPRINTF("a");
	putEOJ(response, (unsigned char *) &request->data[E_OFF_FDZERO]);
	//PPRINTF("b");
	computeFCC(response);

	//PPRINTF("c");
	//pray...
	//	PPRINTF("NO FWRITE: used: %d fdout: %d\n\n", response->used, adapter->out);

	fwrite(response->data, 1, response->used, adapter->out);

	//////////////////////////////////
	//perform actual notification   //
	//////////////////////////////////
	if (!adapter->context) {
		PPRINTF("handleNR: assert fails, init your context\n");
	}
	ECHOFRAME_PTR
	nframe = initFrame(ECHOFRAME_MAXSIZE,
			incTID(adapter->context));
	putEOJ(nframe, &request->data[E_OFF_FDZERO]);
	putEOJ(nframe, (unsigned char *) PROFILEEOJ);
	putESVnOPC(nframe, ESV_INF);
	//-1: exclude the epc
	uint16_t datalen = getShort(request, E_OFF_NREQ_LEN) - 1;
	putEPC(nframe, request->data[E_OFF_NREQ_EPC], (uint8_t) datalen,
			&request->data[E_OFF_NREQ_EDT]);

	PPRINTF("Send notification... ");
	sendNotification(adapter->context, nframe);

	PPRINTF("HNR DONE\n");
	freeFrame(nframe);
	freeFrame(response);
	freeFrame(request);
}


/**
 * todo SEMANTICS?! what is this buff?! for read response?
 * what's going on!
 */
int doIAGetup(Property_PTR property, uint8_t size, char * buf) {
	PPRINTF("doIAGETUP\n");
	MADAPTER_PTR adapter = (MADAPTER_PTR) property->opt;
	uint8_t fn;
	getFN(adapter, fn);

	//TODO: check size and len param (uint8_t vs uint16_t)
	ECHOFRAME_PTR request = setupIASetGetupFrame(fn, property, 0, NULL);
	int preq = parseAdapterFrame(request);
	PPRINTF("dIAG: request parse result: %d\n", preq);
	if (preq) {
		int dl = getShort(request, E_OFF_DL);
		PPRINTF("req size (used, aloc, dl): %d %d %d\n",
				request->used,
				request->allocated,
				dl);
	}
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
			//DOING size is actually correctly calculated. use that.
			memcpy(buf, &response->data[E_OFF_RESEDT], reslen - 1);
			result = reslen - 1;
		} else {
			PPRINTF("doIAGETUP: bad response\n");
		}
	} else {
		PPRINTF("doIAGETUP: NULL response\n");
	}
	//free all
	freeFrame(response);
	freeFrame(request);

	PPRINTF("getup res: %d\n", result);
	return result;
}

/**
 * Semantics: what do we return? bytes written?
 */
int doIASetup(Property_PTR property, uint8_t size, char * buf) {
	PPRINTF("doIASETUP\n");
	MADAPTER_PTR adapter = (MADAPTER_PTR) property->opt;
	uint8_t fn;
	getFN(adapter, fn);

	ECHOFRAME_PTR request = setupIASetGetupFrame(fn, property, size, buf);
	ECHOFRAME_PTR response = doSendReceive1(adapter, request);

	int result = -1;
	if (response) {
		uint16_t resresult = getShort(response, E_OFF_RES);
		if (resresult == 0) {
			result = size;
		}
	}
	freeFrame(response);
	freeFrame(request);
	PPRINTF("setup res: %d\n", result);
	return result;
}

Property_PTR createIAupProperty(uint8_t propcode, uint8_t rwn,
		MADAPTER_PTR adapter) {
	//suppress notification generation for adapter properties
	Property_PTR property = createProperty(propcode, rwn | E_SUPPRESS_NOTIFY);
	if (property == NULL) {
		return NULL;
	}
	if (property->rwn_mode & E_READ) {
		property->readf = doIAGetup;
	}
	if (property->rwn_mode & E_WRITE) {
		property->writef = doIASetup;
	}
	property->opt = adapter;
	return property;
}

void startReceiverTask(MADAPTER_PTR adapter) {
	xTaskCreate(madapterReceiverTask, (signed char * )"madapterReceiverTask",
			2048, adapter, 1, NULL);
}

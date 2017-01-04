#ifndef MADAPTER_H
#define MADAPTER_H

//in order to reuse ECHOFRAME
#include "elite.h"

ECHOFRAME_PTR initAdapterFrame(size_t alocsize, uint16_t ft, uint8_t cn,
		uint8_t fn);

typedef struct {
	FILE * out;
	FILE * in;
	SemaphoreHandle_t writelock;
	SemaphoreHandle_t syncresponse;
	ECHOFRAME_PTR request;
	ECHOFRAME_PTR response;
	ECHOCTRL_PTR context;
	uint8_t FN;
} MADAPTER, *MADAPTER_PTR;

MADAPTER_PTR createMiddlewareAdapter(FILE * in, FILE * out);
#define setContext( x, cont) x->context = cont
#define getFN(x, fn) do { \
	fn = x->FN; \
	x->FN++; \
	x->FN ? x->FN : x->FN++ ; \
} while (0)

Property_PTR createIAupProperty(uint8_t propcode, uint8_t rwn,
		MADAPTER_PTR adapter);

void startReceiverTask(MADAPTER_PTR adapter);


#endif

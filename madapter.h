#ifndef MADAPTER_H
#define MADAPTER_H

//in order to reuse ECHOFRAME
#include "elite.h"

ECHOFRAME_PTR initAdapterFrame(size_t alocsize);

typedef struct {
	FILE * out;
	FILE * in;
	xSemaphoreHandle writelock;
	xSemaphoreHandle syncresponse;
	ECHOFRAME_PTR request;
	ECHOFRAME_PTR response;
} MADAPTER, *MADAPTER_PTR;

MADAPTER_PTR createMiddlewareAdapter(FILE * in, FILE * out);

#endif



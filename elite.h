#ifndef ELITE_H
#define ELITE_H

#include <stddef.h>

size_t test(void);


typedef struct {
	size_t used;
	size_t allocated;
	char * data;
} ECHOFRAME, * ECHOFRAME_PTR;

#define ECHOFRAME_STDSIZE 128
#define ECHOFRAME_MAXSIZE 256

ECHOFRAME_PTR allocateFrame(void);
ECHOFRAME_PTR allocateFrameWithSize(size_t alocsize);

#endif

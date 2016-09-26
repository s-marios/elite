#include "elite.h"
#include <malloc.h>

size_t test(void){
	return 4;
}


ECHOFRAME_PTR allocateFrame(void){
	return allocateFrameWithSize(ECHOFRAME_STDSIZE);
}
ECHOFRAME_PTR allocateFrameWithSize(size_t alocsize){
	ECHOFRAME_PTR fptr = (ECHOFRAME_PTR) malloc(alocsize + sizeof(ECHOFRAME_STDSIZE));
	fptr->allocated = alocsize;
	fptr->used = 0;
	fptr->data = ((char *) fptr) + sizeof(sizeof(ECHOFRAME));
	return fptr;
}

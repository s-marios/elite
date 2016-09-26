#include "elite.h"
#include <malloc.h>
#include <string.h>


size_t test(void){
	return 4;
}

ECHOFRAME_PTR allocateFrame(size_t alocsize){
	if (alocsize <= 0 || alocsize > ECHOFRAME_MAXSIZE){
		alocsize = ECHOFRAME_STDSIZE;
	}
	ECHOFRAME_PTR fptr = (ECHOFRAME_PTR) malloc(alocsize + sizeof(ECHOFRAME_STDSIZE));
	fptr->allocated = alocsize;
	fptr->used = 0;
	fptr->propNum = 0;
	fptr->data = ((char *) fptr) + sizeof(sizeof(ECHOFRAME));
	return fptr;
}
uint8_t incTID(ECHOCTRL_PTR cptr){
	uint8_t result = cptr->TID;
	cptr->TID++;
	if (cptr == 0){
		cptr->TID++;
	}
	return result;
}

ECHOCTRL_PTR createEchonetControl(){
	ECHOCTRL_PTR ecptr = malloc(sizeof(ECHOCTRL));
	ecptr->TID = 1;
	return ecptr;
}

int checkSize(ECHOFRAME_PTR fptr, size_t increase){
	return fptr->used + increase >= fptr->allocated;
}

int putBytes(ECHOFRAME_PTR fptr, uint8_t num, char * data){
	if (checkSize(fptr,num)) return -1;
	memmove(&ECHOFRAME_HEAD(fptr), data, num);
	fptr->used += num;
	return 0;
}

int putByte(ECHOFRAME_PTR fptr, char byte){
	if (checkSize(fptr,1)) return -1;
	ECHOFRAME_HEAD(fptr) = byte;
	fptr->used ++;
	return 0;
}

int putShort(ECHOFRAME_PTR fptr, uint16_t aShort){
	if (checkSize(fptr,2)) return -1;
	ECHOFRAME_HEAD(fptr) = aShort >> 8;
	ECHOFRAME_HEAD(fptr) = ((char) (0x00ff & aShort));
	fptr->used += 2;
	return 0;
}

int putEPC(ECHOFRAME_PTR fptr, uint8_t epc, uint8_t size, char * data){
	putByte(fptr, epc);
	putByte(fptr, size);
	putBytes(fptr, size, data);
	fptr->propNum++;
	return 0;
}

int putESVnOPC(ECHOFRAME_PTR fptr, uint8_t esv){
	putByte(fptr, esv);
	putByte(fptr, 0);
	return 0;
}

void finalizeFrame(ECHOFRAME_PTR fptr){
	//setByteAt function necessary?
	fptr->data[E_OPC_OFFSET] = fptr->propNum;
}

ECHOFRAME_PTR initFrame(ECHOCTRL_PTR cptr, size_t alocsize, uint16_t TID){
	ECHOFRAME_PTR fptr = allocateFrame(alocsize);
	putByte(fptr, E_HID1);
	putByte(fptr, E_HID2);
	if (TID == 0){
		TID = incTID(cptr);
	}
	putShort(fptr, TID);
	return fptr;
}

//void scratch(){
//	ttt  yourtest = ena;
//	EOJ eoj;
//	putBytes(NULL, 3, eoj.eoj);
//}

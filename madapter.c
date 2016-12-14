#include "elite.h"

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

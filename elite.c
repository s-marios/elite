#include "elite.h"
#include <stdio.h>
#include <malloc.h>
#include <string.h>

size_t test(void) {
	return 4;
}

ECHOFRAME_PTR allocateFrame(size_t alocsize) {
	if (alocsize <= 0 || alocsize > ECHOFRAME_MAXSIZE) {
		alocsize = ECHOFRAME_STDSIZE;
	}
	ECHOFRAME_PTR fptr = (ECHOFRAME_PTR) malloc(
			alocsize + sizeof(ECHOFRAME_STDSIZE));
	if (fptr) {
		fptr->allocated = alocsize;
		fptr->used = 0;
		fptr->propNum = 0;
		fptr->data = ((char *) fptr) + sizeof(ECHOFRAME);
	}
	return fptr;
}
uint16_t incTID(ECHOCTRL_PTR cptr) {
	uint16_t result = cptr->TID;
	cptr->TID++;
	if (cptr == 0) {
		cptr->TID++;
	}
	return result;
}

ECHOCTRL_PTR createEchonetControl() {
	ECHOCTRL_PTR ecptr = malloc(sizeof(ECHOCTRL));
	ecptr->TID = 1;
	return ecptr;
}

int checkSize(ECHOFRAME_PTR fptr, size_t increase) {
	return fptr->used + increase >= fptr->allocated;
}

int putBytes(ECHOFRAME_PTR fptr, uint8_t num, char * data) {
	if (checkSize(fptr, num)) {
		return -1;
	}
	//memmove(&ECHOFRAME_HEAD(fptr), data, num);
	memcpy(&fptr->data[fptr->used], data, num);
	fptr->used += num;
	return 0;
}

int putByte(ECHOFRAME_PTR fptr, char byte) {
	if (checkSize(fptr, 1))
		return -1;
	ECHOFRAME_HEAD(fptr) = byte;
	fptr->used++;
	return 0;
}

int putShort(ECHOFRAME_PTR fptr, uint16_t aShort) {
	if (checkSize(fptr, 2))
		return -1;
	ECHOFRAME_HEAD(fptr) = aShort >> 8;
	fptr->used++;
	ECHOFRAME_HEAD(fptr) = ((char) (0x00ff & aShort));
	fptr->used++;
	return 0;
}

int putEOJ(ECHOFRAME_PTR fptr, EOJ eoj) {
	return putBytes(fptr, 3, eoj);
}

int putEPC(ECHOFRAME_PTR fptr, uint8_t epc, uint8_t size, char * data) {
	putByte(fptr, epc);
	putByte(fptr, size);
	putBytes(fptr, size, data);
	fptr->propNum++;
	return 0;
}

int putESVnOPC(ECHOFRAME_PTR fptr, ESV esv) {
	putByte(fptr, esv);
	putByte(fptr, 0);
	return 0;
}

void finalizeFrame(ECHOFRAME_PTR fptr) {
	//setByteAt function necessary?
	fptr->data[OFF_OPC] = fptr->propNum;
}

ECHOFRAME_PTR initFrame(ECHOCTRL_PTR cptr, size_t alocsize, uint16_t TID) {
	ECHOFRAME_PTR fptr = allocateFrame(alocsize);
	putByte(fptr, E_HD1);
	putByte(fptr, E_HD2);
	if (TID == 0) {
		TID = incTID(cptr);
	}
	putShort(fptr, TID);
	return fptr;
}

uint16_t getShort(ECHOFRAME_PTR fptr, uint16_t offset) {
	if (offset < 0 || offset > fptr->used) {
		return 0xffff;
	} else {
		return (fptr->data[offset] << 8 | fptr->data[offset + 1]);
	}
}

void dumpFrame(ECHOFRAME_PTR fptr) {
	PRINTF("Frame info used, allocated, propNum: %d %d %d\n", fptr->used,
			fptr->allocated, fptr->propNum);
	PRINTF("Data: ");
	for (int i = 0; i < fptr->used; i++) {
		PRINTF("\\0x%02x", fptr->data[i]);
	}
	PRINTF("\n");
}

int isValidESV(uint8_t esv) {
	switch (esv) {
	case ESV_SETI:
	case ESV_SETC:
	case ESV_GET:
	case ESV_INFREQ:
	case ESV_SETGET:
	case ESV_SETRES:
	case ESV_GETRES:
	case ESV_INF:
	case ESV_INFC:
	case ESV_INFCRES:
	case ESV_SETGETRES:
	case ESV_SETI_SNA:
	case ESV_SETC_SNA:
	case ESV_GETC_SNA:
	case ESV_INF_SNA:
	case ESV_SETGET_SNA:
		return 1;
	}
	return 0;
}

PARSERESULT parseFrame(ECHOFRAME_PTR fptr) {
	//basic checks.
	if (fptr == NULL || fptr->data == NULL) {
		return PR_NULL;
	}
	if (fptr->used < ECHOFRAME_MINSIZE) {
		return PR_TOOSHORT;
	}
	//hard cut-off for big frames
	if (fptr->used > ECHOFRAME_MAXSIZE) {
		return PR_TOOLONG;
	}
	//check EHD1 & EHD2
	if (fptr->data[OFF_EHD1] != E_HD1 || fptr->data[OFF_EHD2] != E_HD2) {
		return PR_HD;
	}
	//ignore TID and SEOJ/DEOJ
	//TODO: check for valid-looking eoj? not sure it's a good idea.
	if (!isValidESV(fptr->data[OFF_ESV])) {
		return PR_INVESV;
	}

	//parsing a property
	//a relative index into the current location in the frame data
	uint8_t index = OFF_EPC;
	uint8_t opc = fptr->data[OFF_OPC];
	if (opc == 0) {
		return PR_OPCZERO;
	}

	PARSE_EPC epc;
	memset(&epc, 0, sizeof(PARSE_EPC));
	int skipResult = 1;
	char * curProp = &fptr->data[index];
	while (skipResult) {
		//parse data into EPC struct
		//which property is this? first second etc.
		epc.propIndex++;
		//setup the actual property code
		epc.epc = curProp[0];
		index++;
		//setup property data count
		epc.pdc = curProp[1];
		index++;
		//setup property data if it exists.
		if (epc.pdc != 0) {
			epc.edt = &curProp[2];
			index += epc.pdc;
		}

		PPRINTF("EPC: 0x%2x %d\n", epc.epc, epc.pdc);
		//all thing accounted for?
		if (index < fptr->used) {
			PPRINTF("readin next prop...\n");
			skipResult = 1;
			curProp = &fptr->data[index];
			continue;
		} else if (index == fptr->used) {
			//did we read all the properties?
				PPRINTF("opc propIndex: %d %d", opc, epc.propIndex);
			if (epc.propIndex == opc) {
				//everything matches
				return PR_OK;
			} else {
				PPRINTF("too long but correct number of props...\n");
				//we were about to read more properties.
				return PR_TOOLONG;
			}
		} else if (index > fptr->used) {
			PPRINTF("packet parsing too long...\n");
			return PR_TOOLONG;
		}
	}

	//unreachable
	return PR_OK;
}

int getNextEPC(ECHOFRAME_PTR fptr, PARSE_EPC_PTR epc) {
	//is this an uninitialized epc? get the first property
	if (epc->propIndex == 0) {
		//setup total property count
		epc->opc = fptr->data[OFF_OPC];

		epc->epc = fptr->data[OFF_EPC];
		epc->pdc = fptr->data[OFF_PDC];
		epc->edt = &fptr->data[OFF_EDT];
		epc->propIndex++;
		return 1;
	}
	if (epc->propIndex >= epc->opc) {
		//this was the last epc we had
		// > (or we might be lucky to catch an uninitialized one).
		//cannot do anything more with this.
		return 0;
	}
	//we have a next property, figure out the details here.
	//this is the base address of the next property.
	char * nextEPCptr = epc->edt + epc->pdc;
	//parse the data
	epc->epc = *nextEPCptr;
	epc->pdc = *(nextEPCptr +1);
	epc->edt = nextEPCptr +2;
	epc->propIndex++;
	return 1;
}

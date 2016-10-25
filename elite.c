#include <stdio.h>
#include <malloc.h>
#include <string.h>

#include "elite.h"
#include "macrolist.h"

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
	epc->pdc = *(nextEPCptr + 1);
	epc->edt = nextEPCptr + 2;
	epc->propIndex++;
	return 1;
}

OBJ_PTR createObject(char * eoj) {
	OBJ_PTR obj = malloc(sizeof(OBJ));
	if (obj == NULL) {
		return 0;
	}
	memcpy(obj->eoj, eoj, 3);
	return obj;
}

int readProperty(Property_PTR property, uint8_t size, char * buf) {
	if (property == NULL || property->read == NULL) {
		return -1;
	} else if (buf == NULL) {
		return -2;
	} else {
		return property->read(property, size, buf);
	}
}

int writeProperty(Property_PTR property, uint8_t size, char * buf) {
	if (property == NULL || property->write == NULL) {
		return -1;
	} else if (buf == NULL) {
		return -2;
	} else {
		return property->write(property, size, buf);
	}
}

int testRead(Property_PTR property, uint8_t size, char * buf) {
	memcpy(buf, "test", 4);
	return 4;
}

void freeProperty(Property_PTR property) {
	if (property == NULL) {
		return;
	}
	if (property->freeptr) {
		property->freeptr(property);
		return;
	} else {
		//Default destructor
		//Frees the optional pointer and then the property itself.
		if (property->opt != NULL) {
			free(property->opt);
			property->opt = NULL;
		}
		free(property);
	}
}

void initTestProperty(Property_PTR property) {
	if (property == NULL)
		return;
	memset(property, 0, sizeof(Property));
	property->propcode = 0x80;
	property->read = testRead;
}

typedef struct {
	uint8_t maxsize;
	uint8_t used;
	char * data;
} DATABUFFER, *DATABUFFER_PTR;

//read function for a "data property"
int readData(Property_PTR property, uint8_t size, char * buf) {
	DATABUFFER_PTR databuffer = (DATABUFFER_PTR) property->opt;
	if (databuffer->used > size) {
		return 0;
	} else {
		memcpy(buf, &databuffer->data, databuffer->used);
		return databuffer->used;
	}
}

//write function for a "data property"
int writeData(Property_PTR property, uint8_t size, char * buf) {
	DATABUFFER_PTR databuffer = (DATABUFFER_PTR) property->opt;
	if (size > databuffer->maxsize) {
		return 0;
	} else {
		memcpy(&databuffer->data, buf, size);
		databuffer->used = size;
		return size;
	}
}

int writeDataExact(Property_PTR property, uint8_t size, char * buf) {
	DATABUFFER_PTR databuffer = (DATABUFFER_PTR) property->opt;
	if (size != databuffer->maxsize) {
		return 0;
	} else {
		memcpy(&databuffer->data, buf, size);
		databuffer->used = size;
		return size;
	}
}

Property_PTR createProperty(uint8_t propcode, uint8_t mode) {
	Property_PTR property = malloc(sizeof(Property));
	if (property == NULL) {
		return NULL;
	}
	//we have the basic property, memzero everything
	//we also use the default destructor
	memset(property, 0, sizeof(Property));
	property->rwn_mode = mode;
	property->propcode = propcode;
	return property;
}

Property_PTR createDataProperty(uint8_t propcode, uint8_t rwn, uint8_t maxsize,
		uint8_t datasize, char * data) {
	Property_PTR property = createProperty(propcode, rwn);
	if (property == NULL) {
		return NULL;
	}

	//check if maxSize is less than data size, and BAIL
	if (datasize > maxsize) {
		freeProperty(property);
		return NULL;
	}

	//init the space for the databuffer and the data;
	property->opt = malloc(sizeof(DATABUFFER) + maxsize);
	if (property->opt == NULL) {
		//somehow we failed. bail out
		free(property);
		return NULL;
	}


	//copy the incoming data
	DATABUFFER_PTR databuffer = (DATABUFFER_PTR) property->opt;
	databuffer->maxsize = maxsize;
	databuffer->used = datasize;

	if (datasize > 0 && data != NULL) {
		//copy the data into the buffer
		memcpy(&databuffer->data, data, databuffer->used);
	} else {
		//just clear the data space
		memset(&databuffer->data, 0, databuffer->maxsize);
	}

	//setup the read write accessors
	if (rwn & E_WRITE) {
		if (datasize < maxsize) {
			property->write = writeData;
		} else if (datasize == maxsize) {
			property->write = writeDataExact;
		}
	}
	if (rwn & E_READ) {
		property->read = readData;
	}
	if (rwn & E_NOTIFY) {
		//TODO, what do we do about notifications?!
	}
	return property;
}

/**
 * Flips the corresponding bit in the property bit map.
 *
 * This is used in type-2 (binary) property map format.
 *
 * param code the property code
 * param pbitmap the property bitmap to be manipulated
 */
void flipPropertyBit(uint8_t code, char * pbitmap) {
	//high nibble specifies the bit to flip through shifting
	//low nibble is the byte in which to flip
	uint8_t highnibble, lownibble;
	//zero-th byte is property count
	lownibble = (code & 0x0F) + 1;
	highnibble = code >> 4;
	//also subtract 0x08 from highnibble
	highnibble -= 0x08;

	//flip the bit
	pbitmap[lownibble] |= 0x01 << highnibble;
}

int computeListMaps(OBJ_PTR obj, char * maps);
int computeBinaryMaps(OBJ_PTR obj, char * maps);

int computePropertyMaps(OBJ_PTR obj) {
	//in order: notify, set, get rawmaps
	char * rawmaps = malloc(3 * 17);
	if (rawmaps == NULL) {
		PRINTF("run out of memory.");
		return -1;
	}
	//first, count the properties to decide the format
	int count = 0;
	Property_PTR pHead = obj->properties;
	FOREACHPURE(pHead)
	{
		count++;
	}

	if (count < 16) {
		//TODO simpe bytes
		return computeListMaps(obj, rawmaps);
	} else {
		//TODO bitflips
		return computeBinaryMaps(obj, rawmaps);
	}
	return 0;
}

#define MAPOFF_N 0
#define MAPOFF_S 17
#define MAPOFF_G 34
int computeListMaps(OBJ_PTR obj, char * maps) {
	FOREACH(obj->properties, Property_PTR)
	{
		if (element->rwn_mode & E_NOTIFY) {
			//maps[MAPOFF_N]++; preincrement
			maps[MAPOFF_N + (++maps[MAPOFF_N])] = element->propcode;
		}
		if (element->rwn_mode & E_WRITE) {
			//maps[MAPOFF_S]++; preincrement
			maps[MAPOFF_S + (++maps[MAPOFF_S])] = element->propcode;
		}
		if (element->rwn_mode & E_READ) {
			//maps[MAPOFF_G]++; preincrement
			maps[MAPOFF_G + (++maps[MAPOFF_G])] = element->propcode;
		}
	}
	return 0;
}

int computeBinaryMaps(OBJ_PTR obj, char * maps) {
	FOREACH(obj->properties, Property_PTR)
	{
		if (element->rwn_mode & E_NOTIFY) {
			maps[MAPOFF_N]++;
			flipPropertyBit(element->propcode, &maps[MAPOFF_N]);
		}
		if (element->rwn_mode & E_WRITE) {
			maps[MAPOFF_S]++;
			flipPropertyBit(element->propcode, &maps[MAPOFF_S]);
		}
		if (element->rwn_mode & E_READ) {
			maps[MAPOFF_G]++;
			flipPropertyBit(element->propcode, &maps[MAPOFF_G]);
		}
	}
	return 0;
}


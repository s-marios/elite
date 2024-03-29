/**
 * \file
 *
 * This is the bulk of the ECHONET Lite implementation.
 */

//silence inet_aton warning
#ifdef __unix
#define _DEFAULT_SOURCE 1
#endif

#include <stdint.h>
#include <stdio.h>
//sshhh... no malloc.h now, only stdlib...
#include <stdlib.h>

#include <limits.h>
#include <string.h>

#include "elite_priv.h"

ECHOFRAME_PTR allocateFrame(size_t alocsize) {
    if (alocsize > ECHOFRAME_MAXSIZE) {
        alocsize = ECHOFRAME_STDSIZE;
    }

    ECHOFRAME_PTR fptr = (ECHOFRAME_PTR) malloc(alocsize + sizeof(ECHOFRAME));

    if (fptr) {
        fptr->allocated = alocsize;
        fptr->used = 0;
        fptr->propNum = 0;
        fptr->data = ((unsigned char *) fptr) + sizeof(ECHOFRAME);
    }

    return fptr;
}

ECHOFRAME_PTR
wrapDataIntoFrame(ECHOFRAME_PTR frame, unsigned char * data, size_t length) {
    if (frame == NULL) {
        return NULL;
    }

    frame->data = data;
    frame->used = length;
    frame->allocated = 0;
    frame->propNum = 0;
    return frame;
}

uint16_t incTID(ECHOCTRL_PTR cptr) {
    uint16_t result = cptr->TID;
    cptr->TID++;

    if (cptr->TID == 0) {
        cptr->TID++;
    }

    return result;
}

ECHOCTRL_PTR createEchonetControl() {
    ECHOCTRL_PTR ecptr = malloc(sizeof(ECHOCTRL));
    memset(ecptr, 0, sizeof(ECHOCTRL));
    ecptr->TID = 1;
    //settign up multicast address
    //the socket is not setup here!
#if defined __unix || defined ESP_PLATFORM

    if (inet_aton(ELITE_MADDR, &ecptr->maddr.sin_addr) == 0) {
        PRINTF("SHOULD NOT HAPPEN: failed to convert ip");
    }

    ecptr->maddr.sin_family = AF_INET;
    ecptr->maddr.sin_port = ntohs(ELITE_PORT);

#else
    ip_addr_t dummy;
    IP4_ADDR(&dummy, 224, 0, 23, 0);

    ecptr->maddr.sin_addr.s_addr = dummy.addr;
    ecptr->maddr.sin_len = 0;
#endif
    ecptr->maddr.sin_family = AF_INET;
    ecptr->maddr.sin_port = ntohs(ELITE_PORT);
    ecptr->msock = -1;
    return ecptr;
}

int checkSize(ECHOFRAME_PTR fptr, size_t increase) {
    return fptr->used + increase > fptr->allocated;
}

int putBytes(ECHOFRAME_PTR fptr, uint8_t num, unsigned char * data) {
    if (checkSize(fptr, num) || data == NULL) {
        return -1;
    }

    memcpy(&fptr->data[fptr->used], data, num);
    fptr->used += num;
    return 0;
}

int putByte(ECHOFRAME_PTR fptr, char byte) {
    if (checkSize(fptr, 1)) {
        return -1;
    }

    ECHOFRAME_HEAD(fptr) = byte;
    fptr->used++;
    return 0;
}

int putShort(ECHOFRAME_PTR fptr, uint16_t aShort) {
    if (checkSize(fptr, 2)) {
        return -1;
    }

    // TODO kind of a WTF, it'll bite us if we move to big endian machines
    ECHOFRAME_HEAD(fptr) = aShort >> 8;
    fptr->used++;
    ECHOFRAME_HEAD(fptr) = ((char) (0x00ff & aShort));
    fptr->used++;
    return 0;
}

int putEOJ(ECHOFRAME_PTR fptr, EOJ eoj) {
    return putBytes(fptr, 3, (unsigned char *) eoj);
}

int putEPC(
    ECHOFRAME_PTR fptr, uint8_t epc, uint8_t size, unsigned char * data) {
    putByte(fptr, epc);
    putByte(fptr, size);
    putBytes(fptr, size, data);
    fptr->propNum++;
    return 0;
}

int putProperty(ECHOFRAME_PTR fptr, Property_PTR property) {
    unsigned char * to = &fptr->data[fptr->used + 2];
    int result = readProperty(property, fptr->allocated - fptr->used, to);

    if (result > 0) {
        putByte(fptr, property->propcode);
        putByte(fptr, result);
        //increase according to the read result
        fptr->used += result;
        fptr->propNum++;
        return result;
    } else {
        return -1;
    }
}

int putESVnOPC(ECHOFRAME_PTR fptr, ESV esv) {
    putByte(fptr, esv);
    putByte(fptr, 0);
    return 0;
}

void finalizeFrame(ECHOFRAME_PTR fptr) {
    fptr->data[OFF_OPC] = fptr->propNum;
}

ECHOFRAME_PTR initFrame(size_t alocsize, uint16_t TID) {
    ECHOFRAME_PTR fptr = allocateFrame(alocsize);
    putByte(fptr, E_HD1);
    putByte(fptr, E_HD2);
    putShort(fptr, TID);
    return fptr;
}

void freeFrame(ECHOFRAME_PTR frame) {
    if (frame) {
        free(frame);
    }
}

/**
 * Currently all affirmative ESV are +10 except ESV_INFC
 */
ESV getAffirmativeESV(ESV esv) {
    if (esv == ESV_INFC) {
        return ESV_INFCRES;
    } else {
        return esv + 0x10;
    }
}

ECHOFRAME_PTR initFrameResponse(
    ECHOFRAME_PTR incoming, unsigned char * eoj, size_t alocsize) {
    if (incoming == NULL) {
        return NULL;
    }

    ECHOFRAME_PTR fptr = initFrame(alocsize, getTID(incoming));

    //source eoj is us
    if (eoj) {
        putEOJ(fptr, eoj);
    } else {
        putEOJ(fptr, getDEOJ(incoming));
    }

    //deoj is source eoj of incoming
    putEOJ(fptr, getSEOJ(incoming));

    ESV esvr = getAffirmativeESV(getESV(incoming));

    if (esvr == 0) {
        PPRINTF("init frame simple request: unrecognized ESV, dropping..\n");
        free(fptr);
        return NULL;
    }

    putESVnOPC(fptr, esvr);
    return fptr;
}

uint16_t getShort(ECHOFRAME_PTR fptr, uint16_t offset) {
    if (offset > fptr->used) {
        return 0xffff;
    } else {
        return (fptr->data[offset] << 8 | fptr->data[offset + 1]);
    }
}

void dumpFrame(ECHOFRAME_PTR fptr) {
    PRINTF(
        "Frame info used, allocated, propNum: %zu %zu %d\n",
        fptr->used,
        fptr->allocated,
        fptr->propNum);
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
    unsigned char * curProp = &fptr->data[index];

    for (;;) {
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
            PPRINTF("readin next prop...\r\n");
            curProp = &fptr->data[index];
            continue;
        } else if (index == fptr->used) {
            //did we read all the properties?
            PPRINTF("opc propIndex: %d %d\r\n", opc, epc.propIndex);

            if (epc.propIndex == opc) {
                //everything matches
                return PR_OK;
            } else {
                PPRINTF("too long but correct number of props...\r\n");
                //we were about to read more properties.
                return PR_TOOLONG;
            }
        } else {
            // index > fptr->used
            PPRINTF("packet parsing too long...\r\n");
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
    unsigned char * nextEPCptr = epc->edt + epc->pdc;
    //parse the data
    epc->epc = *nextEPCptr;
    epc->pdc = *(nextEPCptr + 1);
    epc->edt = nextEPCptr + 2;
    epc->propIndex++;
    return 1;
}

void freeObject(OBJ_PTR obj) {
    FREEPROPERTIES(obj->pHead);
    free(obj);
}

OBJ_PTR createObject(char * eoj) {
    OBJ_PTR obj = malloc(sizeof(OBJ));

    if (obj == NULL) {
        return 0;
    }

    memset(obj, 0, sizeof(OBJ));
    setEOJ(obj, eoj);
    return obj;
}

void addObject(ECHOCTRL_PTR ectrl, OBJ_PTR obj) {
    LAPPEND((void **) &ectrl->oHead, obj);
    obj->ectrl = ectrl;
}

void addProperty(OBJ_PTR obj, Property_PTR property) {
    Property_PTR exists = getProperty(obj, property->propcode);

    if (exists) {
        LREPLACE((void **) &obj->pHead, exists, property);
    } else {
        LAPPEND((void **) &obj->pHead, property);
    }

    property->pObj = obj;
}

/**
 * return bytes read, zero or less in failure
 */
int readProperty(Property_PTR property, uint8_t size, unsigned char * buf) {
    if (property == NULL || property->readf == NULL) {
        return -1;
    } else if (buf == NULL) {
        return -2;
    } else {
        return property->readf(property, size, buf);
    }
}

int requiresNotification(
    Property_PTR property, uint8_t size, const unsigned char * buff);

/**
 * return bytes written, zero or less on failure
 */
int writeProperty(
    Property_PTR property, uint8_t size, const unsigned char * buf) {
    if (property == NULL || property->writef == NULL) {
        return -1;
    } else if (buf == NULL) {
        return -2;
    } else {
        //Do we need notifications?
        int doNotify = 0;

        if (property->rwn_mode & E_NOTIFY) {
            if (property->rwn_mode & E_SUPPRESS_NOTIFY) {
                doNotify = requiresNotification(property, size, buf);
            } else {
                doNotify = 1;
            }
        }

        //write
        int result = property->writef(property, size, buf);

        //perform notification if necessary.
        if (result > 0 && doNotify) {
            makeNotification(property);
        }

        return result;
    }
}

int requiresNotification(
    Property_PTR property, uint8_t size, const unsigned char * incoming) {
    unsigned char readbuffer[size];

    //tricky: zero read bytes considered too
    if (readProperty(property, size, readbuffer) <= 0) {
        //error during read ... let's notify???
        return 1;
    }

    //compare the two buffers and return the result..
    return memcmp(readbuffer, incoming, size);
}

int testRead(Property_PTR property, uint8_t size, unsigned char * buf) {
    memcpy(buf, "test", 4);
    return 4;
}

Property_PTR freeProperty(Property_PTR property) {
    if (property == NULL) {
        return NULL;
    }

    Property_PTR next = property->next;

    if (property->freeptr) {
        property->freeptr(property);
    } else {
        //Default destructor
        //Frees the optional pointer and then the property itself.
        if (property->opt != NULL) {
            free(property->opt);
            property->opt = NULL;
        }

        free(property);
    }

    return next;
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

int compareProperties(void * prop, void * other) {
    if (prop == other) {
        return 0;
    }

    if (prop == NULL) {
        return INT_MIN;
    } else if (other == NULL) {
        return INT_MAX;
    }

    Property_PTR property = (Property_PTR) prop;
    Property_PTR o = (Property_PTR) other;
    return property->propcode - o->propcode;
}

int comparePropertyCode(void * prop, void * code) {
    if (prop == NULL) {
        return INT_MIN;
    }

    return ((Property_PTR) prop)->propcode - (size_t) code;
}

void initTestProperty(Property_PTR property) {
    if (property == NULL) {
        return;
    }

    memset(property, 0, sizeof(Property));
    property->propcode = 0x80;
    property->readf = testRead;
}

typedef struct {
    uint8_t maxsize;
    uint8_t used;
    char * data;
} DATABUFFER, *DATABUFFER_PTR;

//read function for a "data property"
int readData(Property_PTR property, uint8_t size, unsigned char * buf) {
    DATABUFFER_PTR databuffer = (DATABUFFER_PTR) property->opt;

    if (databuffer->used > size) {
        return 0;
    } else {
        memcpy(buf, &databuffer->data, databuffer->used);
        return databuffer->used;
    }
}

//write function for a "data property"
int writeData(Property_PTR property, uint8_t size, const unsigned char * buf) {
    DATABUFFER_PTR databuffer = (DATABUFFER_PTR) property->opt;

    if (size > databuffer->maxsize) {
        return 0;
    } else {
        memcpy(&databuffer->data, buf, size);
        databuffer->used = size;
        return size;
    }
}

int writeDataExact(
    Property_PTR property, uint8_t size, const unsigned char * buf) {
    DATABUFFER_PTR databuffer = (DATABUFFER_PTR) property->opt;

    if (size != databuffer->maxsize) {
        return 0;
    } else {
        memcpy(&databuffer->data, buf, size);
        databuffer->used = size;
        return size;
    }
}

Property_PTR createDataProperty(
    uint8_t propcode, uint8_t rwn, uint8_t maxsize, uint8_t datasize,
    char * data) {
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
    //readwrite are hooked by default.
    //access to them is available internally.

    if (datasize < maxsize) {
        property->writef = writeData;
    } else {
        // datasize == maxsize case
        property->writef = writeDataExact;
    }

    property->readf = readData;
    return property;
}

Property_PTR createDataProperty2(
    uint8_t propcode, uint8_t rwn, storage_spec_e spec, uint8_t maxsize,
    uint8_t datasize, char * data) {

    Property_PTR property =
        createDataProperty(propcode, rwn, maxsize, datasize, data);

    if (!property) {
        return NULL;
    }

    //rewire the write function depending on the storage spec
    //the default is write exact
    if (spec == STORAGE_UPTO) {
        property->writef = writeData;
    } else {
        property->writef = writeDataExact;
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

Property_PTR getProperty(OBJ_PTR obj, uint8_t code) {
    return (Property_PTR) LFIND(
        obj->pHead, (void *) (size_t) code, comparePropertyCode);
}

void copyBitmapsToProperties(OBJ_PTR obj, const unsigned char * rawmaps) {
    uint8_t codes[] = {0x9D, 0x9E, 0x9F};

    for (int i = 0; i < sizeof(codes); i++) {
        Property_PTR prop = getProperty(obj, codes[i]);

        //the size of the buffer. Account for list/binary format.
        uint8_t size = 1 + rawmaps[i * 17];

        if (size > 17) {
            size = 17;
        }

        writeProperty(prop, size, &rawmaps[i * 17]);
    }
}

#define MAPOFF_N 0
#define MAPOFF_S 17
#define MAPOFF_G 34

int computeMaps(OBJ_PTR obj, unsigned char * maps);

int computePropertyMaps(OBJ_PTR obj) {
    //in order: notify, set, get rawmaps
    unsigned char * rawmaps = malloc(3 * 17);

    if (rawmaps == NULL) {
        PRINTF("run out of memory.");
        return -1;
    }

    memset(rawmaps, 0, 3 * 17);

    computeMaps(obj, rawmaps);
    copyBitmapsToProperties(obj, rawmaps);

    free(rawmaps);
    return 0;
}

int computeMaps(OBJ_PTR obj, unsigned char * maps) {
    //first, count the properties to decide the format
    int count = 0;
    FOREACH(obj->pHead, Property_PTR) {
        if (element->rwn_mode & E_NOTIFY) {
            maps[MAPOFF_N]++;
        }

        if (element->rwn_mode & E_WRITE) {
            maps[MAPOFF_S]++;
        }

        if (element->rwn_mode & E_READ) {
            maps[MAPOFF_G]++;
        }

        count++;
    }

    int counts[3] = {0, 0, 0};
    FOREACH(obj->pHead, Property_PTR) {
        if (element->rwn_mode & E_NOTIFY) {
            if (maps[MAPOFF_N] < 16) {
                counts[0]++;
                maps[MAPOFF_N + counts[0]] = element->propcode;
            } else {
                flipPropertyBit(element->propcode, (char *) &maps[MAPOFF_N]);
            }
        }

        if (element->rwn_mode & E_WRITE) {
            if (maps[MAPOFF_S] < 16) {
                counts[1]++;
                maps[MAPOFF_S + counts[1]] = element->propcode;
            } else {
                flipPropertyBit(element->propcode, (char *) &maps[MAPOFF_S]);
            }
        }

        if (element->rwn_mode & E_READ) {
            if (maps[MAPOFF_G] < 16) {
                counts[2]++;
                maps[MAPOFF_G + counts[2]] = element->propcode;
            } else {
                flipPropertyBit(element->propcode, (char *) &maps[MAPOFF_G]);
            }
        }
    }
    return 0;
}

int computeListMaps(OBJ_PTR obj, char * maps) {
    FOREACH(obj->pHead, Property_PTR) {
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
    FOREACH(obj->pHead, Property_PTR) {
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

/**
 * creates a basic object with properties 0x80, 0x81, 0x82, 0x88,
 * 0x8A, 0x9D, 0x9E, 0x9F
 */
OBJ_PTR createBasicObject(char * eoj) {
    OBJ_PTR base = createObject(eoj);

    if (base == NULL) {
        return NULL;
    }

    addProperty(
        base, createDataProperty(0x80, E_READ | E_NOTIFY, 1, 1, "\x30"));
    addProperty(
        base,
        createDataProperty(0x81, E_READ | E_WRITE | E_NOTIFY, 1, 1, NULL));
    addProperty(base, createDataProperty(0x82, E_READ, 4, 4, "\0\0H\0"));
    addProperty(
        base, createDataProperty(0x88, E_READ | E_NOTIFY, 1, 1, "\x42"));
    addProperty(base, createDataProperty(0x8A, E_READ, 3, 3, "AAA"));
    addProperty(base, createDataProperty(0x9D, E_READ | E_NOTIFY, 17, 0, NULL));
    addProperty(base, createDataProperty(0x9E, E_READ | E_NOTIFY, 17, 0, NULL));
    addProperty(base, createDataProperty(0x9F, E_READ | E_NOTIFY, 17, 0, NULL));

    return base;
}

OBJ_PTR createNodeProfileObject() {
    //only one node profile.
    OBJ_PTR node = createObject(PROFILEEOJ);
    //superclass properties.
    addProperty(node, createDataProperty(0x8A, E_READ, 3, 3, "AAA"));
    addProperty(node, createDataProperty(0x9D, E_READ | E_NOTIFY, 17, 0, NULL));
    addProperty(node, createDataProperty(0x9E, E_READ | E_NOTIFY, 17, 0, NULL));
    addProperty(node, createDataProperty(0x9F, E_READ | E_NOTIFY, 17, 0, NULL));
    //class properties
    addProperty(
        node, createDataProperty(0x80, E_READ | E_NOTIFY, 1, 1, "\x30"));
    addProperty(
        node, createDataProperty(0x82, E_READ, 4, 4, "\x01\0x0C\0x01\x00"));
    addProperty(
        node,
        createDataProperty(
            0x83,
            E_READ,
            17,
            17,
            "\x31\x32\x33\x34\x35\x36\x37\x38\x39\x30\x31\x32\x33\x34\x35\x36"
            "\x37"));
    addProperty(node, createDataProperty(0xD3, E_READ | E_NOTIFY, 3, 0, NULL));
    addProperty(node, createDataProperty(0xD4, E_READ | E_NOTIFY, 2, 0, NULL));
    addProperty(
        node,
        createDataProperty(
            0xD5, E_READ | E_NOTIFY, E_INSTANCELISTSIZE, 0, NULL));
    addProperty(
        node,
        createDataProperty(
            0xD6, E_READ | E_NOTIFY, E_INSTANCELISTSIZE, 0, NULL));
    addProperty(node, createDataProperty(0xD7, E_READ | E_NOTIFY, 17, 0, NULL));
    return node;
}

OBJ_PTR getObject(OBJ_PTR oHead, char * eoj) {
    FOREACH(oHead, OBJ_PTR) {
        if (CMPEOJ(element->eoj, eoj) == 0) {
            return element;
        }
    }
    return NULL;
}

int computeNumberOfInstances(OBJ_PTR oHead) {
    int num = 0;
    FOREACHPURE(oHead) {
        num++;
    }
    //remove node profile instance
    return num - 1;
}

int isClassPresent(unsigned char * buf, uint8_t * eoj) {
    for (int i = 1; i < 17; i = i + 2) {
        if (memcmp(&buf[i], eoj, 2) == 0) {
            return i;
        }
    }

    return 0;
}

/**
 * computes the number of unique classes and creates a list of them
 * param result the class result (exactly 17 bytes of length)
 * return number of classes +1 node profile
 */
int computeClasses(OBJ_PTR oHead, unsigned char * result) {
    if (result == NULL || oHead == NULL) {
        return -1;
    }

    memset(result, 0, 17);
    FOREACH(oHead, OBJ_PTR) {
        if (!isClassPresent(result, element->eoj)) {
            if (result[0] >= 8) {
                PPRINTF("compute classes: out of class list space, ignoring "
                        "class...\r\n");
            } else {
                //check for node profile (exclude)
                if (CMPEOJ(element->eoj, PROFILEEOJ)) {
                    //copy that unique class in the result buffer.
                    memcpy(&result[1 + result[0] * 2], element->eoj, 2);
                    result[0]++;
                }
            }
        }
    }
    //+1 is for property d4
    return result[0] + 1;
}

int isInstancePresent(unsigned char * buf, uint8_t * eoj) {
    for (int i = 1; i < E_INSTANCELISTSIZE; i = i + 3) {
        if (memcmp(&buf[i], eoj, 3) == 0) {
            return i;
        }
    }

    return 0;
}

int computeInstances(OBJ_PTR oHead, unsigned char * result) {
    if (result == NULL || oHead == NULL) {
        return -1;
    }

    memset(result, 0, E_INSTANCELISTSIZE);
    int maxinstances = (E_INSTANCELISTSIZE - 1) / 3;
    FOREACH(oHead, OBJ_PTR) {
        if (!isInstancePresent(result, element->eoj)) {
            if (result[0] >= (maxinstances)) {
                PPRINTF(
                    "compute instances: out of space, ignoring instance\r\n");
            } else {
                //check for node profile (exclude)
                if (CMPEOJ(element->eoj, PROFILEEOJ)) {
                    memcpy(&result[1 + result[0] * 3], element->eoj, 3);
                    result[0]++;
                }
            }
        }
    }
    return result[0];
}

void computeNodeClassInstanceLists(OBJ_PTR oHead) {
    OBJ_PTR profile = getObject(oHead, PROFILEEOJ);

    if (profile == NULL) {
        PPRINTF("compute classes and instances: NULL profile object\r\n");
        return;
    }

    Property_PTR prop = NULL;
    int d3 = computeNumberOfInstances(oHead);
    unsigned char d3buf[] = {0, 0, d3};
    //d3 write
    prop = getProperty(profile, 0xD3);
    writeProperty(prop, 3, d3buf);

    //class list and number of classes calculation
    unsigned char * res = malloc(17);
    memset(res, 0, 17);
    int d4 = computeClasses(oHead, res);
    //d4 write
    prop = getProperty(profile, 0xD4);
    unsigned char d4buff[] = {0, d4};
    writeProperty(prop, 2, d4buff);
    //d7 write
    prop = getProperty(profile, 0xD7);
    int propsize = res[0] * 2 + 1; //2byte eoj + objcount (1 byte)
    writeProperty(prop, propsize, res);
    free(res);

    //instance lists, common for d5,d6
    res = malloc(E_INSTANCELISTSIZE);
    memset(res, 0, E_INSTANCELISTSIZE);
    computeInstances(oHead, res);
    //d5 write
    prop = getProperty(profile, 0xD5);
    propsize = res[0] * 3 + 1; //3byte eojs + objcount (1byte)
    writeProperty(prop, propsize, res);
    //d6 is same as d5, write
    prop = getProperty(profile, 0xD6);
    writeProperty(prop, propsize, res);
    //TODO copy to property
    free(res);
}

int processWrite(ECHOFRAME_PTR incoming, ECHOFRAME_PTR response, OBJ_PTR obj) {
    PARSE_EPC parsedepc;
    memset(&parsedepc, 0, sizeof(parsedepc));

    while (getNextEPC(incoming, &parsedepc)) {
        Property_PTR property = getProperty(obj, parsedepc.epc);

        if (property
            && writeProperty(property, parsedepc.pdc, parsedepc.edt) > 0) {
            //property exists and write succeeded
            if (response) { //SETI generates no response
                putEPC(response, property->propcode, 0, NULL);
            }
        } else {
            //no such property OR write fail. same handling
            //write fail - put the original contents in.
            if (response) { //SETI generates no response
                putEPC(response, parsedepc.epc, parsedepc.pdc, parsedepc.edt);
                //change ESV to failure
                setESV(response, getESV(incoming) - 0x10);
            }

            return -1;
        }
    }

    //all properties processed properly
    return 0;
}

int processRead(
    ECHOFRAME_PTR incoming, ECHOFRAME_PTR response, OBJ_PTR obj,
    E_WRITEMODE rwn) {
    if (response == NULL) {
        return -2;
    }

    int readres;
    PARSE_EPC parsedepc;
    memset(&parsedepc, 0, sizeof(parsedepc));

    while (getNextEPC(incoming, &parsedepc)) {
        Property_PTR property = getProperty(obj, parsedepc.epc);

        if (property && (property->rwn_mode & rwn)) {

            readres = putProperty(response, property);

            if (readres < 0) {
                // failed to read the property!
                putEPC(response, parsedepc.epc, 0, NULL);
                //change ESV to failure
                setESV(response, getESV(incoming) - 0x10);
                return -1;
            }

        } else {
            //no such property
            putEPC(response, parsedepc.epc, 0, NULL);
            //change ESV to failure
            setESV(response, getESV(incoming) - 0x10);
            return -1;
        }
    }

    //all properties processed properly
    return 0;
}

ECHOFRAME_PTR getPerObjectResponse(ECHOFRAME_PTR incoming, OBJ_PTR obj) {
    if (incoming == NULL) {
        return NULL;
    }

    ECHOFRAME_PTR res = NULL;
    uint8_t esv = getESV(incoming);

    switch (esv) {
        case ESV_SETC: //intentional fall through.
            res = initFrameResponse(incoming, obj->eoj, ECHOFRAME_MAXSIZE);

        // fall through
        case ESV_SETI:
            processWrite(incoming, res, obj);
            break;

        case ESV_GET:
            PRINTF("ESV_GET\r\n");
            res = initFrameResponse(incoming, obj->eoj, ECHOFRAME_MAXSIZE);
            processRead(incoming, res, obj, E_READ);
            break;

        case ESV_INFREQ:
            res = initFrameResponse(incoming, obj->eoj, ECHOFRAME_MAXSIZE);
            processRead(incoming, res, obj, E_NOTIFY | E_READ);
            break;

        case ESV_SETGET:
            PPRINTF("TODO: SetGet not supported yet\r\n");
            break;

        case ESV_INF:
            PPRINTF("Notification: ignore\r\n");
            break;

        default:
            PPRINTF("pIF: Unrecognized ESV (%d), doing nothing\r\n", esv);
    }

    return res;
}

OBJ_PTR matchObjects(OBJMATCH_PTR matcher) {
    OBJ_PTR head = NULL;

    if (matcher == NULL || matcher->eoj == NULL || matcher->oHead == NULL) {
        return NULL;
    }

    if (matcher->lastmatch != NULL) {
        head = matcher->lastmatch->next;
    } else {
        head = matcher->oHead;
    }

    uint8_t matchlength = 3;

    if (GETINSTANCE(matcher->eoj) == 0) {
        matchlength = 2;
    }

    //all setup, do the matching.
    //only instance matching supported right now
    FOREACH(head, OBJ_PTR) {
        if (memcmp(matcher->eoj, element->eoj, matchlength) == 0) {
            matcher->lastmatch = element;
            return matcher->lastmatch;
        }
    }
    matcher->lastmatch = NULL;
    return NULL;
}

void * applyOutgoingHandler(HANDLER_PTR handler, ECHOFRAME_PTR outgoing) {
    if (handler->func) {
        return handler->func(handler, outgoing);
    } else {
        PPRINTF("aOH: null processor handler\n");
        return NULL;
    }
}

void processIncomingFrame(
    ECHOFRAME_PTR incoming, OBJ_PTR oHead, HANDLER_PTR handler) {
    if (parseFrame(incoming) != PR_OK) {
        PPRINTF("bad echonet frame, dropping...\r\n");
        return;
    }

    //we have a good frame, get the object matches.
    OBJMATCH m;
    OBJMATCH_PTR matcher = &m;
    memset(matcher, 0, sizeof(OBJMATCH));
    matcher->eoj = getDEOJ(incoming);
    matcher->oHead = oHead;

    PRINTF("\r\n*** INCOMING ESV: 0x%2x ***\r\n", incoming->data[OFF_ESV]);

    while (matchObjects(matcher)) {
        ECHOFRAME_PTR response =
            getPerObjectResponse(incoming, matcher->lastmatch);

        if (response) {
            finalizeFrame(response);
            //SEND THEM over the wire
            PPRINTF("pIF: created a response.. ");

            if (handler) {
                PPRINTF("pIF: applying handler\r\n");
                applyOutgoingHandler(handler, response);
            } else {
                free(response);
            }
        }
    }
}

typedef struct {
    struct sockaddr_in * dst;
    ECHOCTRL_PTR ectrl;
} DEFAULTOUT, *DEFAULTOUT_PTR;

void * defaultOut(HANDLER_PTR handler, void * outgoing) {
    DEFAULTOUT_PTR dout = (DEFAULTOUT_PTR) handler->opt;
    ECHOFRAME_PTR outframe = (ECHOFRAME_PTR) outgoing;
    sendto(
        dout->ectrl->msock,
        outframe->data,
        outframe->used,
        0,
        (struct sockaddr *) dout->dst,
        sizeof(struct sockaddr_in));
    freeFrame(outframe);
    return NULL;
}

void sendNotification(ECHOCTRL_PTR context, ECHOFRAME_PTR outgoing) {
    if (context && outgoing) {
        finalizeFrame(outgoing);
        sendto(
            context->msock,
            outgoing->data,
            outgoing->used,
            0,
            (const struct sockaddr *) &context->maddr,
            sizeof(struct sockaddr));
    }
}

void makeNotification(Property_PTR property) {
    ECHOCTRL_PTR ectrl = property->pObj->ectrl;

    if (!ectrl || ectrl->msock < 0) {
        return;
    }

    //send a packet with the data from property
    ECHOFRAME_PTR nframe = initFrame(ECHOFRAME_MAXSIZE, incTID(ectrl));
    putEOJ(nframe, property->pObj->eoj);
    putEOJ(nframe, (unsigned char *) PROFILEEOJ);
    putESVnOPC(nframe, ESV_INF);

    int readres = putProperty(nframe, property);

    if (readres > 0) {
        finalizeFrame(nframe);

        sendto(
            ectrl->msock,
            nframe->data,
            nframe->used,
            0,
            (const struct sockaddr *) &ectrl->maddr,
            sizeof(struct sockaddr));
    }

    freeFrame(nframe);
}

void receiveLoop(ECHOCTRL_PTR ectrl) {
    for (;;) {
        PPRINTF(" R ");
        socklen_t addrlen = sizeof(struct sockaddr_in);
        int res = recvfrom(
            ectrl->msock,
            ectrl->buffer,
            ECHOCTRL_BUFSIZE,
            0,
            (struct sockaddr *) &ectrl->incoming,
            &addrlen);
        PPRINTF("rL: received UDP packet\r\n");

        if (res < -1) {
            PPRINTF("recvfrom error\r\n");
        }

        ECHOFRAME frame;
        wrapDataIntoFrame(&frame, ectrl->buffer, res);

        HANDLER handler;
        DEFAULTOUT defaultOutArgs;

        handler.func = defaultOut;
        handler.opt = &defaultOutArgs;

        defaultOutArgs.dst = &ectrl->incoming;
        defaultOutArgs.ectrl = ectrl;

        PPRINTF("i");
        processIncomingFrame(&frame, ectrl->oHead, &handler);
    }
}

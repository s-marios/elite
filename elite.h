#ifndef ELITE_H
#define ELITE_H

#include <stdint.h>
#include <stddef.h>

/*
 * Debug printfs
 */
#define ELITE_DEBUG 2

#define PRINTF
#define PPRINTF

#ifdef ELITE_DEBUG
#undef PRINTF
#define PRINTF printf
#if ELITE_DEBUG > 1
#undef PPRINTF
#define PPRINTF printf
#endif
#endif

//this is essentially forward declaration for the OBJ struct
typedef struct OBJ OBJ;
typedef struct OBJ * OBJ_PTR;

//this is essentially forward declaration for the property struct
typedef struct Property Property;
typedef struct Property * Property_PTR;

/**
 * This is the overall control structure for ECHONET Lite operations
 */
typedef struct {
	uint16_t TID;
	OBJ_PTR oHead;
} ECHOCTRL, *ECHOCTRL_PTR;

ECHOCTRL_PTR createEchonetControl();

/**
 * A structure representing an ECHONET Lite frame.
 */
typedef struct {
	size_t used;
	size_t allocated;
	uint8_t propNum;
	char * data;
} ECHOFRAME, *ECHOFRAME_PTR;

#define ECHOFRAME_HEAD(x) (((x)->data[(x)->used]))
#define ECHOFRAME_STDSIZE 128
#define ECHOFRAME_MAXSIZE 255
//frame should be at least 14 bytes or else discarded
//header (2) + TID (2)+ SDEOJ (6)+ ESVOPC (2) + EPCPDC (2)
#define ECHOFRAME_MINSIZE 14

typedef enum {
	ESV_SETI = 0x60,
	ESV_SETC = 0x61,
	ESV_GET = 0x62,
	ESV_INFREQ = 0x63,
	ESV_SETGET = 0x6E,
	ESV_SETRES = 0x71,
	ESV_GETRES = 0x72,
	ESV_INF = 0x73,
	ESV_INFC = 0x74,
	ESV_INFCRES = 0x7A,
	ESV_SETGETRES = 0x7E,
	ESV_SETI_SNA = 0x50,
	ESV_SETC_SNA = 0x51,
	ESV_GETC_SNA = 0x52,
	ESV_INF_SNA = 0x53,
	ESV_SETGET_SNA = 0x5E
} ESV;
#define ESV_REQUIRESANSWER (ESV_SETC | ESV_GET | ESV_INFREQ | ESV_SETGET)

#define E_HD1 0x10
#define E_HD2 0x81

//offset indexes
//#define E_ESV_OFFSET 10
//#define E_OPC_OFFSET 11

typedef char EOJ[3];

#define SETSUPERCLASS(x, scl) ((x[0]) = (scl))
#define SETCLASS(x, cl) ((x[1]) = (cl))
#define SETINSTANCE(x, inst) ((x[2]) = inst)
#define SETEOJ(x, scl, cl, inst) do { SETSUPERCLASS(x, scl); SETCLASS(x, cl); SETINSTANCE(x, inst); } while (0)
#define GETSUPERCLASS(x) ((x)[0])
#define GETCLASS(x) ((x)[1])
#define GETINSTANCE(x) ((x)[2])
#define CMPEOJ(x, y) memcmp(x, y, 3)

ECHOFRAME_PTR allocateFrame(size_t alocsize);
ECHOFRAME_PTR initFrameDepr(ECHOCTRL_PTR cptr, size_t alocsize, uint16_t TID);
ECHOFRAME_PTR initFrame(size_t alocsize, uint16_t TID);
/**
 * creates a simple response frame.
 *
 * reverses seoj/deoj of incoming packet
 */
ECHOFRAME_PTR initFrameResponse(ECHOFRAME_PTR incoming, char * eoj,
		size_t alocsize);

int putByte(ECHOFRAME_PTR fptr, char byte);
int putBytes(ECHOFRAME_PTR fptr, uint8_t num, char * data);
int putShort(ECHOFRAME_PTR fptr, uint16_t aShort);

uint16_t getShort(ECHOFRAME_PTR fptr, uint16_t offset);
int putEOJ(ECHOFRAME_PTR fptr, EOJ eoj);

int putEPC(ECHOFRAME_PTR fptr, uint8_t epc, uint8_t size, char * data);
int putESVnOPC(ECHOFRAME_PTR fptr, ESV esv);
#define setESV(fptr, esv) (fptr->data[OFF_ESV] = esv)

void finalizeFrame(ECHOFRAME_PTR fptr);
void dumpFrame(ECHOFRAME_PTR fptr);

typedef enum {
	PR_OK = 1, PR_TOOLONG, PR_TOOSHORT, PR_HD, PR_INVESV, PR_OPCZERO, PR_NULL
} PARSERESULT;

typedef enum {
	OFF_EHD1 = 0,
	OFF_EHD2 = 1,
	OFF_TID = 2,
	OFF_SEOJ = 4,
	OFF_DEOJ = 7,
	OFF_ESV = 10,
	OFF_OPC = 11,
	OFF_EPC = 12,
	OFF_PDC = 13,
	OFF_EDT = 14
} OFFSETS;

PARSERESULT parseFrame(ECHOFRAME_PTR fptr);

//this is a parsed EPC, but the name EPC conflicts
typedef struct {
	uint8_t opc; /**< number of properties */
	uint8_t propIndex; /**< current property index? */
	uint8_t epc; /**< property code */
	uint8_t pdc; /**< data count */
	char * edt; /**< data */
} PARSE_EPC, *PARSE_EPC_PTR;

/**
 * return 1 if found, 0 if no next epc found
 */
int getNextEPC(ECHOFRAME_PTR fptr, PARSE_EPC_PTR epc);
#define getTID(x) getShort(x, OFF_TID)
#define getSEOJ(x) &x->data[OFF_SEOJ]
#define getDEOJ(x) &x->data[OFF_DEOJ]
#define getESV(x) x->data[OFF_ESV]
#define getOPC(x) x->data[OFF_OPC]

/**
 * function type for read write operations
 * return the number of bytes read or written.
 */
typedef int (*READWRITE)(Property_PTR property, uint8_t size, char * buf);
typedef Property_PTR (*FREEFUNC)(Property_PTR property);

#define FREE(aptr) aptr->freeptr(aptr)

typedef enum {
	E_READ = 1, E_WRITE = 2, E_NOTIFY = 4,
} E_WRITEMODE;

struct Property {
	void * next;
	void * opt;
	FREEFUNC freeptr;
	READWRITE read;
	READWRITE write;
	uint8_t propcode;
	uint8_t rwn_mode;
};

#define classgroup eoj[0]
#define class eoj[1]
#define instance eoj[2]

struct OBJ {
	void * next;
	int i; /*!< this was used for tests */
	Property_PTR pHead;
	uint8_t eoj[3];
};

void freeObject(OBJ_PTR obj);
OBJ_PTR createObject(char * eoj);
#define setClassGroup(obj_ptr, val) (obj_ptr)->classgroup = val
#define setClass(obj_ptr, val) (obj_ptr)->class = val
#define setInstance(obj_ptr, val) (obj_ptr)->instance = val
#define setEOJ(obj_ptr, eoj) 	do { \
	if (obj_ptr && eoj) { \
		memcpy(obj_ptr->eoj, eoj, 3); \
	} \
} while (0)

#define addProperty(obj_ptr, property) LAPPEND( &obj_ptr->pHead, property)

int readProperty(Property_PTR property, uint8_t size, char * buf);
int writeProperty(Property_PTR property, uint8_t size, char * buf);
Property_PTR getProperty(OBJ_PTR obj, uint8_t code);

/**
 * Frees a property.
 *
 * It will try to invoke the custom free function if present, else it will
 * perform default clean up actions and return the next property, if present.
 * Use FREEPROPERTIES to delete a list of properties in a single setp. Does
 * NOT work with FOREACH.
 *
 * @param proeprty the property to be freed
 * @return the address of the next property or NULL if not present
 */
Property_PTR freeProperty(Property_PTR property);
#define FREEPROPERTIES(head) while(head){head = freeProperty(head);}
Property_PTR createProperty(uint8_t propcode, uint8_t mode);

/**
 * Creates a pure data property that stores data.
 */
Property_PTR createDataProperty(uint8_t propcode, uint8_t rwn, uint8_t maxsize,
		uint8_t dataSize, char * data);

int compareProperties(void * prop, void * other);
int comparePropertyCode(void * prop, void * code);

//for some testing
int testRead(Property_PTR property, uint8_t size, char * buf);
void initTestProperty(Property_PTR property);

void flipPropertyBit(uint8_t code, char * pbitmap);
/**
 * This function computes the property maps for this object and
 * adds the corresponding properties (9d,9e,9f) to it.
 */
int computePropertyMaps(OBJ_PTR obj);

/**
 * creates a basic object with the bare minimum necessary properties.
 */
OBJ_PTR createBasicObject(char * eoj);

#define PROFILEEOJ "\x0E\xF0\x01"
/**
 * creates a basic node profile object with the bare minimum necessary
 * properties.
 */
OBJ_PTR createNodeProfileObject();

void computeNodeClassInstanceLists(OBJ_PTR obj);

OBJ_PTR getObject(OBJ_PTR oHead, char * eoj);
//get the object by passing in the elite control structure
#define GETOBJECT(ctrl, eoj) getObject(ctrl->oHead, eoj)

//TODO move to a sane place
/**
 * 63 /3 = 21 instances + instance number
 */

#define E_INSTANCELISTSIZE 64

typedef struct {
	OBJ_PTR lastmatch;
	OBJ_PTR oHead;
	char * eoj;
} OBJMATCH, *OBJMATCH_PTR;
OBJ_PTR matchObjects(OBJMATCH_PTR matcher);

#endif

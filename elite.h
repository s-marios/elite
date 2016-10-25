#ifndef ELITE_H
#define ELITE_H

#include <stdint.h>
#include <stddef.h>

/*
 * Debug printfs
 */
#define ELITE_DEBUG 1

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
/**
 * This is the overall control structure for ECHONET Lite operations
 */
typedef struct {
	uint16_t TID;
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
#define ECHOFRAME_MAXSIZE 256
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
ECHOFRAME_PTR initFrame(ECHOCTRL_PTR cptr, size_t alocsize, uint16_t EID);

int putByte(ECHOFRAME_PTR fptr, char byte);
int putBytes(ECHOFRAME_PTR fptr, uint8_t num, char * data);
int putShort(ECHOFRAME_PTR fptr, uint16_t aShort);

uint16_t getShort(ECHOFRAME_PTR fptr, uint16_t offset);
int putEOJ(ECHOFRAME_PTR fptr, EOJ eoj);
//quick dirty macro for putting eojs in frames. the eoj thing is getting ridiculous.
//#define putEOJ(fptr, eojptr) putBytes(fptr, 3, eojptr)

int putEPC(ECHOFRAME_PTR fptr, uint8_t epc, uint8_t size, char * data);
int putESVnOPC(ECHOFRAME_PTR fptr, ESV esv);
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
	uint8_t opc;
	uint8_t propIndex;
	uint8_t epc;
	uint8_t pdc;
	char * edt;
} PARSE_EPC, *PARSE_EPC_PTR;

int getNextEPC(ECHOFRAME_PTR fptr, PARSE_EPC_PTR epc);
#define getTID(x) getShort(x, OFF_TID)
#define getSEOJ(x) &x->data[OFF_SEOJ]
#define getDEOJ(x) &x->data[OFF_DEOJ]
#define getESV(x) x->data[OFF_ESV]
#define getOPC(x) x->data[OFF_OPC]

//this is essentially forward declaration for the property struct
typedef struct Property Property;
typedef struct Property * Property_PTR;
typedef int (*READWRITE)(Property_PTR property, uint8_t size, char * buf);
typedef void (*FREE)(Property_PTR property);

typedef enum {
	E_READ = 1, E_WRITE = 2, E_NOTIFY = 4,
} E_WRITEMODE;


struct Property {
	void * next;
	void * opt;
	FREE freeptr;
	READWRITE read;
	READWRITE write;
	uint8_t propcode;
	uint8_t rwn_mode;
};

#define classgroup eoj[0]
#define class eoj[1]
#define instance eoj[2]

typedef struct {
	void * next;
	int i; /*!< this was used for tests */
	Property_PTR properties;
	uint8_t eoj[3];
} OBJ, *OBJ_PTR;

OBJ_PTR createObject(char * eoj);
#define setClassGroup(obj_ptr, val) (obj_ptr)->classgroup = val
#define setClass(obj_ptr, val) (obj_ptr)->class = val
#define setInstance(obj_ptr, val) (obj_ptr)->instance = val

int readProperty(Property_PTR property, uint8_t size, char * buf);
int writeProperty(Property_PTR property, uint8_t size, char * buf);

void freeProperty(Property_PTR property);
Property_PTR createProperty(uint8_t propcode, uint8_t mode);

Property_PTR createDataProperty(uint8_t propcode, uint8_t rwn, uint8_t maxsize,
		uint8_t dataSize, char * data);



//for some testing
int testRead(Property_PTR property, uint8_t size, char * buf);
void initTestProperty(Property_PTR property);

void flipPropertyBit(uint8_t code, char * pbitmap);
/**
 * This function computes the property maps for this object and
 * adds the corresponding properties (9d,9e,9f) to it.
 */
int computePropertyMaps(OBJ_PTR obj);

#endif

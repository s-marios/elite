#ifndef ELITE_H
#define ELITE_H

#include <stdint.h>
#include <stddef.h>

/*
 * Debug printfs
 */
//#define ELITE_DEBUG 2

#ifdef ELITE_DEBUG
#define PRINTF printf
#if ELITE_DEBUG > 1
	#define PPRINTF printf
#endif
#else
#define PRINTF
#define PPRINTF
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
	PR_OK = 1,
	PR_TOOLONG,
	PR_TOOSHORT,
	PR_HD,
	PR_INVESV,
	PR_OPCZERO,
	PR_NULL
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
} PROP, * PROP_PTR;

int getNextEPC(ECHOFRAME_PTR fptr, PROP_PTR epc);
#define getTID(x) getShort(x, OFF_TID)
#define getSEOJ(x) &x->data[OFF_SEOJ]
#define getDEOJ(x) &x->data[OFF_DEOJ]
#define getESV(x) x->data[OFF_ESV]
#define getOPC(x) x->data[OFF_OPC]

#endif

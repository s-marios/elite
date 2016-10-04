#ifndef ELITE_H
#define ELITE_H

#include <stdint.h>
#include <stddef.h>

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

#define E_HID1 0x10
#define E_HID2 0x81

//offset indexes
#define E_ESV_OFFSET 10
#define E_OPC_OFFSET 11

typedef char EOJ[3];

#define SETSUPERCLASS(x, scl) ((x[0]) = (scl))
#define SETCLASS(x, cl) ((x[1]) = (cl))
#define SETINSTANCE(x, inst) ((x[2]) = inst)
#define SETEOJ(x, scl, cl, inst) do { SETSUPERCLASS(x, scl); SETCLASS(x, cl); SETINSTANCE(x, inst); } while (0)
#define GETSUPERCLASS(x) ((x)[0])
#define GETCLASS(x) ((x)[1])
#define GETINSTANCE(x) ((x)[2])

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

#endif
#ifndef ELITE_H
#define ELITE_H

#include <stdint.h>
#include <stddef.h>

size_t test(void);
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

ECHOFRAME_PTR allocateFrame(size_t alocsize);
ECHOFRAME_PTR initFrame(ECHOCTRL_PTR cptr, size_t alocsize, uint16_t EID);

int putByte(ECHOFRAME_PTR fptr, char byte);
int putBytes(ECHOFRAME_PTR fptr, uint8_t num, char * data);
int putShort(ECHOFRAME_PTR fptr, uint16_t aShort);
//quick dirty macro for putting eojs in frames. the eoj thing is getting ridiculous.
#define putEOJ(fptr, eojptr) putBytes(fptr, 3, eojptr->eoj)
int putEPC(ECHOFRAME_PTR fptr, uint8_t epc, uint8_t size, char * data);
int putESVnOPC(ECHOFRAME_PTR fptr, uint8_t esv);
void finalizeFrame(ECHOFRAME_PTR fptr);

#define E_HID1 0x10
#define E_HID2 0x81

//offset indexes
#define E_ESV_OFFSET 10
#define E_OPC_OFFSET 11

//esv defs - maybe use an enum?
#define E_ESV_SETI 0x60
#define E_ESV_SETC 0x61
#define E_ESV_GET 0x62
#define E_ESV_INFREQ 0x63
#define E_ESV_SETGET 0x6E
#define E_ESV_SETRES 0x71
#define E_ESV_GETRES 0x72
#define E_ESV_INF 0x73
#define E_ESV_INFC 0x74
#define E_ESV_INFCRES 0x7A
#define E_ESV_SETGETRES 0x7E
#define E_ESV_SETI_SNA 0x50
#define E_ESV_SETC_SNA 0x51
#define E_ESV_GET_SNA 0x52
#define E_ESV_INF_SNA 0x53
#define E_ESV_SETGET_SNA 0x5E

typedef struct {
	char eoj[3];
} EOJ, *EOJ_PTR;

#define SETSUPERCLASS(x, scl) ((x->eoj[0]) = (scl))
#define SETCLASS(x, cl) ((x->eoj[1]) = (cl))
#define SETINSTANCE(x, inst) ((x->eoj[2]) = inst)
#define GETSUPERCLASS(x) ((x)->eoj[0])
#define GETCLASS(x) ((x)->eoj[1])
#define GETINSTANCE(x) ((x)->eoj[2])


//typedef enum {ena = 1, dyo=2, ekato=100} ttt;
#endif

/**
 * \file
 *
 * Main header file for ECHONET Lite functionality. You should not have to
 * include any file other than this to use ELITE.
 */

/**
 *
 * \mainpage ELITE landing page
 *
 * This is the landing page of the ELITE documentation. Read the following
 * section for a \ref how_to.
 *
 * \section how_to Simple How-To
 *
 * This is a brief introduction on how to use this library. Depending on your platform
 * the details will be different, but here are the common steps of any ELITE application:
 *
 * - Setup an ELITE context (i.e. an \ref ECHOCTRL block).
 * - Setup your multicast socket.
 * - Setup ECHONET Lite objects and properties.
 *
 * It is necessary to inlcude the file elite.h in your program.
 *
 * ======
 *
 * \subsection setup_context Setup an ELITE Context
 *
 * An ELITE context is used to interact with any of the functionality used
 * by ELITE. This context is an \ref ECHOCTRL struct. Memory allocation and
 * partial initialization for this struct is achieved using the
 * \ref createEchonetControl function.
 *
 * \subsection setup_msocket Setup a multicast socket.
 *
 * The socket used by ELITE is a UDP socket that has joined the multicast
 * network for ECHONET Lite devices. Linux and lwip provide a socket API.
 * However, there are slight differences in data structures and ioctl.
 * On linux, You'll need to call socket(), bind() and setsockopt(), the
 * last one with IP_ADD_MEMBERSHIP to join the multicast network.
 *
 * Enabling multicast reception may need further initialization steps
 * deptending on your microcontroller. Consult the documentation for your
 * platform.
 *
 * After initializing the socket, assign it to the \ref ECHOCTRL_PTR::msock
 * member of the ELITE context.
 *
 * \subsection setup_objects Setup ECHONET Lite Objects
 *
 * For each ECHONET Lite object:
 *
 * - Create an object using the call \ref createBasicObject()
 * - Add the desired properties using the function \ref addProperty()
 * - After you've finiished adding properties, call \ref computePropertyMaps()
 * - After you've computed the property maps, register the object with the
 *   ECHONET Lite context using \ref addObject().
 *
 * The same rules apply for the special profile object, that has to be set up
 * manually. To set up the profile object:
 *
 * - Create the profile object using \ref createNodeProfileObject()
 * - Add any extra properties you want
 * - Call \ref computePropertyMaps()
 * - Register it with the ECHONET Lite context using \ref addObject()
 *
 *
 * Finally, after all objects have been created and registered, you *have to*
 * compute the class and instance lists (properties d3, d4, d5, d6, d7) using
 * computeNodeClassInstanceLists()
 *
 *
 */
#ifndef ELITE_H
#define ELITE_H

#include <stdint.h>
#include <stddef.h>

#include "logging.h"
#include "networking.h"


//this is essentially forward declaration for the OBJ struct
typedef struct OBJ OBJ;
typedef struct OBJ *OBJ_PTR;

//this is essentially forward declaration for the property struct
typedef struct Property Property;
typedef struct Property *Property_PTR;

//this is essentially forward declaration for the handler struct
typedef struct HANDLER HANDLER;
typedef struct HANDLER *HANDLER_PTR;

/**
 * Definition of the function that processes outgoing frames. This handler
 * is responsible for freeing the outgoing packet. Optional arguments to this
 * can be found in handler->opt. A handler that send packets on the network
 * is the default setup, but it can be substituted with something different
 * for testing purposes.
 */
typedef void* (*PROCESSORFUNC)(HANDLER_PTR handler, void *outgoing);

struct HANDLER {
	PROCESSORFUNC func; /**< The processing function pointer that will be used */
	void *opt; /**< Optional arguments used by func */
};

/**
 * This is the overall control structure for ECHONET Lite operations. Holds
 * information like the multicast socket used for communications, a list of
 * the registered ECHONET Lite objest, a transaction ID counter among other
 * things.
 */
typedef struct {
	int msock; /**< Multicast socket used for network communications */
	struct sockaddr_in maddr; /**< ECHONET Lite multicast address */
	struct sockaddr_in incoming; /**< Holds the address of incoming packets */

	uint16_t TID; /**< Transaction ID */
	OBJ_PTR oHead; /**< Head of the ECHONET Lite Objects list */
	struct HANDLER sendHandler; /**< Handler for outgoing packets */

#define ECHOCTRL_BUFSIZE 256
	/**
	 * Buffer for incoming packets. Packets received using recv
	 * will be stored here.
	 */
	unsigned char buffer[ECHOCTRL_BUFSIZE];

} ECHOCTRL, *ECHOCTRL_PTR;

/**
 * Allocate an echonet control context and set default values to its fields
 * (warning: it does *not* initialize the multicast socket).
 */
ECHOCTRL_PTR createEchonetControl();

/**
 * Increases and returns the next transaction id.
 * @param cptr the ECHONET Lite control context
 * @return the new transaction id, ready to be used in a frame.
 */
uint16_t incTID(ECHOCTRL_PTR cptr);

/**
 * A structure representing an ECHONET Lite frame.
 */
typedef struct {
	size_t used; /**< actual number of bytes currently used */
	size_t allocated; /**< allocated data bytes */
	uint8_t propNum; /**< associated property number */
	unsigned char *data; /**< raw property data */
} ECHOFRAME, *ECHOFRAME_PTR;

/** point to the head of the data of an echonet frame */
#define ECHOFRAME_HEAD(x) (((x)->data[(x)->used]))
/** standard echoframe size */
#define ECHOFRAME_STDSIZE 128
/** maximum echoframe size */
#define ECHOFRAME_MAXSIZE 255
/**
 *	minimum possible echonet frame size.
 *
 *frame should be at least 14 bytes or else discarded
 *header (2) + TID (2)+ SDEOJ (6)+ ESVOPC (2) + EPCPDC (2)
 */
#define ECHOFRAME_MINSIZE 14

/**
 * enum holding the various ESV services of ECHONET Lite.
 */
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

typedef unsigned char EOJ[3];

#define SETSUPERCLASS(x, scl) ((x[0]) = (scl))
#define SETCLASS(x, cl) ((x[1]) = (cl))
#define SETINSTANCE(x, inst) ((x[2]) = inst)
#define SETEOJ(x, scl, cl, inst) do { SETSUPERCLASS(x, scl); SETCLASS(x, cl); SETINSTANCE(x, inst); } while (0)
#define GETSUPERCLASS(x) ((x)[0])
#define GETCLASS(x) ((x)[1])
#define GETINSTANCE(x) ((x)[2])
#define CMPEOJ(x, y) memcmp(x, y, 3)

/**
 * Allocates an echonet frame
 * @param alocsize the data size of the frame. Up to ECHOFRAME_MAXSIZE.
 * @return pointer to the echoframe, NULL on failure.
 */
ECHOFRAME_PTR allocateFrame(size_t alocsize);
/**
 * Use an echoframe pointer to track data as a frame. Allocated bytes is zero.
 * @param frame the frame that will be used to track the data
 * @param data the data to be tracked
 * @param length data length
 * @return the frame supplied, NULL on failure
 */
ECHOFRAME_PTR wrapDataIntoFrame(ECHOFRAME_PTR frame, unsigned char *data,
		size_t length);

/**
 * Init an echonet frame. Sets up the first two bytes of the header and the TID.
 *
 * @param alocsize the size used for the data section
 * @param TID the transaction id to be used
 * @return a pointer to the allocated echonet frame;
 */
ECHOFRAME_PTR initFrame(size_t alocsize, uint16_t TID);

/**
 * Free the memory occupied by an echonet frame.
 *
 * @param frame the frame to free.
 */
void freeFrame(ECHOFRAME_PTR frame);
/**
 * Creates a simple response frame by reversing seoj/deoj of incoming frame.
 *
 * @param incoming the incoming request frame.
 * @param eoj the eoj of the answering object, if NULL it is auto-filled.
 * @param alocsize data size
 * @return the new response frame, NULL on failure.
 */
ECHOFRAME_PTR initFrameResponse(ECHOFRAME_PTR incoming, unsigned char *eoj,
		size_t alocsize);

/**
 * Put one byte as data in an echonet frame.
 * @return 0 on success, -1 on failure.
 */
int putByte(ECHOFRAME_PTR fptr, char byte);
/**
 * Put many bytes in an echonet frame.
 *
 * @param fptr the echonet frame pointer to put the data into
 * @param num the number of bytes in data
 * @param data the actual data
 * @return zero on success, -1 on failure
 */
int putBytes(ECHOFRAME_PTR fptr, uint8_t num, unsigned char *data);

/** Put a short (two byte integer) in an ECHONET Lite frame.
 *
 * @param fptr the echonet frame pointer to put the data into
 * @param aShort the short to put
 * @return zero on success, -1 in failure.
 */
int putShort(ECHOFRAME_PTR fptr, uint16_t aShort);

/**
 * Copies the contents of a property into the buffer.
 * If the internal readProperty fails, buffer remains unchanged.
 *
 * @param fptr the echonet frame pointer to put the data into
 * @param property the pointer to a property
 * @return number of bytes put into the buffer, -1 otherwise
 */
int putProperty(ECHOFRAME_PTR fptr, Property_PTR property);

/** Read the short located at the given data offset in an echonet frame.
 * @param fptr the echonet frame pointer to read from
 * @param offset the offset from which to read
 *
 * @return the short read, or 0xffff in case of failure
 * \todo change to uint32_t so we can differentiate in case of failure
 *
 */
uint16_t getShort(ECHOFRAME_PTR fptr, uint16_t offset);

/**
 * Put an eoj into a frame. Call this right after an initFrame twice to setup
 * source/destination eoj.
 * @param fptr the echonet frame pointer to put the eoj into
 * @param eoj the eoj to put in
 * @return zero on success, -1 on failure.
 */
int putEOJ(ECHOFRAME_PTR fptr, EOJ eoj);

/**
 * Put an EPC "type-length-value" in a frame.
 * @param fptr the echonet frame pointer to put data into
 * @param epc the property code
 * @param size the size of data
 * @param data the actual data
 * @return zero (see todo).
 * \todo setup error checking.
 */
int putEPC(ECHOFRAME_PTR fptr, uint8_t epc, uint8_t size, unsigned char *data);

/**
 * Put the esv and take the space for the operand count at the same time.
 * @param fptr the echonet frame
 * @param esv the esv to put in
 * @return zero (see todo).
 * \todo setup error checking.
 */
int putESVnOPC(ECHOFRAME_PTR fptr, ESV esv);

/**
 * set the ESV of a frame. Rarely used, when creating a frame use putESVnOPC() @see putESVnOPC.
 */
#define setESV(fptr, esv) (fptr->data[OFF_ESV] = esv)

/**
 * Finalizes an echonet frame (i.e. fills in the OPC count). Call this after all
 * the contents of a frame have been filled (i.e. when you have finished
 * creating your frame).
 * @param fptr the echonet frame to finalize.
 */
void finalizeFrame(ECHOFRAME_PTR fptr);

/**
 * Prints a frame using #PRINTF macro. @see PRINTF
 */
void dumpFrame(ECHOFRAME_PTR fptr);

/**
 * parse result of an echonet frame
 */
typedef enum {
	PR_OK = 1, PR_TOOLONG, PR_TOOSHORT, PR_HD, PR_INVESV, PR_OPCZERO, PR_NULL
} PARSERESULT;

/**
 * offsets into various indexes in a frame
 */
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

/**
 * Parses an ECHONET Lite frame.
 * @param fptr the frame to parse.
 * @result the result of parsing (on success it is #PR_OK).
 * @see PARSERESULT
 */
PARSERESULT parseFrame(ECHOFRAME_PTR fptr);

/**
 * An iteration structure used for parsing the EPCs of an incoming frame.
 * Make sure to memset everything to zero before using it with getNextEPC()
 * @see getNextEPC
 */
typedef struct {
	uint8_t opc; /**< number of properties */
	uint8_t propIndex; /**< current property index? */
	uint8_t epc; /**< current property code */
	uint8_t pdc; /**< data count */
	unsigned char *edt; /**< data */
} PARSE_EPC, *PARSE_EPC_PTR;

/**
 * Used to iterate oveer the EPCs of an incoming frame.
 *
 * The fields of #PARSE_EPC epc are updated every time this is called. Usable in while loops.
 * @param a memset'd to zero PARSE_EPC struct pointer
 * @return 1 if found, zero if no next EPC found
 * @see PARSE_EPC
 */
int getNextEPC(ECHOFRAME_PTR fptr, PARSE_EPC_PTR epc);

/**
 * ...better not use these...
 */
#define getTID(x) getShort(x, OFF_TID)
#define getSEOJ(x) &x->data[OFF_SEOJ]
#define getDEOJ(x) &x->data[OFF_DEOJ]
#define getESV(x) x->data[OFF_ESV]
#define getOPC(x) x->data[OFF_OPC]
#define getEPC(x) &x->data[OFF_EPC]

/**
 * Function type for property read/write operations
 * @return the number of bytes read or written, 0 on failure.
 */
typedef int (*WRITEFUNC)(Property_PTR property, uint8_t size,
		const unsigned char *buf);
typedef int (*READFUNC)(Property_PTR property, uint8_t size, unsigned char *buf);

typedef Property_PTR (*FREEFUNC)(Property_PTR property);
/** not sure I use this any more*/
#define FREE(aptr) aptr->freeptr(aptr)

/**
 * Property write modes.
 * E_SUPPRES_NOTIFY suppresses notification generation after a write occurs on a
 * property that notifies.
 */
typedef enum {
	E_READ = 1, E_WRITE = 2, E_NOTIFY = 4, E_SUPPRESS_NOTIFY = 8,
} E_WRITEMODE;

/**
 * The structure representing a property of an object. New properties can be
 * implemented by creating new READWRITE functions and setting this structure
 * appropriately. This structure can be used with the macrolist functions, with
 * the next field holding the next property of an object.
 *
 * @see macrolist.h
 * @see OBJ
 */
struct Property {
	void *next; /**< the next property in the list */
	void *opt; /**< optional data pointer used by this property */
	OBJ_PTR pObj; /**< pointer to the echonet object that this property is part of*/
	FREEFUNC freeptr; /**< custom free function pointer, NULL for standard dealocation*/
	READFUNC readf; /**< the read function pointer (used for GETs) */
	WRITEFUNC writef; /**< the write function pointer (used for SETs) */
	uint8_t propcode; /**< 1byte property code */
	uint8_t rwn_mode; /**< access mode */
};

#define classgroup eoj[0]
#define class eoj[1]
#define instance eoj[2]

/**
 * The structure that represents an ECHONET Lite object in the code. Has an
 * EOJ, the head of the property list it contains, the next object in the node
 * as well as a pointer to the context it is managed by. This structure can be
 * used with the macrolist functions, with the next field holding the next
 * object of the node.
 */
struct OBJ {
	void *next; /**< the next object in the node */
	int i; /*!< this was used for tests \todo remove */
	Property_PTR pHead; /**< the head of the property list of this object */
	uint8_t eoj[3]; /**< EOJ */
	ECHOCTRL_PTR ectrl; /**< echonet control context pointer */
};

/**
 * frees an object. Rarely used.
 */
void freeObject(OBJ_PTR obj);

/**
 * Create an object with the given EOJ.
 * @param eoj the eoj of the object to create
 * @return a pointer to the object, NULL on failure.
 */
OBJ_PTR createObject(char *eoj);

/**
 * Register an object with the ECHONET Lite context. Use this during the
 * initial setup of objects.
 *
 * @param ectrl the ECHONET Lite context
 * @param obj the object to register
 */
void addObject(ECHOCTRL_PTR ectrl, OBJ_PTR obj);

#define setClassGroup(obj_ptr, val) (obj_ptr)->classgroup = val
#define setClass(obj_ptr, val) (obj_ptr)->class = val
#define setInstance(obj_ptr, val) (obj_ptr)->instance = val
#define setEOJ(obj_ptr, eoj)	do { \
	if (obj_ptr && eoj) { \
		memcpy(obj_ptr->eoj, eoj, 3); \
	} \
} while (0)

/**
 * Adds a property to an object. If a property with the same property code
 * already exists, it is replaced. Use during initial setup.
 *
 * @param obj the object to add the property to
 * @param property the property to register
 */
void addProperty(OBJ_PTR obj, Property_PTR property);

/**
 * Make a notification for the given property. This function will use the
 * read function pointer to acquire the latest data of this property, and
 * automatically send a broadcast notification over the network.
 */
void makeNotification(Property_PTR property);

/**
 * finalize and send a multicast notification using the outgoing frame. Use this
 * if you want to craft a custom multicast notification.
 */
void sendNotification(ECHOCTRL_PTR context, ECHOFRAME_PTR outgoing);

/**
 * Read (GET) the contents/data/value of a property.
 *
 * @param property the property to read
 * @param size the size of the receiving buffer
 * @param buffer the buffer to store the data at.
 *
 * @return number of bytes read, 0 or negative on failure.
 */
int readProperty(Property_PTR property, uint8_t size, unsigned char *buf);

/**
 * Write (SET) a property with the given data
 *
 * @param property the property to write
 * @param size the size of the data to write
 * @param the actual data to write
 *
 * @return number of bytes written, 0 or negative on failure
 */
int writeProperty(Property_PTR property, uint8_t size, const unsigned char *buf);

/**
 * Find a property with a specific code in an object.
 * @param obj the obj to search in
 * @param code the property code to match
 * @return a pointer to the property, NULL on failure
 */
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

/** use this to free a list of proeprties. Rare. */
#define FREEPROPERTIES(head) while(head){head = freeProperty(head);}

/**
 * Create a property.
 * @param mode the access mode (or'ed bitfield)
 * @param propcode the property code
 * @return a pointer to the property, NULL on failure.
 *
 * @see E_WRITEMODE for the mode
 */
Property_PTR createProperty(uint8_t propcode, uint8_t mode);

/**
 * Creates a pure data property that stores data. If maxsize == dataSize
 * this property accepts EXACTLY maxsize bytes of info, no more no less
 * if maxsize > datasize this property accepts UP TO maxsize bytes (i.e.
 * fewer bytes are also ok). Use dataSize == maxsize  for EXACT behaviour,
 * datasize < maxsize for UP TO behaviour. NULL data acceptable in every
 * scenario.
 * @param propcode property code
 * @param rwn access mode
 * @param maxsize the maximum data size for this property
 * @param dataSize (may be 0/NULL, independent of data) the size of the initial data
 * @param data (may be NULL) the initial data.
 */
Property_PTR createDataProperty(uint8_t propcode, uint8_t rwn, uint8_t maxsize,
		uint8_t dataSize, char *data);

/** flips the appropriate bit for a property code in a bitmap */
void flipPropertyBit(uint8_t code, char *pbitmap);

/**
 * This function computes the property maps for this object and
 * writes the corresponding properties (9d,9e,9f). Make sure to
 * add properties 9d,9e,9f beforehand
 */
int computePropertyMaps(OBJ_PTR obj);

/**
 * Creates a basic object with the bare minimum necessary properties.
 * The bare minimum properties are specified in the super class object.
 *
 * @param eoj the eoj of the object to create
 * @return the pointer to the newly created object, NULL on failure.
 */
OBJ_PTR createBasicObject(char *eoj);

/** profile EOJ static definition */
#define PROFILEEOJ "\x0E\xF0\x01"
/**
 * Creates a basic node profile object with the bare minimum necessary
 * properties.
 */
OBJ_PTR createNodeProfileObject();

/**
 * Compute the contents of properties D3, D4, D5, D6, D7 of the node
 * profile object. Use this at the end of object initialization.
 *
 * @param oHead the head of the ECHONET Lite object list (found in the control context).
 */
void computeNodeClassInstanceLists(OBJ_PTR oHead);

/**
 * Get a pointer to the object with the specified eoj.
 * @param head of the object list
 * @param eoj the eoj to search for
 * @return a pointer to the object found, NULL otherwise.
 */
OBJ_PTR getObject(OBJ_PTR oHead, char *eoj);

/**
 * Macro for getting an object by passing in the elite control structure directly
 * @param ctrl the ECHONET Lite control context
 * @param eoj the eoj to search for
 */
#define GETOBJECT(ctrl, eoj) getObject(ctrl->oHead, eoj)

/**
 * I don't remember what this is for.
 * 63 /3 = 21 instances + instance number
 * \todo move to a sane place
 */
#define E_INSTANCELISTSIZE 64

/**
 * Iterator structure for matching multiple objects. Used with matchObjects.
 * Setup oHead and eoj appropriately before usage. oHead set to the head of the
 * ECHONET Lite object list, eoj the desired eoj. If eoj instance code is zero,
 * multimatch all objects only on classgroup/class code (match all instances).
 * @see matchObjects
 */
typedef struct {
	OBJ_PTR lastmatch;
	OBJ_PTR oHead;
	unsigned char *eoj;
} OBJMATCH, *OBJMATCH_PTR;

/**
 * Match all objects based on the matcher criteria. Can be used in a while loop.
 * Setup oHead and eoj of the matcher object appropriately.
 *
 * @see OBJMATCH
 * @see ECHOCTRL_PTR
 */
OBJ_PTR matchObjects(OBJMATCH_PTR matcher);

/**
 * The main receiving loop. Call this when you are ready to receive
 * ECHONET Lite UDP packets.
 */
void receiveLoop(ECHOCTRL_PTR ectrl);

#endif

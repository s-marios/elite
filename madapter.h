/**
 * \file
 *
 * Include this file for minimal middleware adapter functionality.
 *
 * In the future, full adapter functionality is planned.
 */
#ifndef MADAPTER_H
#define MADAPTER_H

//in order to reuse ECHOFRAME
#include "elite.h"

/**
 * Initialize a middleware adapter frame, used for communication with a ready
 * device.
 *
 * @param alocsize size of the new frame
 * @param ft frame type
 * @param cn command number
 * @param fn frame number
 * @return a pointer to the new adapter frame, NULL on failure
 */
ECHOFRAME_PTR initAdapterFrame(size_t alocsize, uint16_t ft, uint8_t cn,
		uint8_t fn);

/**
 * The control structure for middleware adapter functionality.
 */
typedef struct {
	FILE * out; /**< output file descriptor TOWARDS the ready device*/
	FILE * in; /**< input file descriptor FROM the ready device */
	SemaphoreHandle_t writelock; /**< a write semaphore preventing multiple writes*/
	SemaphoreHandle_t syncresponse; /**< a signal semaphore upon reception of an answer*/
	ECHOFRAME_PTR request; /**< current request packet on the wire*/
	ECHOFRAME_PTR response; /**< response to current request */
	ECHOCTRL_PTR context; /**< the echonet lite control context */
	uint8_t FN; /**< frame number source */
} MADAPTER, *MADAPTER_PTR;

/**
 * Create a middleware adapter control structure
 * @param in file descriptor representing input from the ready device. If NULL,
 * stdin is assumed.
 * @param out file descriptor representing output to the ready device. If NULL,
 * stdout is assumed.
 * @return a pointer to the middleware adapter control structure.
 */
MADAPTER_PTR createMiddlewareAdapter(FILE * in, FILE * out);

/**
 * use this to set the echonet lite context of the middleware adapter struct.
 * necessary for proper initialization.
 * @param x middleware adapter control
 * @param cont echonet lite context
 */
#define setContext( x, cont) x->context = cont

/**
 * use this whenever you need a new fn number for a frame.
 * Currently you should not have a use for this...
 * \todo make private?
 */
#define getFN(x, fn) do { \
	fn = x->FN; \
	x->FN++; \
	x->FN ? x->FN : x->FN++ ; \
} while (0)

/**
 * Create an IAupProperty. This property can be used to represent IA
 * properties of the ready device. IAGetUp and IASetUp are used as the
 * read/write functions.
 * @param propcode the code of the property to be created
 * @param rwn the access mode, see #E_WRITEMODE
 * @param adapter the middleware adapter control structure
 * @return a pointer to the newly created property, NULL otherwise
 */
Property_PTR createIAupProperty(uint8_t propcode, uint8_t rwn,
		MADAPTER_PTR adapter);

/**
 * Start the receiving task of the middleware adapter.
 * @param adapter a pointer to the middleware adapter struct
 */
void startReceiverTask(MADAPTER_PTR adapter);


#endif

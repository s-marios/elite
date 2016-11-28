#ifndef ELITE_PRIV_H
#define ELITE_PRIV_H

void flipPropertyBit(uint8_t code, char * pbitmap);
int computeListMaps(OBJ_PTR obj, char * maps);
int computeBinaryMaps(OBJ_PTR obj, char * maps);
int computeNumberOfInstances(OBJ_PTR oHead);
int computeClasses(OBJ_PTR oHead, char * result);
int computeInstances(OBJ_PTR oHead, char * result);
int processWrite(ECHOFRAME_PTR incoming, ECHOFRAME_PTR response, OBJ_PTR oHead);

typedef struct HANDLER HANDLER;
typedef struct HANDLER * HANDLER_PTR;
/**
 * definition of the function that processes outgoing frames. This handler
 * is responsible for freeing the outgoing packet. Optional arguments to this
 * can be found in handler->opt.
 */
typedef void * (*PROCESSORFUNC)(HANDLER_PTR handler, void * outgoing);

struct HANDLER {
	PROCESSORFUNC func;
	void * opt;
};

void * sendOutgoingFrame(HANDLER_PTR handler, ECHOFRAME_PTR outgoing);
void * handleOutgoingFrame(HANDLER_PTR handler, ECHOFRAME_PTR outgoing);
void * applyOutgoingHandler(HANDLER_PTR handler, ECHOFRAME_PTR outgoing);
void processIncomingFrame(ECHOFRAME_PTR incoming, OBJ_PTR oHead,
		HANDLER_PTR handler);
int processRead(ECHOFRAME_PTR incoming, ECHOFRAME_PTR response, OBJ_PTR obj,
		E_WRITEMODE rwn);



#endif

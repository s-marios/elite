/**
 * \file
 *
 * Internally used header file, not intended to be used by an application directly
 */
#ifndef ELITE_PRIV_H
#define ELITE_PRIV_H

#include "elite.h"

void flipPropertyBit(uint8_t code, char * pbitmap);
int computeListMaps(OBJ_PTR obj, char * maps);
int computeBinaryMaps(OBJ_PTR obj, char * maps);
int computeNumberOfInstances(OBJ_PTR oHead);
int computeClasses(OBJ_PTR oHead, unsigned char * result);
int computeInstances(OBJ_PTR oHead, unsigned char * result);
int processWrite(ECHOFRAME_PTR incoming, ECHOFRAME_PTR response, OBJ_PTR oHead);
int comparePropertyCode(void * prop, void * code);

void * sendOutgoingFrame(HANDLER_PTR handler, ECHOFRAME_PTR outgoing);
void * handleOutgoingFrame(HANDLER_PTR handler, ECHOFRAME_PTR outgoing);
void * applyOutgoingHandler(HANDLER_PTR handler, ECHOFRAME_PTR outgoing);
void processIncomingFrame(
    ECHOFRAME_PTR incoming, OBJ_PTR oHead, HANDLER_PTR handler);
int processRead(
    ECHOFRAME_PTR incoming, ECHOFRAME_PTR response, OBJ_PTR obj,
    E_WRITEMODE rwn);

// for internal testing
int testRead(Property_PTR property, uint8_t size, unsigned char * buf);
void initTestProperty(Property_PTR property);
int readData(Property_PTR property, uint8_t size, unsigned char * buf);
int writeData(Property_PTR property, uint8_t size, const unsigned char * buf);
int writeDataExact(
    Property_PTR property, uint8_t size, const unsigned char * buf);

#endif

#ifndef ELITE_PRIV_H
#define ELITE_PRIV_H

void flipPropertyBit(uint8_t code, char * pbitmap);
int computeListMaps(OBJ_PTR obj, char * maps);
int computeBinaryMaps(OBJ_PTR obj, char * maps);
int computeNumberOfInstances(OBJ_PTR oHead);
int computeClasses(OBJ_PTR oHead, char * result);
int computeInstances(OBJ_PTR oHead, char * result);

#endif

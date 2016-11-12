#ifndef ELITE_PRIV_H
#define ELITE_PRIV_H
void flipPropertyBit(uint8_t code, char * pbitmap);
int computeListMaps(OBJ_PTR obj, char * maps);
int computeBinaryMaps(OBJ_PTR obj, char * maps);
OBJ_PTR createBasicObject(char * eoj);


#endif

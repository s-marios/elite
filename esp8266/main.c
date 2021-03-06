/**
 * \file
 *
 * This is the main application, and currently the only example of how to
 * use the echonet lite library. Go through this code for initialization
 * examples.
 *
 * The functions named test_??????? test various parts of the system and
 * were used during development. Currently tests are disabled and a few
 * tests may not pass in their current form.
 */
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include "espressif/esp_common.h"
#include "esp/uart.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "esp8266.h"
#include "esp/gpio.h"

#include "lwip/api.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/inet.h"
#include "lwip/netif.h"

//is this really necessary?
#include "ipv4/lwip/ip.h"
#include "lwip/sockets.h"

#include "minunit.h"
#include "macrolist.h"
#include "elite.h"
#include "elite_priv.h"
#include "madapter.h"
//#include "gdbstub.h"

int tests_run = 0;
char scratch[128];

//net logging declarations

/** for net debug */
SemaphoreHandle_t debugsem;
/** for net debug */
SemaphoreHandle_t debugdowrite;
/** for net debug */
char ndbuf[256];
/** for net debug */
uint8_t ndsize;

static char * test_PRINTF() {
//#ifdef ELITE_DEBUG
//	mu_assert("PRINTF: did not print", PRINTF("hello!") == 6);
//#else
//	PRINTF("hello!");
//#endif
	return 0;
}

static char * test_allocateFrame() {
	ECHOFRAME_PTR fptr = allocateFrame(64);
	mu_assert("frame allocation failed", fptr != NULL);
	mu_assert("allocated frame size wrong", fptr->allocated == 64);
	free(fptr);
	return 0;
}

static char * test_malloc64() {
	void * malloced = malloc(64);
	mu_assert("malloc 64 bytes failed.", malloced != NULL);
	free(malloced);
	return 0;
}

static char * test_malloc32() {
	void * malloced = malloc(32);
	mu_assert("malloc 32 bytes failed.", malloced != NULL);
	free(malloced);
	return 0;
}

static char * test_malloc1() {
	void * malloced = malloc(1);
	mu_assert("malloc 1 bytes failed.", malloced != NULL);
	free(malloced);
	return 0;
}

static char * test_putgetShort() {
	ECHOCTRL_PTR ectrl = createEchonetControl();
	mu_assert("echonet control init TID not 1", ectrl->TID == 1);
	ECHOFRAME_PTR frame = initFrameDepr(ectrl, ECHOFRAME_STDSIZE, 0);
	mu_assert("frame init failed", frame != NULL);
	mu_assert("putShort failed", putShort(frame, 0xABCD) != -1);
	mu_assert("frame size(used) is not 6", frame->used == 6);
	mu_assert("frame short insert first byte fail", frame->data[4] == 0xAB);
	mu_assert("frame short insert first byte fail", frame->data[5] == 0xCD);
	mu_assert("getShort failed horribly", getShort(frame, 4) == 0xABCD);
	return 0;
}

static char * test_initFrame() {
	ECHOCTRL_PTR ectrl = createEchonetControl();
	mu_assert("echonet control init TID not 1", ectrl->TID == 1);

	ECHOFRAME_PTR frame = initFrameDepr(ectrl, ECHOFRAME_STDSIZE, 0);
	mu_assert("frame init failed", frame != NULL);
	mu_assert("frame header invalid byte 1", frame->data[0] == E_HD1);
	mu_assert("frame header invalid byte 1", frame->data[1] == E_HD2);
	PPRINTF("getShort result: %d \n", getShort(frame, 2));
	PPRINTF("first four bytes: %x %x %x %x\n", frame->data[0], frame->data[1],
			frame->data[2], frame->data[3]);

	mu_assert("frame TID invalid", getShort(frame, 2) == 1);
	mu_assert("frame epc counter is not 0", frame->propNum == 0);

	char * data = "data";
	mu_assert("putBytes before: propNum not zero", frame->propNum == 0);
	putBytes(frame, 4, data);
	mu_assert("putBytes: failed", memcmp(data, &frame->data[4], 4) == 0);
	mu_assert("putBytes: size increase failed", frame->used == 8);
	mu_assert("putBytes: messed propNum", frame->propNum == 0);

	free(ectrl);
	free(frame);
	return 0;
}

static char * test_putEOJESV() {
	ECHOCTRL_PTR ectrl = createEchonetControl();
	ECHOFRAME_PTR frame = initFrameDepr(ectrl, 0, 0);

	mu_assert("after initFrame :frame epc counter not zero",
			frame->propNum == 0);

	mu_assert("getTID failed", getTID(frame) == 1);

	EOJ seoj;
	SETSUPERCLASS(seoj, 0x01);
	SETCLASS(seoj, 0x02);
	SETINSTANCE(seoj, 0x03);
	mu_assert("SET SUPERCLASS FAILED", seoj[0] == 0x01);
	mu_assert("SET CLASS FAILED", seoj[1] == 0x02);
	mu_assert("SET INSTANCE FAILED", seoj[2] == 0x03);
	mu_assert("GET SUPERCLASS FAILED", GETSUPERCLASS(seoj) == 0x01);
	mu_assert("GET CLASS FAILED", GETCLASS(seoj) == 0x02);
	mu_assert("GET INSTANCE FAILED", GETINSTANCE(seoj) == 0x03);

	mu_assert("before putEOJ1 :frame epc counter not zero",
			frame->propNum == 0);
	putEOJ(frame, seoj);
	mu_assert("SEOJ[0] not 0x01", frame->data[4] == 0x01);
	mu_assert("SEOJ[1] not 0x02", frame->data[5] == 0x02);
	mu_assert("SEOJ[2] not 0x03", frame->data[6] == 0x03);

	mu_assert("after putEOJ1 :frame epc counter not zero", frame->propNum == 0);

	mu_assert("frame size is not 7", frame->used == 7);

	EOJ deoj;
	SETEOJ(deoj, 0x04, 0x05, 0x06);
	mu_assert("GET SUPERCLASS FAILED 2", GETSUPERCLASS(deoj) == 0x04);
	mu_assert("GET CLASS FAILED 2", GETCLASS(deoj) == 0x05);
	mu_assert("GET INSTANCE FAILED 2", GETINSTANCE(deoj) == 0x06);

	putEOJ(frame, deoj);
	mu_assert("after putEOJ2 :frame epc counter: %d\n", frame->propNum == 0);

	mu_assert("frame size is not 10", frame->used == 10);

	mu_assert("before ESVnOPC :frame epc counter: %d\n", frame->propNum == 0);

	mu_assert("CMPEOJ: different eoj are same", CMPEOJ(seoj, deoj) != 0);
	mu_assert("getSEOJ failed", CMPEOJ(seoj, getSEOJ(frame)) == 0);
	mu_assert("getDEOJ failed", CMPEOJ(deoj, getDEOJ(frame)) == 0);

	putESVnOPC(frame, ESV_SETI);
	mu_assert("putESVnOPC failed", frame->data[10] == ESV_SETI);
	mu_assert("getESV failed", getESV(frame) == ESV_SETI);
	mu_assert("frame size is not 12", frame->used == 12);
	mu_assert("epc number non-zero (1)", frame->data[OFF_OPC] == 0);
	mu_assert("getOPC failed (1)", getOPC(frame) == 0);
	char * data = "data";
	putEPC(frame, 0x80, 4, data);
	mu_assert("frame epc counter is not 1", frame->propNum == 1);
	mu_assert("frame size is not 18", frame->used == 18);
	mu_assert("EPC is wrong", frame->data[12] == 0x80);
	mu_assert("EPC size is wrong", frame->data[13] == 4);
	mu_assert("EPC DATA[0] is wrong", frame->data[14] == 'd');
	mu_assert("EPC_DATA (all) is wrong",
			memcmp(&frame->data[14], data, 4) == 0);
	mu_assert("epc number non-zero (1)", frame->data[OFF_OPC] == 0);
	finalizeFrame(frame);
	mu_assert("frame opc number is wrong", frame->data[OFF_OPC] == 1);
	mu_assert("getOPC failed (2)", getOPC(frame) == 1);
	dumpFrame(frame);

	free(ectrl);
	free(frame);
	return 0;
}

static char * test_parseFrame() {
	ECHOCTRL_PTR ectrl = createEchonetControl();
	ECHOFRAME_PTR frame = initFrameDepr(ectrl, 0, 0);
	mu_assert("parseFrame: empty frame detection failed",
			parseFrame(NULL) == PR_NULL);
	mu_assert("parseFrame: header/tid frame too short detection failed",
			parseFrame(frame) == PR_TOOSHORT);
	EOJ eoj;
	SETEOJ(eoj, 0x01, 0x02, 0x03);
	//sender
	putEOJ(frame, eoj);
	//receiver
	putEOJ(frame, eoj);
	mu_assert("parseFrame: eoj frame too short detection failed",
			parseFrame(frame) == PR_TOOSHORT);
	putESVnOPC(frame, ESV_GET);
	mu_assert("parseFrame: ESVOPC frame too short detection failed",
			parseFrame(frame) == PR_TOOSHORT);
	putEPC(frame, 0x80, 0, NULL);
	int result = parseFrame(frame);
	//PPRINTF("parse shortest frame result: %d opc:%d\n", result, frame->propNum);
	//have not called frame finalizer!
	mu_assert("parseFrame: opc zero detection failed", result == PR_OPCZERO);

	//shortest frame test
	finalizeFrame(frame);
	mu_assert("parseFrame: shortest correct frame length not 14",
			frame->used == 14);
	mu_assert("parseFrame: shortest correct frame discarded",
			parseFrame(frame) == PR_OK);
	mu_assert("parseFrame: shortest correct frame opc not 1",
			frame->propNum == 1);
	//add a few more properties...
	putEPC(frame, 0x81, 1, "1");
	finalizeFrame(frame);

	dumpFrame(frame);
	mu_assert("parseFrame: created frame parse not OK (1)",
			parseFrame(frame) == PR_OK);

	putEPC(frame, 0x90, 4, "data");
	finalizeFrame(frame);
	mu_assert("parseFrame: created frame parse not OK (2)",
			parseFrame(frame) == PR_OK);

	putEPC(frame, 0xFF, 6, "hello!");
	finalizeFrame(frame);
	mu_assert("parseFrame: created frame parse not OK (3)",
			parseFrame(frame) == PR_OK);

	mu_assert("parseFrame: created frame propNum is not 4",
			frame->propNum == 4);
	PPRINTF("frame size: %d\n", frame->used);
	mu_assert("parseFrame: created frame lenght not 31", frame->used == 31);
	mu_assert("parseFrame: created frame parse not OK",
			parseFrame(frame) == PR_OK);
	free(ectrl);
	free(frame);
	return 0;
}

static char * test_getNextEPC() {

	ECHOCTRL_PTR ectrl = createEchonetControl();
	ECHOFRAME_PTR frame = initFrameDepr(ectrl, 0, 0);
	EOJ eoj;
	SETEOJ(eoj, 0x01, 0x02, 0x03);
	putEOJ(frame, eoj);
	putEOJ(frame, eoj);
	putESVnOPC(frame, ESV_SETI);
	static char * data[] = { "1", "22", "333", "4444" };
	static uint8_t propcodes[] = { 0x80, 0x81, 0x82, 0x83 };
	for (int i = 0; i < 4; i++) {
		putEPC(frame, propcodes[i], i + 1, data[i]);
	}
//	putEPC(frame, 0x80, 1, "1");
//	putEPC(frame, 0x81, 2, "22");
//	putEPC(frame, 0x82, 3, "333");
//	putEPC(frame, 0x83, 4, "4444");
	finalizeFrame(frame);
	mu_assert("getNextEPC: frame parsing failed", parseFrame(frame) == PR_OK);
	PARSE_EPC_PTR prop_ptr = malloc(sizeof(PARSE_EPC));
	memset(prop_ptr, 0, sizeof(PARSE_EPC));

	int i = 0;
	while (getNextEPC(frame, prop_ptr)) {
		PPRINTF("getNextEPC repetition: %d\n", i);
		mu_assert("getNextEPC epc not match", prop_ptr->epc == propcodes[i]);
		mu_assert("getNextEPC pdc not match", prop_ptr->pdc == i + 1);
		mu_assert("getNextEPC edt not match",
				memcmp(prop_ptr->edt, data[i], prop_ptr->pdc) == 0);
		i++;
	}

	free(ectrl);
	free(frame);
	free(prop_ptr);
	return 0;
}

static char * test_MLISTS() {
	int num = 4;
	OBJ_PTR object_p = malloc(sizeof(OBJ) * num);
	memset(object_p, 0, sizeof(OBJ) * num);

	OBJ_PTR p1 = object_p + 1;
	p1->i = 1;
	OBJ_PTR p2 = p1 + 1;
	p2->i = 2;
	mu_assert("empty head has next", LHASNEXT(object_p) == NULL);
	LAPPEND(&object_p, p1);
	PRINTF("object_p->next is: %s\n", object_p ? "true" : "false");
	mu_assert("next field not adjusted properly", object_p->next == p1);
	mu_assert("p1->next is not null", p1->next == NULL);
	mu_assert("p1 i is not 1 (1)", p1->i == 1);
	LAPPEND(&object_p, p2);
	mu_assert("object_p->next changed", object_p->next == p1);

	FOREACHPURE(object_p)
	{
		OBJ_PTR tobj = (OBJ_PTR) element;
		PRINTF("test next empty? %s\n", tobj->next ? "false" : "true");
	}

	FOREACH(object_p, OBJ_PTR)
	{
		PRINTF("elem i: %d, next empty? %s\n", element->i,
				element->next ? "false" : "true");
	}

	PRINTF("p1->next: %d, p2: %d\n", p1->next, p2);
	PRINTF("p1 i: %d, p2 i: %d\n", p1->i, p2->i);
	mu_assert("p1 i not 1 (2)", p1->i == 1);
	mu_assert("p2_i not 2", p2->i == 2);
	mu_assert("p1 next field not correct", p1->next == p2);

	OBJ_PTR p4 = object_p + 3;
	p4->i = 4;
	LPREPEND(&object_p, p4);
	FOREACH(object_p, OBJ_PTR)
	{
		PRINTF("elem i: %d, next empty? %s\n", element->i,
				element->next ? "false" : "true");
	}
	mu_assert("LPREPEND did not update head", object_p == p4);
	free(object_p);
	return 0;
}

static char * test_testProperty() {
	char * buf = malloc(16);
	Property_PTR property = malloc(sizeof(Property));
	initTestProperty(property);
	mu_assert("testProperty: read is not test read",
			property->readf == testRead);
	mu_assert("testProperty: write is not NULL", property->writef == NULL);
	mu_assert("testProperty: opcode is not 0x80", property->propcode == 0x80);
	mu_assert("testProperty: next is not NULL", property->next == NULL);
	int res = readProperty(property, 16, buf);
	mu_assert("testProperty (read): read failed", res > 0);
	mu_assert("testProperty (read): read bytes not 4", res == 4);
	mu_assert("testProperty (read): data not in buffer",
			memcmp(buf, "test", res) == 0);
	mu_assert("testProperty (write): write result not -1",
			writeProperty(property, 0, NULL) == -1);

	freeProperty(property);
	free(buf);
	return 0;
}

static char * test_testDataProperty() {
	Property_PTR property = createDataProperty(0x80, E_READ | E_WRITE, 4, 4,
			"test");
	//test a bunch of things... all of the things.
	mu_assert("testDataProperty: property is null", property != NULL);
	mu_assert("testDataProperty: property code is not 0x80",
			property->propcode == 0x80);
	mu_assert("testDataProperty: property access is not READWRITE",
			property->rwn_mode == (E_READ | E_WRITE));
	char * buf = malloc(16);

	int res = readProperty(property, 16, buf);
	mu_assert("testDataProperty (read): read failed", res != 0);
	mu_assert("testDataProperty (read): read count incorrect", res == 4);
	mu_assert("testDataProperty (read): read buffer incorrect",
			memcmp(buf, "test", 4) == 0);

	res = writeProperty(property, 4, "abcd");
	mu_assert("testDataProperty (write): exact write failed", res != 0);
	mu_assert("testDataProperty (write): exact write count incorrect",
			res == 4);
	readProperty(property, 4, buf);
	mu_assert("testDataProperty (write): exact write buffer incorrect",
			memcmp(buf, "abcd", 4) == 0);
	res = writeProperty(property, 3, "GOM");
	mu_assert("testDataProperty (write): nonexact write succeeded", res == 0);
	res = writeProperty(property, 5, "GOMII");
	mu_assert("testDataProperty (write): write longer than 4 succeeded",
			res == 0);
	freeProperty(property);

	//UPTO property
	char empty[] = "\0\0\0\0\0\0\0\0";
	property = createDataProperty(0x80, E_READ | E_WRITE, 12, 8, empty);
	res = readProperty(property, 16, buf);
	mu_assert("testDataProperty (read init): bytes read not 8", res == 8);
	mu_assert("testDataProperty (read init): non-null data were read",
			memcmp(buf, empty, 8) == 0);
	res = writeProperty(property, 4, "test");
	mu_assert("testDataProperty (write): upto write failed", res == 4);

	res = readProperty(property, 4, buf);
	mu_assert("testDataProperty (write): upto write buffer incorrect",
			memcmp(buf, "test", 4) == 0);

	freeProperty(property);
	free(buf);

	return 0;
}

static char * test_freePropertyAndEOJ() {
	char anEoj[] = { 0, 1, 1 };
	OBJ_PTR obj = createObject("\x00\x01\x01");
	mu_assert("object eoj not set properly", memcmp(obj->eoj, anEoj, 3) == 0);
	mu_assert("object: classgroup macro wrong", obj->classgroup == 0);
	mu_assert("object: class macro wrong", obj->class == 1);
	mu_assert("object: instance macro wrong", obj->instance == 1);

#define propsize 4
	Property_PTR props[propsize + 1];
	props[propsize] = NULL;
	//fix the first property as a data property
	props[0] = createDataProperty(0x80, E_READ | E_WRITE | E_NOTIFY, 1, 1,
	NULL);

	mu_assert("property code[0] does not match", props[0]->propcode == 0x80);
	//propnum for other properties
	uint8_t mappropcode = 0x9D;
	int i = 1;
	for (i = 1; i < propsize; i++) {
		props[i] = createProperty(mappropcode++, E_READ);

		//string the props together
		LAPPEND(&props[0], props[i]);
		PRINTF("property created: %d\n", i);
		mu_assert("property creation failed", props[i]);
		mu_assert("property code does not match",
				props[i]->propcode == (mappropcode - 1));

	}
	mu_assert("i index not 4 (1st)", i == 4);

	i = 0;
	FOREACH(props[0], Property_PTR)
	{
		mu_assert("4 props: broken chain", element->next == props[++i]);
	}
	mu_assert("i index not 4 (2nd)", i == 4);

	void * ptr = NULL;
	i = 0;
	while ((ptr = freeProperty(props[i]))) {
		PRINTF("free loop: %d\n", i);
		mu_assert("freeProperty: incorrect return value", ptr == props[i + 1]);
		i++;
	}
	mu_assert("i index not 4 (3rd)", i == 3);
	PPRINTF("4 props: cleared\n");
	free(obj);
	return 0;
}

static char * test_LFINDREPLACE() {
#define propsize 10
	Property_PTR props[propsize + 1];
	Property_PTR prev = NULL;
	Property_PTR toFind = createProperty(0xFF, E_READ);
	Property_PTR replacement = createProperty(0xA0, E_READ);
	Property_PTR found = NULL;

	uint8_t epc = 0x80;
	for (int i = 0; i < propsize; i++) {
		props[i] = createProperty(epc++, E_READ | E_WRITE);
		LAPPEND(&prev, props[i]);
	}
	mu_assert("LFINDREPLACE: prev not head", prev == props[0]);
	props[propsize] = NULL;

	found = LFIND(props[0], toFind, compareProperties);
	mu_assert("found should not have been found", found == NULL);
	toFind->propcode = 0x88;
	found = LFIND(props[0], toFind, compareProperties);
	mu_assert("found is not props[8]", found == props[8]);
	for (int i = 0; i < propsize; i++) {
		toFind->propcode = 0x80 + i;
		found = LFIND(props[0], toFind, compareProperties);
		mu_assert("found is not props[i]", found == props[i]);
	}

	toFind->propcode = 0x81;

	//replacement test
	//Property_PTR phead = props[0];
	found = LFIND(props[0], toFind, compareProperties);
	mu_assert("LREPLACE (before rep): target not found", found != NULL);
	found = LREPLACE(&props[0], found, replacement);
	mu_assert("LREPLACE: found is null", found != NULL);
	mu_assert("LREPLACE: found is not old prop0", found->propcode == 0x81);
	Property_PTR gotten = LGET(props[0], 1);
	mu_assert("LGET: gotten is null", gotten != NULL);
	mu_assert("LREPLACE: prop[1] was not replaced", gotten->propcode == 0xA0);

	//replace the head
	Property_PTR oldhead = props[0];
	found = LREPLACE(&props[0], props[0], props[1]);
	mu_assert("LREPLACE: head not replaced", props[0]->propcode == 0x81);
	mu_assert("LREPLACE: old head is wrong", found->propcode = 0x80);
	freeProperty(found);

	int i = 0;
	PRINTF("LFIND FREE: ");
	for (Property_PTR aptr = props[0]; aptr; aptr = freeProperty(aptr)) {
		PRINTF("%d, ", i++);
	}
	PRINTF(" done.\n");
	mu_assert("LREPLACE: prop list length changed", i == propsize);
	freeProperty(replacement);
	PRINTF("free 1");
	freeProperty(toFind);
	PRINTF("we done?");
	return 0;
}

static char * test_flipPropertyBit() {
	PRINTF("test property bit\n");
	char * bitmap = malloc(17 * sizeof(uint8_t));
	memset(bitmap, 0, 17 * sizeof(uint8_t));
	flipPropertyBit(0x80, bitmap);
	mu_assert("flip: 0x80 incorrect flip", bitmap[1] & 0x01);
	flipPropertyBit(0xF0, bitmap);
	mu_assert("flip: 0xF0 incorrect flip", bitmap[1] & 0x80);
	flipPropertyBit(0xBF, bitmap);
	mu_assert("flip: 0xBF incorrect flip", bitmap[16] & 0x08);
	flipPropertyBit(0xFF, bitmap);
	mu_assert("flip: 0xFF incorrect flip", bitmap[16] & 0x80);
	flipPropertyBit(0x8F, bitmap);
	mu_assert("flip: 0x8F incorrect flip", bitmap[16] & 0x01);
	flipPropertyBit(0xEC, bitmap);
	mu_assert("flip: 0xEC incorrect flip", bitmap[13] & 0x40);
	free(bitmap);
	return 0;
}

static char * test_createBasicObject() {
	PRINTF("test create basic object");
	OBJ_PTR obj = (OBJ_PTR) createBasicObject("\x01\x02\x03");
	uint8_t codes[] = { 0x80, 0x81, 0x82, 0x88, 0x8A, 0x9D, 0x9E, 0x9F };
	for (int i = 0; i < sizeof(codes); i++) {
		Property_PTR prop = (Property_PTR) LFIND(obj->pHead, codes[i],
				comparePropertyCode);
		sprintf(scratch, "create basic: wrong prop code (%d)", i);
		mu_assert(scratch, prop->propcode == codes[i]);
	}
	computePropertyMaps(obj);
	//maps computed, time to check correct values.
	uint8_t ncodes[] = { 0x80, 0x81, 0x88, 0x9D, 0x9E, 0x9F };
	Property_PTR prop = getProperty(obj, 0x9D);
	mu_assert("create basic: no 0x9D", prop != NULL && prop->propcode == 0x9D);
	int res = readProperty(prop, 17, scratch);
	PPRINTF("Read result: %d\n", res);
	mu_assert("create basic: cannot read nbitmap", res > 0);

	for (int i = 0; i < sizeof(ncodes); i++) {
		mu_assert("create basic: wrong notify list map",
				memcmp(&scratch[1], ncodes, sizeof(ncodes)) == 0);
	}
	mu_assert("create basic: wrong notify list number",
			scratch[0] == sizeof(ncodes));
	//test the binary map
	for (int i = 0; i < 16; i++) {
		addProperty(obj, createProperty(0x8F + i * 0x10, E_READ | E_NOTIFY));
	}
	computePropertyMaps(obj);
	readProperty(getProperty(obj, 0x9D), 17, scratch);
	mu_assert("create basic: wrong notify bit number",
			16 + sizeof(ncodes) == scratch[0]);
	mu_assert("create basic: wrong notify bit map (1)", scratch[16] == 0xFF);
	mu_assert("cb: 9E", scratch[15] & 0x02);
	mu_assert("cb: 9D", scratch[14] & 0x02);
	mu_assert("cb: 88", scratch[9] & 0x01);
	mu_assert("cb: 81", scratch[2] & 0x01);
	mu_assert("cb: 80", scratch[1] & 0x01);
	FREEPROPERTIES(obj->pHead);

	return 0;
}

static char * test_propertyMaps() {
	return 0;
}

static char * test_createNodeProfileObject() {
	PRINTF("create node profile object");
	OBJ_PTR profile = createNodeProfileObject();
	int propNum = 0;
	FOREACH(profile->pHead, Property_PTR)
	{
		propNum++;
	}
	mu_assert("createNPO: wrong number of properties", propNum == 12);
	Property_PTR property = getProperty(profile, 0x8A);
	int res = readProperty(property, 3, scratch);
	mu_assert("createNPO: 0x8A read size wrong", res == 3);
	mu_assert("createNPO: 0x8A read contents wrong",
			memcmp(scratch, "AAA", 3) == 0);
	uint8_t propNums[] = { 0x8A, 0x9D, 0x9E, 0x9F, 0x80, 0x82, 0x83, 0xD3, 0xD4,
			0xD5, 0xD6, 0xD7 };
	for (int i = 0; i < sizeof(propNums); i++) {
		sprintf(scratch, "createNPO: unavailable prop (index = %d)", i);
		property = getProperty(profile, propNums[i]);
		mu_assert(scratch, property != NULL);
	}

	freeObject(profile);
	return 0;
}

static char * test_computeClassesAndInstances() {
	OBJ_PTR profile = createNodeProfileObject();
	mu_assert("node profile: wrong EOJ", CMPEOJ(profile->eoj, PROFILEEOJ) == 0);
	char * eojs[] = { "\x01\x02\x01", "\x01\x02\x02", "\x03\x04\x05" };
	OBJ_PTR objs[3];
	for (int i = 0; i < 3; i++) {
		objs[i] = createBasicObject(eojs[i]);
		LAPPEND(&profile, objs[i]);
	}
	int objnum = 0;
	FOREACHPURE(profile)
	{
		objnum++;
	}
	mu_assert("CI: not four objects", objnum == 4);

	computeNodeClassInstanceLists(profile);

	Property_PTR property = NULL;
	//d3 check
	property = getProperty(profile, 0xD3);
	int res = readProperty(property, 3, scratch);
	PPRINTF("we crash here?");
	PPRINTF("ni: %d res: %d\n", scratch[2], res);
	mu_assert("CI: read size of d3", res == 3);
	mu_assert("CI: d3 wrong number of instances", scratch[2] == 3);
	property = getProperty(profile, 0xD4);
	res = readProperty(property, 2, scratch);
	PPRINTF("nc: %d res: %d\n", scratch[1], res);
	mu_assert("CI: d4 read size", res == 2);
	mu_assert("CI: d4 wrong number of classes", scratch[1] == 3);
	//d7
	property = getProperty(profile, 0xD7);
	res = readProperty(property, 17, scratch);
	PPRINTF("d7 size: %d\n", res);
	mu_assert("CI: d7 read size", res == 5);
	mu_assert("CI: d7 data", memcmp(scratch, "\x02\x01\x02\x03\x04", res) == 0);

	//d6
	property = getProperty(profile, 0xD6);
	res = readProperty(property, 17, scratch);
	PPRINTF("d6 size: %d\n", res);
	mu_assert("CI: d6 read size", res == 10);
	mu_assert("CI: d6 data",
			memcmp(scratch, "\x03\x01\x02\x01\x01\x02\x02\x03\x04\x05", res)
					== 0);

	//d5 is the same as d6
	property = getProperty(profile, 0xD5);
	res = readProperty(property, 17, scratch);
	PPRINTF("d5 size: %d\n", res);
	mu_assert("CI: d5 read size", res == 10);
	mu_assert("CI: d5 data",
			memcmp(scratch, "\x03\x01\x02\x01\x01\x02\x02\x03\x04\x05", res)
					== 0);

	//matcher test - don't want to recreate everything..
	OBJMATCH m;
	OBJMATCH_PTR matcher = &m;
	memset(matcher, 0, sizeof(OBJMATCH));
	matcher->oHead = profile;
	int i = 0;
	//single match

	matcher->eoj = "\x03\x04\x05";
	while (matchObjects(matcher)) {
		mu_assert("matcher (single): null match", matcher->lastmatch != NULL);
		mu_assert("matcher (single): object does not match",
				matcher->lastmatch == objs[2]);
		i++;
	}
	mu_assert("matcher (single): object matched count not 1", i == 1);
	mu_assert("matcher (single): not null after end",
			matcher->lastmatch == NULL);

	//multi match
	matcher->eoj = "\x01\x02\x00";
	i = 0;
	while (matchObjects(matcher)) {
		sprintf(scratch, "matcher: %d th object is not matched\n", i);
		//mu_assert(scratch,
		//		CMPEOJ(matcher->lastmatch->eoj, eojs[i]) == 0);
		mu_assert("matcher: is null", matcher->lastmatch != NULL);
		mu_assert(scratch, matcher->lastmatch == objs[i]);
		i++;
	}
	mu_assert("matcher: 2 objects matched", i == 2);
	mu_assert("matcher: not null after end", matcher->lastmatch == NULL);

	matcher->eoj = "\xff\xff\xff"; //non-existing object
	i = 0;
	while (matchObjects(matcher)) {
		i++;
	}
	mu_assert("matcher (non-existing): matched something!!",
			matcher->lastmatch == NULL && i == 0);

	FOREACH(profile, OBJ_PTR)
	{
		freeObject(element);
	}

	return 0;
}

/**
 * This was an outgoing processor used instead of sending a packet out
 * on the network. Helped for basic testing.
 */
void * outgoingTestProcessor(HANDLER_PTR handler, void * out) {
	static uint8_t processorIndex = 0;
	ECHOFRAME_PTR outgoing = (ECHOFRAME_PTR) out;
	//store it in opt
	ECHOFRAME_PTR * frames = (ECHOFRAME_PTR *) &handler->opt;
	if (frames[processorIndex] != NULL) {
		free(frames[processorIndex]);
	}
	frames[processorIndex] = outgoing;
	processorIndex = (processorIndex + 1) % 8;
	return 0;
}

HANDLER_PTR createTestHandler() {
	size_t size = sizeof(HANDLER) + 8 * sizeof(ECHOFRAME_PTR);
	HANDLER_PTR handler = malloc(size);
	memset(handler, 0, size);
	handler->func = outgoingTestProcessor;
	return handler;
}

void destroyTestHandler(HANDLER_PTR handler) {
	ECHOFRAME_PTR * frames = (ECHOFRAME_PTR *) &handler->opt;
	for (int i = 0; i < 8; i++) {
		if (frames[i] != NULL) {
			free(frames[i]);
		}
	}
	free(handler);
}

static char * test_processIncoming() {
	OBJ_PTR profile = createNodeProfileObject();
	char * eojs[] = { "\x01\x02\x01", "\x01\x02\x02", "\x03\x04\x05" };
	OBJ_PTR objs[3];
	for (int i = 0; i < 3; i++) {
		objs[i] = createBasicObject(eojs[i]);
		LAPPEND((void **) &profile, objs[i]);
		computePropertyMaps(objs[i]);
	}
	computeNodeClassInstanceLists(profile);
	Property_PTR property = NULL;

	//create test handler
	HANDLER_PTR handler = createTestHandler();

	//create a request frame
	int RTID = 512;
	ECHOFRAME_PTR request = initFrame(48, RTID);
	unsigned char * seoj = (unsigned char *) "\x04\x04\x04";
	unsigned char * deoj = (unsigned char *) "\x01\x02\x00";
	putEOJ(request, seoj);
	putEOJ(request, deoj);
	putESVnOPC(request, ESV_GET);
	putEPC(request, 0x82, 0, NULL);
	putEPC(request, 0x8A, 0, NULL);
	finalizeFrame(request);

	processIncomingFrame(request, profile, handler);

	//time to do our tests
	/***********
	 * READS
	 ***********/
	PPRINTF("pIF reads\n");
	ECHOFRAME_PTR * frames = (ECHOFRAME_PTR *) &handler->opt;
	int responses = 0;
	for (int i = 0; i < 8; i++) {
		if (frames[i] != NULL) {
			responses++;
		}
	}
	mu_assert("pIF: number of responses not 2", responses == 2);
	char * responsedata = "\x82\x04\x00\x00" "H\00\x8A\x03" "AAA";
	for (int i = 0; i < 8; i++) {
		if (frames[i] != NULL) {
			PPRINTF("pIF: %d-th iteration\n", i);
			dumpFrame(frames[i]);
			mu_assert("pIF: wrong TID", getTID(frames[i]) == RTID);
			mu_assert("pIF: wrong deoj", CMPEOJ(getDEOJ(frames[i]), seoj) == 0);
			mu_assert("pIF: wrong seoj",
					CMPEOJ(getSEOJ(frames[i]), eojs[i]) == 0);
			mu_assert("pIF: wrong ESV", getESV(frames[i]) == ESV_GETRES);
			mu_assert("pIF: wrong OPC", getOPC(frames[i]) == 2);

			mu_assert("pIF: sample data wrong",
					memcmp(responsedata, getEPC(frames[i]), sizeof(responsedata)-1) == 0);
		}
	}

	//introduce non-existent property
	putEPC(request, 0xF1, 0, NULL);
	finalizeFrame(request);
	processIncomingFrame(request, profile, handler);
	//responses at frames 2 & 3 i.e. offset 2
	frames += 2;
	responsedata = "\x82\x04\x00\x00H\00\x8A\x03AAA\xF1\x00";
	for (int i = 0; i < 2; i++) {
		dumpFrame(frames[i]);
		mu_assert("pIF: success with non-existant property",
				getESV(frames[i]) == ESV_GETC_SNA);
		mu_assert("pIF (non-e): sample data wrong",
				memcmp(responsedata, getEPC(frames[i]), sizeof(responsedata) -1) == 0);
	}
	free(request);

	/*******
	 * WRITES
	 *******/
	PPRINTF("pIF writes");
	//add some writable properties for testing.
	for (int i = 0; i < 2; i++) {
		addProperty(objs[i],
				createDataProperty(0xE0, E_READ | E_WRITE, 4, 0, NULL));
	}
	request = initFrame(48, RTID + 1);
	putEOJ(request, seoj);
	putEOJ(request, deoj);
	putESVnOPC(request, ESV_SETC);
	putEPC(request, 0xE0, 4, "TEST");
	finalizeFrame(request);

	responsedata = "\xE0\x00";
	processIncomingFrame(request, profile, handler);
	frames += 2;
	for (int i = 0; i < 2; i++) {
		dumpFrame(frames[i]);
		mu_assert("pIF (write): sample data wrong",
				memcmp(getEPC(frames[i]), responsedata, 2) == 0);
		readProperty(getProperty(objs[i], 0xE0), 8, scratch);
		mu_assert("pIF (write): written data mismatch",
				memcmp(scratch, "TEST", 4) == 0);
	}
	//add non-existent property
	putEPC(request, 0xFE, 4, "FAIL");
	finalizeFrame(request);

	responsedata = "\xE0\x00\xFE\x04" "FAIL";
	processIncomingFrame(request, profile, handler);
	frames += 2;
	for (int i = 0; i < 2; i++) {
		dumpFrame(frames[i]);
		mu_assert("pIF (write fail): wrong ESV",
				getESV(frames[i]) == ESV_SETC_SNA);
		mu_assert("pIF (write fail): wrong data",
				memcmp(getEPC(frames[i]), responsedata, 7) == 0);
	}

	free(request);
	destroyTestHandler(handler);
	FOREACH(profile, OBJ_PTR)
	{
		freeObject(element);
	}

	return 0;
}

static char * allTests() {
	mu_run_test(test_PRINTF);
	mu_run_test(test_malloc1);
	mu_run_test(test_malloc32);
	mu_run_test(test_malloc64);
	mu_run_test(test_allocateFrame);
	mu_run_test(test_putgetShort);
	mu_run_test(test_initFrame);
	mu_run_test(test_putEOJESV);
	mu_run_test(test_parseFrame);
	mu_run_test(test_getNextEPC);
	mu_run_test(test_MLISTS);
	mu_run_test(test_testProperty);
	mu_run_test(test_testDataProperty);
	mu_run_test(test_freePropertyAndEOJ);
	mu_run_test(test_LFINDREPLACE);
	mu_run_test(test_flipPropertyBit);
	mu_run_test(test_createBasicObject);
	mu_run_test(test_propertyMaps);
	mu_run_test(test_createNodeProfileObject);
	mu_run_test(test_computeClassesAndInstances);
	mu_run_test(test_processIncoming);
	return 0;
}

/**
 * the main test task. if you want to enable tests again, create a task
 * that has this function as its entry point.
 */
void runTestsTask(void * params) {
	PPRINTF("running tests... \n");
	char * result = allTests();
	if (result != 0) {
		PPRINTF("%s\n", result);
	} else {
		PPRINTF("ALL TESTS PASSED.");
	}

	//genuinely WTF:
	//if this task returns the system crashes.
	//so... sleep forever!
	while (1) {
		vTaskDelay(portMAX_DELAY);
	}
}

/**
 * the Wifi SSID to connect to. Change this to your needs.
 */
#define WIFI_SSID "testnet"
/**
 * the password used to connet to the wifi. Change this to your needs.
 */
#define WIFI_PASSWORD "password"
/**
 * This function is used to setup the wireless connection. Currently it is
 * using DHCP. Furthermore, it initializes IGMP, with ESP8266 specific calls
 * and registers.
 */
void setupWirelessIF() {

	struct sdk_station_config config = { .ssid = WIFI_SSID, .password =
	WIFI_PASSWORD, };
	sdk_wifi_set_opmode(STATION_MODE);
	sdk_wifi_station_set_config(&config);

	printf("before connect");
	PPRINTF("connecting to wifi");
	while (sdk_wifi_station_get_connect_status() != STATION_GOT_IP) {
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		PPRINTF(".");
	}
	PPRINTF("connected.");

	/****** netif IGMP INIT *************/
	printf("before igmp");
	//struct netif* netif = eagle_lwip_getif(STATION_IF);
	struct netif* netif = netif_default;
	PPRINTF("default netif is NULL? :%s\n", netif == NULL ? "yes" : "no");
	if (NULL != netif) {

		if (!(netif->flags & NETIF_FLAG_IGMP)) {
			netif->flags |= NETIF_FLAG_IGMP;
			printf("start igmp\n");
			igmp_start(netif);
			printf("end igmp\n");
		}
	}

}

/**
 * This creates the debug socket. Unfortunately, this is quite unreliable
 * and not a substitute for proper debugging facilities. Use this in desperate
 * times only, disable it through #DOWEBLOG if not needed.
 */
int createDebugSocket() {
	int dsock = socket(AF_INET, SOCK_STREAM, 0);
	if (dsock < 0) {
		return dsock;
	}

	struct sockaddr_in saddr;
	memset(&saddr, 0, sizeof(struct sockaddr_in));
	saddr.sin_addr.s_addr = INADDR_ANY;
	saddr.sin_family = AF_INET;
	saddr.sin_port = ntohs(6666);
	int res = bind(dsock, &saddr, sizeof(saddr));
	PPRINTF("Bind result: %d\n", res);
	if (res < 0) {
		return res;
	}
	return dsock;
}

/**
 * Creates the multicast socket that will be used for echonet lite purposes
 */
int createMulticastSocket() {
	//create socket
	int msock = socket(AF_INET, SOCK_DGRAM, 0);
	PPRINTF("socket FD: %d\n", msock);

	if (msock < 0) {
		return msock;
	}
	//get local if address
	struct ip_info pinfo;
	int res = sdk_wifi_get_ip_info(0, &pinfo);
	PPRINTF("get_ip_info result: %d\n", res);
	PPRINTF("ip address: %s\n", ip_ntoa(&pinfo.ip));
	PPRINTF("gateway: %s\n", ip_ntoa(&pinfo.gw));
	int optlen = 8;

	struct ip_addr dummy;
	IP4_ADDR(&dummy, 224, 0, 23, 0);

	/*********BIND ********/
	struct sockaddr_in lsaddr;
	memset(&lsaddr, 0, sizeof(lsaddr));
	lsaddr.sin_addr.s_addr = pinfo.ip.addr;
	lsaddr.sin_family = AF_INET;
	lsaddr.sin_port = ntohs(ELITE_PORT);
	res = bind(msock, &lsaddr, sizeof(lsaddr));
	PPRINTF("Bind result: %d\n");

	/*********MULTICAST*************/
	struct ip_mreq ipmreq;
	ipmreq.imr_multiaddr.s_addr = dummy.addr;
	ipmreq.imr_interface.s_addr = pinfo.ip.addr;

	res = setsockopt(msock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &ipmreq,
			sizeof(ipmreq));
	PPRINTF("multicast setsockopt result: %d\n", res);

	return msock;
}

// i don't remember what this is...
//void startReceivingLoop(ECHOCTRL_PTR ectrl) {
//	xTaskCreate(receiveLoop, (signed char * ) "eliteReceiveLoop", 256, ectrl, 1,
//			NULL);
//}

/**
 * This is an example of how to set up objects and properties. Make sure to
 * imitate this for your object initialization needs. General steps:
 * 1: create a node profile object
 * 2: create a basic object
 * 3: add properties to the object of stage 2
 * 4: compute property maps for both objects
 * 5: register the objects with the control context.
 * 6: calculate the node clas instance lists at the very end.
 *
 * @param ectrl the echonet lite control context
 * @param adapter the middleware adapter control struct
 */
void setupObjects(ECHOCTRL_PTR ectrl, MADAPTER_PTR adapter) {
	OBJ_PTR profile = createNodeProfileObject();
    //Emergency button object creation
	OBJ_PTR eobject = createBasicObject("\x00\x03\x01");

    //the properties have no functionality implemented
	addProperty(eobject,
			createDataProperty(0xB1, E_READ | E_WRITE | E_NOTIFY, 0x01, 0x01,
					"A"));
	addProperty(eobject,
			createDataProperty(0xBF, E_READ | E_WRITE , 0x01, 0x01,
					"A"));

	computePropertyMaps(eobject);
	computePropertyMaps(profile);
	addObject(ectrl, profile);
	addObject(ectrl, eobject);
	computeNodeClassInstanceLists(ectrl->oHead);
}

void netDebugTask(void);
/**
 * This function sets up UART0 appropriately, see the implementation for details
 */
void setupUART(void);

/**
 * entry point for the main task. Activities happening here:
 * 1: setup the wireless network
 * 2: create the debug task (comment this out here if you need, but also
 * dont forget #DOWEBLOG
 * 3: (optional) run tests
 * 4: initialize the multicast socket.
 * 5: setup the middleware adapter (tricky, initialize all members appropriately)
 * 6: start adapter receiver task
 * 7: setup all objects.
 * 8: perform startup notifications
 * 9: enter main receiver loop.
 *
 * Each step must happen for things to work correctly. However, the order of
 * some of the steps can be slightly changed. Pay attention to any field
 * initialization that didn't get handled internaly neatly in some function,
 * because you will have to reproduce it in your code.
 *
 * \todo refactor this?
 */
void eliteTask(void) {

	printf("wireless start\n");
	setupWirelessIF();
	printf("wireless end\n");
	printf("Sleeping for 5...\n");

	xTaskCreate(netDebugTask, (signed char *) "netDebugTask", 256, NULL, 4,
	NULL);

	vTaskDelay(5000 / portTICK_PERIOD_MS);
	PPRINTF("sdk version:%s\n", sdk_system_get_sdk_version());

//	xTaskCreate(runTestsTask, (signed char * )"runTestsTask", 512, NULL, 1,
//			NULL);

	int msock = createMulticastSocket();

	//setup our control struct.
	PPRINTF("Echonet startup.");
	ECHOCTRL_PTR ectrl = createEchonetControl();
	ectrl->msock = msock;

	//setup middlewareAdapter
	PPRINTF("madapter init");
	setupUART();
	MADAPTER_PTR adapter = createMiddlewareAdapter(stdin, stdout);
	setContext(adapter, ectrl);
	PPRINTF("madapter set context");

	PPRINTF("receiver task");
	startReceiverTask(adapter);

	setupObjects(ectrl, adapter);
	Property_PTR wakeupcall = getProperty(getObject(ectrl->oHead, PROFILEEOJ),
			0xD5);
	for (int i = 0; i < 3; i++) {
		makeNotification(wakeupcall);
	}
	receiveLoop(ectrl);
}

/**
 * the net debug task that performs the actual web logging. Uses semaphores
 * to coordinate with other tasks that call #WEBLOG. two semaphores are
 * necessary at least. Have a look at #WEBLOG to understand the interaction
 * through semaphores.
 */
void netDebugTask(void) {

	int sock = createDebugSocket();
	if (sock < 0) {
		PPRINTF("netDebugTask: failed to setup debug port. Exit.\n");
		return;
	}
	int res = listen(sock, 1);
	if (res < 0) {
		PPRINTF("netDebugTask: listen failed. Exit.\n");
		return;
	}

	//time to setup our semaphores
	vSemaphoreCreateBinary(debugdowrite);
	vSemaphoreCreateBinary(debugsem);
	if (debugsem == NULL) {
		PPRINTF("debug mutex creation fail\n");
	} else {
		PPRINTF("debug mutex creation success\n");
	}

	struct sockaddr_in clientaddr;
	socklen_t size = sizeof(clientaddr);
	memset(&clientaddr, 0, size);
	static char greeting[] = "Hello, hope this helps!\n";
	do {
		int clientfd = accept(sock, &clientaddr, &size);
		write(clientfd, greeting, sizeof(greeting));

		int writeres = 0;
		while (writeres >= 0) {
			//sleep forever
			xSemaphoreTake(debugsem, portMAX_DELAY);
			writeres = write(clientfd, ndbuf, ndsize);
		}
		close(clientfd);
	} while (1);

}

/*
 * a periodic task. Strictly speaking unnecessary but provides a sense of time
 * disable this for release
 */
void testPeriodic(void) {
	int i = 0;
	while (1) {
		vTaskDelay(300 / portTICK_PERIOD_MS);
		PPRINTF("looping... %d\n", ++i);

	}
}

void setupUART() {
	//the conf0 register
	uint32_t conf = UART(0).CONF0;
	//"or" the desired bits
	UART(0).CONF0 = conf | UART_CONF0_PARITY_ENABLE //enable parity
			| (UART_CONF0_BYTE_LEN_M << UART_CONF0_BYTE_LEN_S) //eight bits
			| UART_CONF0_STOP_BITS_S; //parity even
	uart_clear_rxfifo(0); //clear the receiving fifo while we:'re at it
}

//static char freadbuf[512];
/**
 * entry point for our code. Setup baudrate
 * init the semaphore pointers and startup tasks.
 */
void user_init(void) {
	uart_set_baud(0, 9600);
	//gdbstub_init();

	debugsem = NULL;
	debugdowrite = NULL;

//	printf("portTICK_PERIOD_MS: %d\n", portTICK_PERIOD_MS);
//	printf("gief input\n");
//	static char test[16];
//	int res = setvbuf(stdin, freadbuf, _IONBF, 512);
//	for (int i = 0; i < 15; i++) {
//	char get = getc(stdin);
//	printf("char get: %c\n", get);
//	}
//	fwrite("test\n", 1, 5, stdout);
	//printf("fread test\n");
	//this crashes
	//fread(test, 1, 15, stdin);
	//fread_unlocked(test, 1, 15, stdin);
	//printf("test input: %s\n", test);

//main task
	xTaskCreate(eliteTask, (signed char *) "eliteTask", 1024, NULL, 1, NULL);
//periodic looping task, comment this out if you don't need it.
	xTaskCreate(testPeriodic, (signed char *) "testPeriodic", 256, NULL, 1,
	NULL);

}

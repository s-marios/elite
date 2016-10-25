/* The classic "blink" example
 *
 * This sample code is in the public domain.
 */
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include "espressif/esp_common.h"
#include "esp/uart.h"
#include "FreeRTOS.h"
#include "task.h"
#include "esp8266.h"
#include "esp/gpio.h"
#include "elite.h"
#include "minunit.h"
#include "macrolist.h"
//#include "gdbstub.h"

int tests_run = 0;

static char * test_PRINTF() {
#ifdef ELITE_DEBUG
	mu_assert("PRINTF: did not print", PRINTF("hello!") == 6);
#else
	PRINTF("hello!");
#endif
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
	ECHOFRAME_PTR frame = initFrame(ectrl, ECHOFRAME_STDSIZE, 0);
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

	ECHOFRAME_PTR frame = initFrame(ectrl, ECHOFRAME_STDSIZE, 0);
	mu_assert("frame init failed", frame != NULL);
	mu_assert("frame header invalid byte 1", frame->data[0] == E_HD1);
	mu_assert("frame header invalid byte 1", frame->data[1] == E_HD2);
	printf("getShort result: %d \n", getShort(frame, 2));
	printf("first four bytes: %x %x %x %x\n", frame->data[0], frame->data[1],
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
	ECHOFRAME_PTR frame = initFrame(ectrl, 0, 0);

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
	ECHOFRAME_PTR frame = initFrame(ectrl, 0, 0);
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
	//printf("parse shortest frame result: %d opc:%d\n", result, frame->propNum);
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
	printf("frame size: %d\n", frame->used);
	mu_assert("parseFrame: created frame lenght not 31", frame->used == 31);
	mu_assert("parseFrame: created frame parse not OK",
			parseFrame(frame) == PR_OK);
	free(ectrl);
	free(frame);
	return 0;
}

static char * test_getNextEPC() {

	ECHOCTRL_PTR ectrl = createEchonetControl();
	ECHOFRAME_PTR frame = initFrame(ectrl, 0, 0);
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
		printf("getNextEPC repetition: %d\n", i);
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
	LAPPEND(object_p, p1);
	PRINTF("object_p->next is: %s\n", object_p ? "true" : "false");
	mu_assert("next field not adjusted properly", object_p->next == p1);
	mu_assert("p1->next is not null", p1->next == NULL);
	mu_assert("p1 i is not 1 (1)", p1->i == 1);
	LAPPEND(object_p, p2);
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
	LPREPEND(object_p, p4);
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
			property->read == testRead);
	mu_assert("testProperty: write is not NULL", property->write == NULL);
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
		LAPPEND(props[0], props[i]);
		PRINTF("property created: %d\n", i);
		mu_assert("property creation failed", props[i]);
		mu_assert("property code does not match",
				props[i]->propcode == (mappropcode - 1));

	}
	mu_assert("i index not 4 (1st)", i == 4);

	i = 0;
	FOREACH(props[0], Property_PTR)
	{
		i++;
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
		if (prev) {
			LAPPEND(prev, props[i]);
		}
		prev = props[i];
	}
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
	freeProperty(toFind);
	return 0;
}

static char * test_propertyMaps() {
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
	mu_run_test(test_propertyMaps);
	return 0;
}

void runTestsTask(void * params) {
	void * malloced = malloc(1);
	printf("is malloced? : %s ", malloced ? "true" : "false");
	printf("Sizeof ECHOFRAME: %d\n", sizeof(ECHOFRAME));
	printf("running tests... \n");
	char * result = allTests();
	if (result != 0) {
		printf("%s\n", result);
	} else {
		printf("ALL TESTS PASSED.");
	}
	while (1) {
		vTaskDelay(1000 / portTICK_RATE_MS);
		printf(".");
	}
}

void user_init(void) {
	uart_set_baud(0, 115200);
	//gdbstub_init();
	xTaskCreate(runTestsTask, (signed char * )"runTestsTask", 256, NULL, 2,
			NULL);
}

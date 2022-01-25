#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <string.h>
#include <cmocka.h>

#include "../elite_priv.h"
#include "../elite.h"

static void null_test_success(void **state) {
	(void) state;
}

static void test_data_property2(void **state) {
	Property_PTR prop = createDataProperty2(
	                        0x80U, E_READ | E_WRITE, STORAGE_UPTO, 10, 4, "test");
	assert_non_null(prop);
	assert_ptr_equal(prop->readf, readData);
	assert_ptr_equal(prop->writef, writeData);
	assert_int_equal(prop->rwn_mode, E_READ | E_WRITE);
	assert_non_null(prop->opt);

	//initial read is test
	unsigned char read_buff[10];
	int res = readProperty(prop, 10, read_buff);
	assert_int_equal(res, 4);
	int memcmp_res = memcmp(read_buff, "test", res);
	assert_int_equal(memcmp_res, 0);

	const unsigned char * write_data = (const unsigned char *) "0123456789toolong";
	//write test
	res = writeProperty(prop, 10, write_data);
	assert_int_equal(res, 10);
	res = readProperty(prop, 10, read_buff);
	assert_int_equal(res, 10);
	memcmp_res = memcmp(read_buff, write_data, res);
	assert_int_equal(memcmp_res, 0);

	//write something too long
	res = writeProperty(prop, 15, write_data);
	//failed max size check returns 0
	assert_int_equal(res, 0);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(null_test_success),
		cmocka_unit_test(test_data_property2),
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}

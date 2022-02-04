#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <string.h>
#include <cmocka.h>

#include "../elite_priv.h"
#include "../elite.h"

/***** testing dataProperty2 *****/
static void test_data_property2_basic_setup_is_ok(void **state)
{
	Property_PTR prop = createDataProperty2(
	        0x80U, E_READ | E_WRITE, STORAGE_UPTO, 10, 4, "test");
	assert_non_null(prop);
	assert_ptr_equal(prop->readf, readData);
	assert_ptr_equal(prop->writef, writeData);
	assert_int_equal(prop->rwn_mode, E_READ | E_WRITE);
	assert_non_null(prop->opt);
}

static int setup_dataProperty2(void ** state)
{
	Property_PTR prop = createDataProperty2(
	        0x80U, E_READ | E_WRITE, STORAGE_UPTO, 10, 4, "test");
	*state = prop;
	return 0;
}

static void test_dataProperty2_state_usage(void **state)
{
	assert_non_null(*state);
	Property_PTR prop = *state;
	assert_int_equal(prop->propcode, 0x80U);
}

static void test_initial_read_data_property2(void ** state)
{
	Property_PTR prop = createDataProperty2(
	        0x80U, E_READ | E_WRITE, STORAGE_UPTO, 10, 4, "test");
	//initial read is test
	unsigned char read_buff[10];
	int res = readProperty(prop, 10, read_buff);
	assert_int_equal(res, 4);
	int memcmp_res = memcmp(read_buff, "test", res);
	assert_int_equal(memcmp_res, 0);
}

static void test_write_10_bytes_is_ok(void ** state)
{
	Property_PTR prop = createDataProperty2(
	        0x80U, E_READ | E_WRITE, STORAGE_UPTO, 10, 4, "test");
	const unsigned char * write_data = (const unsigned char *) "0123456789toolong";
	unsigned char read_buff[10];
	//write test
	int res = writeProperty(prop, 10, write_data);
	assert_int_equal(res, 10);
	res = readProperty(prop, 10, read_buff);
	assert_int_equal(res, 10);
	int memcmp_res = memcmp(read_buff, write_data, res);
	assert_int_equal(memcmp_res, 0);
}

static void test_write_11_bytes_fails( void ** state)
{
	Property_PTR prop = createDataProperty2(
	        0x80U, E_READ | E_WRITE, STORAGE_UPTO, 10, 4, "test");
	const unsigned char * write_data = (const unsigned char *) "0123456789toolong";
	//the property only stores 10 bytes, so a write of 11 should fail
	int res = writeProperty(prop, 11, write_data);
	assert_int_equal(res, 0);
}


int main(void)
{
	const struct CMUnitTest dataProperty2_tests[] = {
		cmocka_unit_test_setup(test_dataProperty2_state_usage, setup_dataProperty2),
		cmocka_unit_test(test_data_property2_basic_setup_is_ok),
		cmocka_unit_test(test_initial_read_data_property2),
		cmocka_unit_test(test_write_10_bytes_is_ok),
		cmocka_unit_test(test_write_11_bytes_fails),
	};

	char * group_name = "dataProperty2";
	printf("\nGroup Test: %s\n", group_name);
	cmocka_run_group_tests_name(group_name, dataProperty2_tests, NULL, NULL);

	return 0;
}

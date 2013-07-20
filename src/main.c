/**
 * main.c
 * Tests for 'struct' module
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013 Mozzhuhin Andrey
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "struct.h"
#include <stdint.h>
#include <limits.h>
#include <float.h>
#include <string.h>
#include <stdio.h>

struct TestStructBasic
{
	uint8_t x;
	char c;
	int8_t b;
	uint8_t B;
	uint8_t qm;
	int16_t h;
	uint16_t H;
	int32_t i;
	uint32_t I;
	int32_t l;
	uint32_t L;
	int64_t q;
	uint64_t Q;
	float f;
	double d;
	char s;
} __attribute__((__packed__));

static void test_struct_pack_basic_min(void)
{
	uint8_t buf[100];
	ssize_t size;
	struct TestStructBasic min = {
			.x = 0,
			.c = CHAR_MIN,
			.b = SCHAR_MIN,
			.B = 0,
			.qm = 0,
			.h = SHRT_MIN,
			.H = 0,
			.i = INT_MIN,
			.I = 0,
			.l = LONG_MIN,
			.L = 0,
			.q = LLONG_MIN,
			.Q = 0,
			.f = FLT_MIN,
			.d = DBL_MIN,
			.s = CHAR_MIN
	};

	size = struct_pack(buf, sizeof(buf), "xcbB?hHiIlLqQfds",
			min.c, min.b, min.B, min.qm, min.h, min.H, min.i, min.I,
			min.l, min.L, min.q, min.Q, min.f, min.d, &min.s);

	printf("Pack basic with minimal values: ");
	if (size == sizeof(min) && memcmp(buf, &min, sizeof(min)) == 0)
		printf("PASS\n");
	else
		printf("FAIL\n");
}

static void test_struct_pack_basic_max(void)
{
	uint8_t buf[100];
	ssize_t size;
	struct TestStructBasic max = {
			.x = 0,
			.c = CHAR_MAX,
			.b = SCHAR_MAX,
			.B = UCHAR_MAX,
			.qm = 0,
			.h = SHRT_MAX,
			.H = USHRT_MAX,
			.i = INT_MAX,
			.I = UINT_MAX,
			.l = LONG_MAX,
			.L = ULONG_MAX,
			.q = LLONG_MAX,
			.Q = ULLONG_MAX,
			.f = FLT_MAX,
			.d = DBL_MAX,
			.s = CHAR_MAX
	};

	size = struct_pack(buf, sizeof(buf), "xcbB?hHiIlLqQfds",
			max.c, max.b, max.B, max.qm, max.h, max.H, max.i, max.I,
			max.l, max.L, max.q, max.Q, max.f, max.d, &max.s);

	printf("Pack basic with maximal values: ");
	if (size == sizeof(max) && memcmp(buf, &max, sizeof(max)) == 0)
		printf("PASS\n");
	else
		printf("FAIL\n");
}

static void test_struct_pack_repeat(void)
{
	uint8_t buf[100];
	ssize_t size;
	int32_t arr_i[5] = { 2013, 3, 21, 0, 34 };
	char arr_c[6] = "struct";

	size = struct_pack(buf, sizeof(buf), "5i 6c",
			arr_i[0], arr_i[1], arr_i[2], arr_i[3], arr_i[4],
			arr_c[0], arr_c[1], arr_c[2], arr_c[3], arr_c[4], arr_c[5]);

	printf("Pack repeated values test: ");
	if (size == sizeof(arr_i) + sizeof(arr_c) &&
			memcmp(&buf[0], arr_i, sizeof(arr_i)) == 0 &&
			memcmp(&buf[sizeof(arr_i)], arr_c, sizeof(arr_c)) == 0)
		printf("PASS\n");
	else
		printf("FAIL\n");
}

static void test_struct_unpack_basic_min(void)
{
	ssize_t size;
	struct TestStructBasic value = {
			.x = 0,
			.c = CHAR_MIN,
			.b = SCHAR_MIN,
			.B = 0,
			.qm = 0,
			.h = SHRT_MIN,
			.H = 0,
			.i = INT_MIN,
			.I = 0,
			.l = LONG_MIN,
			.L = 0,
			.q = LLONG_MIN,
			.Q = 0,
			.f = FLT_MIN,
			.d = DBL_MIN,
			.s = CHAR_MIN
	};
	struct TestStructBasic result;

	size = struct_unpack(&value, sizeof(value), "xcbB?hHiIlLqQfds",
			&result.c, &result.b, &result.B, &result.qm, &result.h, &result.H, &result.i, &result.I,
			&result.l, &result.L, &result.q, &result.Q, &result.f, &result.d, &result.s, sizeof(result.s));
	result.x = 0; result.s = CHAR_MIN; // fields not unpacked

	printf("Unpack basic with minimal values: ");
	if (size == sizeof(value) &&
			memcmp(&result, &value, sizeof(value)) == 0)
		printf("PASS\n");
	else
		printf("FAIL\n");
}

static void test_struct_unpack_repeat(void)
{
	uint8_t buf[100];
	ssize_t size;
	int arr_value[3] = { -1, 100500, 42 };
	int arr_result[3];

	size = struct_unpack(&arr_value, sizeof(arr_value), "3i",
			&arr_result[0], &arr_result[1], &arr_result[2]);

	printf("Unpack repeated values test: ");
	if (size == sizeof(arr_value) &&
			memcmp(&arr_value, arr_result, sizeof(arr_result)) == 0)
		printf("PASS\n");
	else
		printf("FAIL\n");
}

static void test_struct_example_1_1(void)
{
	uint8_t buf[100];
	ssize_t size;
#if __BYTE_ORDER == __LITTLE_ENDIAN
	uint8_t result[] = { 0x01, 0x00, 0x02, 0x00, 0x03, 0x00, 0x00, 0x00 };
#else
	uint8_t result[] = { 0x00, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00, 0x03 };
#endif

	size = struct_pack(buf, sizeof(buf), "hhl", 1, 2, 3);

	printf("Example 1.1 test: ");
	if (size == sizeof(result) &&
			memcmp(buf, result, sizeof(result)) == 0)
		printf("PASS\n");
	else
		printf("FAIL\n");
}

static void test_struct_example_1_2(void)
{
	short a, b;
	long c;
	ssize_t size;
#if __BYTE_ORDER == __LITTLE_ENDIAN
	uint8_t buf[] = { 0x01, 0x00, 0x02, 0x00, 0x03, 0x00, 0x00, 0x00 };
#else
	uint8_t buf[] = { 0x00, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00, 0x03 };
#endif

	size = struct_unpack(buf, sizeof(buf), "hhl", &a, &b, &c);

	printf("Example 1.2 test: ");
	if (size == sizeof(buf) &&
			(a == 1) && (b == 2) && (c == 3))
		printf("PASS\n");
	else
		printf("FAIL\n");
}

static void test_struct_example_1_3(void)
{
	ssize_t size;

	size = struct_calcsize("hhl");

	printf("Example 1.3 test: ");
	if (size == 8)
		printf("PASS\n");
	else
		printf("FAIL\n");
}

int main(int argc, char *argv[])
{
	test_struct_pack_basic_min();
	test_struct_pack_basic_max();
	test_struct_pack_repeat();

	test_struct_unpack_basic_min();
	test_struct_unpack_repeat();

	test_struct_example_1_1();
	test_struct_example_1_2();
	test_struct_example_1_3();

	return 0;
}

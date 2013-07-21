/**
 * struct.c
 * Structure packing like python 'struct' module
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
#include <ctype.h>
#include <endian.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>

//
// Private Types
//

/** Context of pack/unpack services */
typedef struct _struct_context
{
	int byte_order;
	int native_alignment;
	int native_size;
	size_t repeat;
} struct_context;

/**
 * Definition of packer function for basic type
 * @param buffer Buffer for store value, must be not NULL and have enough space
 */
typedef ssize_t (*struct_pack_basic)(void *buffer, struct_context *context, va_list *vl);

/** Definition of unpacker function for basic type */
typedef ssize_t (*struct_unpack_basic)(const void *buffer, struct_context *context, va_list *vl);

/** Definition of function for basic type size calculation */
typedef ssize_t (*struct_calcsize_basic)(struct_context *context);

typedef struct _struct_format_field
{
	char format;
	struct_pack_basic pack;
	struct_unpack_basic unpack;
	struct_calcsize_basic calcsize;
} struct_format_field;

//
// Forward Declarations
//

static ssize_t struct_pack_pad(void *buffer, struct_context *context, va_list *vl);
static ssize_t struct_unpack_pad(const void *buffer, struct_context *context, va_list *vl);
static ssize_t struct_calcsize_pad(struct_context *context);

static ssize_t struct_pack_byte(void *buffer, struct_context *context, va_list *vl);
static ssize_t struct_unpack_byte(const void *buffer, struct_context *context, va_list *vl);
static ssize_t struct_calcsize_byte(struct_context *context);

static ssize_t struct_pack_bool(void *buffer, struct_context *context, va_list *vl);
static ssize_t struct_unpack_bool(const void *buffer, struct_context *context, va_list *vl);
static ssize_t struct_calcsize_bool(struct_context *context);

static ssize_t struct_pack_short(void *buffer, struct_context *context, va_list *vl);
static ssize_t struct_unpack_short(const void *buffer, struct_context *context, va_list *vl);
static ssize_t struct_calcsize_short(struct_context *context);

static ssize_t struct_pack_int(void *buffer, struct_context *context, va_list *vl);
static ssize_t struct_unpack_int(const void *buffer, struct_context *context, va_list *vl);
static ssize_t struct_calcsize_int(struct_context *context);

static ssize_t struct_pack_quad(void *buffer, struct_context *context, va_list *vl);
static ssize_t struct_unpack_quad(const void *buffer, struct_context *context, va_list *vl);
static ssize_t struct_calcsize_quad(struct_context *context);

static ssize_t struct_pack_float(void *buffer, struct_context *context, va_list *vl);
static ssize_t struct_unpack_float(const void *buffer, struct_context *context, va_list *vl);
static ssize_t struct_calcsize_float(struct_context *context);

static ssize_t struct_pack_double(void *buffer, struct_context *context, va_list *vl);
static ssize_t struct_unpack_double(const void *buffer, struct_context *context, va_list *vl);
static ssize_t struct_calcsize_double(struct_context *context);

static ssize_t struct_pack_str(void *buffer, struct_context *context, va_list *vl);
static ssize_t struct_unpack_str(const void *buffer, struct_context *context, va_list *vl);
static ssize_t struct_calcsize_str(struct_context *context);

//
// Private Variables
//

/** Format definition table */
static const struct_format_field struct_format_fields[] = {
		{ 'x', struct_pack_pad, struct_unpack_pad, struct_calcsize_pad },
		{ 'c', struct_pack_byte, struct_unpack_byte, struct_calcsize_byte },
		{ 'b', struct_pack_byte, struct_unpack_byte, struct_calcsize_byte },
		{ 'B', struct_pack_byte, struct_unpack_byte, struct_calcsize_byte },
		{ '?', struct_pack_bool, struct_unpack_bool, struct_calcsize_bool },
		{ 'h', struct_pack_short, struct_unpack_short, struct_calcsize_short },
		{ 'H', struct_pack_short, struct_unpack_short, struct_calcsize_short },
		{ 'i', struct_pack_int, struct_unpack_int, struct_calcsize_int },
		{ 'I', struct_pack_int, struct_unpack_int, struct_calcsize_int },
		{ 'l', struct_pack_int, struct_unpack_int, struct_calcsize_int },
		{ 'L', struct_pack_int, struct_unpack_int, struct_calcsize_int },
		{ 'q', struct_pack_quad, struct_unpack_quad, struct_calcsize_quad },
		{ 'Q', struct_pack_quad, struct_unpack_quad, struct_calcsize_quad },
		{ 'f', struct_pack_float, struct_unpack_float, struct_calcsize_float },
		{ 'd', struct_pack_double, struct_unpack_double, struct_calcsize_double },
		{ 's', struct_pack_str, struct_unpack_str, struct_calcsize_str },
		/* end of format table */
		{ '\0', NULL, NULL, NULL }
};

//
// Private Services
//

static ssize_t struct_pack_pad(void *buffer, struct_context *context, va_list *vl)
{
	memset(buffer, 0, context->repeat);
	return context->repeat * sizeof(uint8_t);
}

static ssize_t struct_unpack_pad(const void *buffer, struct_context *context, va_list *vl)
{
	return context->repeat * sizeof(uint8_t);
}

static ssize_t struct_calcsize_pad(struct_context *context)
{
	return context->repeat * sizeof(uint8_t);
}

static ssize_t struct_pack_byte(void *buffer, struct_context *context, va_list *vl)
{
	uint8_t *p = buffer;
	size_t i;

	for (i = 0; i < context->repeat; i++)
		*p++ = va_arg(*vl, int);

	return context->repeat * sizeof(uint8_t);
}

static ssize_t struct_unpack_byte(const void *buffer, struct_context *context, va_list *vl)
{
	const int8_t *p = buffer;
	size_t i;

	for (i = 0; i < context->repeat; i++)
		*va_arg(*vl, int8_t*) = *p++;

	return context->repeat * sizeof(int8_t);
}

static ssize_t struct_calcsize_byte(struct_context *context)
{
	return context->repeat * sizeof(int8_t);
}

static ssize_t struct_pack_bool(void *buffer, struct_context *context, va_list *vl)
{
	int8_t *p = buffer;
	size_t i;

	for (i = 0; i < context->repeat; i++)
		*p++ = va_arg(*vl, int) != 0;

	return context->repeat * sizeof(int8_t);
}

static ssize_t struct_unpack_bool(const void *buffer, struct_context *context, va_list *vl)
{
	const int8_t *p = buffer;
	size_t i;

	for (i = 0; i < context->repeat; i++)
		*va_arg(*vl, int8_t*) = (*p++ != 0);

	return context->repeat * sizeof(int8_t);
}

static ssize_t struct_calcsize_bool(struct_context *context)
{
	return context->repeat * sizeof(uint8_t);
}

static ssize_t struct_pack_short(void *buffer, struct_context *context, va_list *vl)
{
	int16_t *p = buffer;
	size_t i;

	for (i = 0; i < context->repeat; i++)
		*p++ = va_arg(*vl, int);

	return context->repeat * sizeof(int16_t);
}

static ssize_t struct_unpack_short(const void *buffer, struct_context *context, va_list *vl)
{
	const int16_t *p = buffer;
	size_t i;

	for (i = 0; i < context->repeat; i++)
		*va_arg(*vl, int16_t*) = *p++;

	return context->repeat * sizeof(int16_t);
}

static ssize_t struct_calcsize_short(struct_context *context)
{
	return context->repeat * sizeof(int16_t);
}

static ssize_t struct_pack_int(void *buffer, struct_context *context, va_list *vl)
{
	int32_t *p = buffer;
	size_t i;

	for (i = 0; i < context->repeat; i++)
		*p++ = va_arg(*vl, int32_t);

	return context->repeat * sizeof(int32_t);
}

static ssize_t struct_unpack_int(const void *buffer, struct_context *context, va_list *vl)
{
	const int32_t *p = buffer;
	size_t i;

	for (i = 0; i < context->repeat; i++)
		*va_arg(*vl, int32_t*) = *p++;

	return context->repeat * sizeof(int32_t);
}

static ssize_t struct_calcsize_int(struct_context *context)
{
	return context->repeat * sizeof(int32_t);
}

static ssize_t struct_pack_quad(void *buffer, struct_context *context, va_list *vl)
{
	int64_t *p = buffer;
	size_t i;

	for (i = 0; i < context->repeat; i++)
		*p++ = va_arg(*vl, int64_t);

	return context->repeat * sizeof(int64_t);
}

static ssize_t struct_unpack_quad(const void *buffer, struct_context *context, va_list *vl)
{
	const int64_t *p = buffer;
	size_t i;

	for (i = 0; i < context->repeat; i++)
		*va_arg(*vl, int64_t*) = *p++;

	return context->repeat * sizeof(int64_t);
}

static ssize_t struct_calcsize_quad(struct_context *context)
{
	return context->repeat * sizeof(int64_t);
}

static ssize_t struct_pack_float(void *buffer, struct_context *context, va_list *vl)
{
	float *p = buffer;
	size_t i;

	for (i = 0; i < context->repeat; i++)
		*p++ = va_arg(*vl, double);

	return context->repeat * sizeof(float);
}

static ssize_t struct_unpack_float(const void *buffer, struct_context *context, va_list *vl)
{
	const float *p = buffer;
	size_t i;

	for (i = 0; i < context->repeat; i++)
		*va_arg(*vl, float*) = *p++;

	return context->repeat * sizeof(float);
}

static ssize_t struct_calcsize_float(struct_context *context)
{
	return context->repeat * sizeof(float);
}

static ssize_t struct_pack_double(void *buffer, struct_context *context, va_list *vl)
{
	double *p = buffer;
	size_t i;

	for (i = 0; i < context->repeat; i++)
		*p++ = va_arg(*vl, double);

	return context->repeat * sizeof(double);
}

static ssize_t struct_unpack_double(const void *buffer, struct_context *context, va_list *vl)
{
	const double *p = buffer;
	size_t i;

	for (i = 0; i < context->repeat; i++)
		*va_arg(*vl, double*) = *p++;

	return context->repeat * sizeof(double);
}

static ssize_t struct_calcsize_double(struct_context *context)
{
	return context->repeat * sizeof(double);
}

static ssize_t struct_pack_str(void *buffer, struct_context *context, va_list *vl)
{
	char *p = buffer;
	const char *str = va_arg(*vl, const char *);
	size_t i;

	for (i = 0; i < context->repeat; i++)
	{
		if (str != NULL && *str != '0')
			*p++ = *str++;
		else
			*p++ = 0;
	}

	return context->repeat * sizeof(char);
}

static ssize_t struct_unpack_str(const void *buffer, struct_context *context, va_list *vl)
{
	char *str = va_arg(*vl, char *);
	size_t str_size = va_arg(*vl, size_t);

	strncpy(str, buffer, context->repeat < str_size ? context->repeat : str_size);
	str[str_size - 1] = '\0';

	return context->repeat * sizeof(char);
}

static ssize_t struct_calcsize_str(struct_context *context)
{
	return context->repeat * sizeof(char);
}

/**
 * Parse next field format
 * @param format Format string
 * @param context Struct parsing context
 * @param item Returning pointer to item or NULL on error
 * @return Pointer to next character for parsing in format string
 */
const char* struct_parse_field(const char *format, struct_context *context, const struct_format_field **field)
{
	const char *c = format;

	// whitespace characters between formats are ignored
	while (isspace(*c)) c++;

	// read integral repeat count
	if (isdigit(*c))
	{
		context->repeat = 0;
		do {
			context->repeat = context->repeat * 10 + *c - '0';
			c++;
		} while (isdigit(*c));
	}
	else
	{
		context->repeat = 1;
	}
	// find packer for current basic type
	*field = &struct_format_fields[0];
	while ((*field)->format != '\0' && (*field)->format != *c)
		(*field)++;
	if ((*field)->format == *c)
		c++;
	else
		*field = NULL;

	return c;
}

//
// Public Services
//

ssize_t struct_pack(void *buffer, size_t size, const char *format, ...)
{
	const char *c;
	struct_context context;
	const struct_format_field *field;
	uint8_t *p;
	va_list vl;

	if (buffer == NULL || format == NULL)
		return -1;

	context.byte_order = __BYTE_ORDER;
	context.native_alignment = 1;
	context.native_size = 1;

	c = format;
	p = buffer;
	va_start(vl, format);

	while ((*c != '\0') && (p < (uint8_t *) buffer + size))
	{
		c = struct_parse_field(c, &context, &field);
		if (field != NULL && (uint8_t *) buffer + size - p >= field->calcsize(&context))
			p += field->pack(p, &context, &vl);
		else
			break;
	}

	va_end(vl);

	// format string not processed
	if (*c != '\0')
		return -1;

	return p - (uint8_t *) buffer;
}

ssize_t struct_unpack(const void *buffer, size_t size, const char *format, ...)
{
	const char *c;
	struct_context context;
	const struct_format_field *field;
	const uint8_t *p;
	va_list vl;

	if (buffer == NULL || format == NULL)
		return -1;

	context.byte_order = __BYTE_ORDER;
	context.native_alignment = 1;
	context.native_size = 1;

	c = format;
	p = buffer;
	va_start(vl, format);

	while ((*c != '\0') && (p < (uint8_t *) buffer + size))
	{
		c = struct_parse_field(c, &context, &field);
		if (field != NULL && (const uint8_t *) buffer + size - p >= field->calcsize(&context))
			p += field->unpack(p, &context, &vl);
		else
			break;
	}

	va_end(vl);

	// not processed all format string
	if (*c != '\0')
		return -1;

	return p - (uint8_t *) buffer;
}

ssize_t struct_calcsize(const char *format)
{
	const char *c;
	struct_context context;
	const struct_format_field *field;
	ssize_t result;

	if (format == NULL)
		return -1;

	context.byte_order = __BYTE_ORDER;
	context.native_alignment = 1;
	context.native_size = 1;

	c = format;
	result = 0;

	while (*c != '\0')
	{
		c = struct_parse_field(c, &context, &field);
		if (field != NULL)
			result += field->calcsize(&context);
		else
			break;
	}

	// format string not processed
	if (*c != '\0')
		return -1;

	return result;
}

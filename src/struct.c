/**
 * struct.c
 * Structure packing like python 'struct' module.
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
// Private Definitions
//

/**
 * Calculate count of padding bytes needed to align field with given type
 */
#define struct_field_padding(context, type)	(context->native_alignment ? \
		(__alignof__(type) - (uintptr_t) context->offset % __alignof__(type)) % __alignof__(type) : 0)

//
// Private Types
//

/** Context of pack/unpack services */
typedef struct _struct_context
{
	int byte_order;
	int native_alignment;
	int native_size;
	size_t offset;
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

/**
 * Float to 32-bit integer value conversation
 */
typedef union _float32
{
	float		f;
	uint32_t	i;
} float32;

/**
 * Double to 64-bit integer value conversation
 */
typedef union _double64
{
	double		d;
	uint64_t	i;
} double64;

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

/**
 * Load 16-bit value in system order from pointer
 * @param p Data source
 * @return 16-bit value from pointer
 */
static inline uint16_t load_16(const void *p)
{
#if BYTE_ORDER == LITTLE_ENDIAN
	return (*(uint8_t *) p) + (*(uint8_t *) (p + 1) << 8);
#else
	return (*(uint8_t *) (p + 1)) + (*(uint8_t *) p << 8);
#endif
}

/**
 * Store 16-bit value to pointer in system order
 * @param p Data destination
 * @param v 16-bit value to store
 */
static inline void stor_16(void *p, uint16_t v)
{
#if BYTE_ORDER == LITTLE_ENDIAN
	*(uint8_t *) p = v & 0xff;
	*(uint8_t *) (p + 1) = v >> 8;
#else
	*(uint8_t *) p = v >> 8;
	*(uint8_t *) (p + 1) = v & 0xff;
#endif
}

/**
 * Swap bytes in 16-bit value
 * @param v Original 16-bit value
 * @return Swapped 16-bit value
 */
static inline uint16_t swab_16(uint16_t v)
{
	return (v >> 8) | ((v & 0xff) << 8);
}

/**
 * Load 32-bit value in system order from pointer
 * @param p Data source
 * @return 32-bit value from pointer
 */
static inline uint32_t load_32(const void *p)
{
	const uint8_t *p8 = p;
#if BYTE_ORDER == LITTLE_ENDIAN
	return p8[0] + (p8[1] << 8) + (p8[2] << 16) + (p8[3] << 24);
#else
	return p8[3] + (p8[2] << 8) + (p8[1] << 16) + (p8[0] << 24);
#endif
}

/**
 * Store 32-bit value to pointer in system order
 * @param p Data destination
 * @param v 32-bit value to store
 */
static inline void stor_32(void *p, uint32_t v)
{
	uint8_t *p8 = p;
#if BYTE_ORDER == LITTLE_ENDIAN
	p8[0] = v & 0xff;
	p8[1] = (v >> 8) & 0xff;
	p8[2] = (v >> 16) & 0xff;
	p8[3] = v >> 24;
#else
	p8[3] = v & 0xff;
	p8[2] = (v >> 8) & 0xff;
	p8[1] = (v >> 16) & 0xff;
	p8[0] = v >> 24;
#endif
}

/**
 * Swap bytes in 32-bit value
 * @param v Original 32-bit value
 * @return Swapped 32-bit value
 */
static inline uint32_t swab_32(uint32_t v)
{
	return (v >> 24) | ((v & 0x00ff0000) >> 8) | ((v & 0x0000ff00) << 8) | ((v & 0xff) << 24);
}

/**
 * Load 64-bit value in system order from pointer
 * @param p Data source
 * @return 64-bit value from pointer
 */
static inline uint64_t load_64(const void *p)
{
	const uint8_t *p8 = p;
	uint64_t result = 0;
	size_t i;

	for (i = 0; i < sizeof(uint64_t); i++)
	{
		result <<= 8;
#if BYTE_ORDER == LITTLE_ENDIAN
		result |= p8[sizeof(uint64_t) - i - 1];
#else
		result |= p8[i];
#endif
	}
	return result;
}

/**
 * Store 64-bit value to pointer in system order
 * @param p Data destination
 * @param v 64-bit value to store
 */
static inline void stor_64(void *p, uint64_t v)
{
	uint8_t *p8 = p;
	size_t i;

	for (i = 0; i < sizeof(v); i++)
	{
#if BYTE_ORDER == LITTLE_ENDIAN
		p8[i] = v & 0xff;
#else
		p8[sizeof(v) - i - 1] = v & 0xff;
#endif
		v >>= 8;
	}
}

/**
 * Swap bytes in 64-bit value
 * @param v Original 64-bit value
 * @return Swapped 64-bit value
 */
static inline uint64_t swab_64(uint64_t v)
{
	uint64_t result = 0;
	size_t i;

	for (i = 0; i < sizeof(v); i++)
	{
		result <<= 8;
		result |= (v & 0xff);
		v >>= 8;
	}
	return result;
}

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
	int16_t *p;
	size_t i;
	size_t padding = struct_field_padding(context, int16_t);

	memset(buffer, 0, padding);
	p = buffer + padding;

	for (i = 0; i < context->repeat; i++)
	{
		int16_t v = va_arg(*vl, int);
		if (context->byte_order != BYTE_ORDER)
			v = swab_16(v);
		stor_16(p, v);
		p++;
	}

	return (void *) p - buffer;
}

static ssize_t struct_unpack_short(const void *buffer, struct_context *context, va_list *vl)
{
	const int16_t *p;
	size_t i;

	p = buffer + struct_field_padding(context, int16_t);

	for (i = 0; i < context->repeat; i++)
	{
		int16_t v = load_16(p);
		if (context->byte_order != BYTE_ORDER)
			v = swab_16(v);
		*va_arg(*vl, int16_t*) = v;
		p++;
	}

	return (void *) p - buffer;
}

static ssize_t struct_calcsize_short(struct_context *context)
{
	return struct_field_padding(context, int16_t) + context->repeat * sizeof(int16_t);
}

static ssize_t struct_pack_int(void *buffer, struct_context *context, va_list *vl)
{
	uint32_t *p;
	size_t i;
	size_t padding = struct_field_padding(context, uint32_t);

	memset(buffer, 0, padding);
	p = buffer + padding;

	for (i = 0; i < context->repeat; i++)
	{
		uint32_t v = va_arg(*vl, uint32_t);
		if (context->byte_order != BYTE_ORDER)
			v = swab_32(v);
		stor_32(p, v);
		p++;
	}

	return (void *) p - buffer;
}

static ssize_t struct_unpack_int(const void *buffer, struct_context *context, va_list *vl)
{
	const uint32_t *p;
	size_t i;

	p = buffer + struct_field_padding(context, uint32_t);

	for (i = 0; i < context->repeat; i++)
	{
		uint32_t v = load_32(p);
		if (context->byte_order != BYTE_ORDER)
			v = swab_32(v);
		*va_arg(*vl, uint32_t*) = v;
		p++;
	}

	return (void *) p - buffer;
}

static ssize_t struct_calcsize_int(struct_context *context)
{
	return struct_field_padding(context, uint32_t) + context->repeat * sizeof(uint32_t);
}

static ssize_t struct_pack_quad(void *buffer, struct_context *context, va_list *vl)
{
	uint64_t *p;
	size_t i;
	size_t padding = struct_field_padding(context, uint64_t);

	memset(buffer, 0, padding);
	p = buffer + padding;

	for (i = 0; i < context->repeat; i++)
	{
		uint64_t v = va_arg(*vl, uint64_t);
		if (context->byte_order != BYTE_ORDER)
			v = swab_64(v);
		stor_64(p, v);
		p++;
	}

	return (void *) p - buffer;
}

static ssize_t struct_unpack_quad(const void *buffer, struct_context *context, va_list *vl)
{
	const uint64_t *p;
	size_t i;

	p = buffer + struct_field_padding(context, uint64_t);

	for (i = 0; i < context->repeat; i++)
	{
		uint64_t v = load_64(p);
		if (context->byte_order != BYTE_ORDER)
			v = swab_64(v);
		*va_arg(*vl, uint64_t*) = v;
		p++;
	}

	return (void *) p - buffer;
}

static ssize_t struct_calcsize_quad(struct_context *context)
{
	return struct_field_padding(context, uint64_t) + context->repeat * sizeof(uint64_t);
}

static ssize_t struct_pack_float(void *buffer, struct_context *context, va_list *vl)
{
	float *p;
	size_t i;
	size_t padding = struct_field_padding(context, float);

	memset(buffer, 0, padding);
	p = buffer + padding;

	for (i = 0; i < context->repeat; i++)
	{
		float32 v;
		v.f = va_arg(*vl, double);
		if (context->byte_order != BYTE_ORDER)
			v.i = swab_32(v.i);
		stor_32(p, v.i);
		p++;
	}

	return (void *) p - buffer;
}

static ssize_t struct_unpack_float(const void *buffer, struct_context *context, va_list *vl)
{
	const float *p;
	size_t i;

	p = buffer + struct_field_padding(context, float);

	for (i = 0; i < context->repeat; i++)
	{
		float32 v;
		v.i = load_32(p);
		if (context->byte_order != BYTE_ORDER)
			v.i = swab_32(v.i);
		*va_arg(*vl, float*) = v.f;
		p++;
	}

	return (void *) p - buffer;
}

static ssize_t struct_calcsize_float(struct_context *context)
{
	return struct_field_padding(context, float) + context->repeat * sizeof(float);
}

static ssize_t struct_pack_double(void *buffer, struct_context *context, va_list *vl)
{
	double *p = buffer;
	size_t i;
	size_t padding = struct_field_padding(context, double);

	memset(buffer, 0, padding);
	p = buffer + padding;

	for (i = 0; i < context->repeat; i++)
	{
		double64 v;
		v.d = va_arg(*vl, double);
		if (context->byte_order != BYTE_ORDER)
			v.i = swab_64(v.i);
		stor_64(p, v.i);
		p++;
	}

	return (void *) p - buffer;
}

static ssize_t struct_unpack_double(const void *buffer, struct_context *context, va_list *vl)
{
	const double *p;
	size_t i;
	size_t padding = struct_field_padding(context, double);

	p = buffer + padding;

	for (i = 0; i < context->repeat; i++)
	{
		double64 v;
		v.i = load_64(p);
		if (context->byte_order != BYTE_ORDER)
			v.i = swab_64(v.i);
		*va_arg(*vl, double*) = v.d;
		p++;
	}

	return (void *) p - buffer;
}

static ssize_t struct_calcsize_double(struct_context *context)
{
	return struct_field_padding(context, double) + context->repeat * sizeof(double);
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

	return (void *) p - buffer;
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
	if ((*field)->format != '\0')
		c++;
	else
		*field = NULL;

	return c;
}

const char* struct_parse_prefix(const char *format, struct_context *context)
{
	const char *c = format;

	switch (*c)
	{
	case '=':
		c++;
		context->byte_order = __BYTE_ORDER;
		context->native_size = 0;
		context->native_alignment = 0;
		break;

	case '<':
		c++;
		context->byte_order = __LITTLE_ENDIAN;
		context->native_size = 0;
		context->native_alignment = 0;
		break;

	case '>':
	case '!':
		c++;
		context->byte_order = __BIG_ENDIAN;
		context->native_size = 0;
		context->native_alignment = 0;
		break;

	case '@':
		c++;
		/* fall through */
	default:
		context->byte_order = __BYTE_ORDER;
		context->native_size = 1;
		context->native_alignment = 1;
		break;
	}

	return c;
}

//
// Public Services
//

ssize_t struct_pack(void *buffer, size_t size, const char *format, ...)
{
	const char *c, *next;
	struct_context context;
	const struct_format_field *field;
	ssize_t field_size;
	uint8_t *p;
	va_list vl;

	if (buffer == NULL || format == NULL)
		return -1;

	memset(&context, 0, sizeof(context));
	c = struct_parse_prefix(format, &context);
	p = buffer;
	va_start(vl, format);

	while (*c != '\0')
	{
		next = struct_parse_field(c, &context, &field);
		if (field == NULL)
			break;

		field_size = field->calcsize(&context);
		if ((uint8_t *) buffer + size - p < field_size)
			break;

		if (field->pack(p, &context, &vl) != field_size)
			break;

		p += field_size;
		context.offset += field_size;
		c = next;
	}

	va_end(vl);

	// not parse whole format string
	if (*c != '\0')
		return -1;

	return p - (uint8_t *) buffer;
}

ssize_t struct_unpack(const void *buffer, size_t size, const char *format, ...)
{
	const char *c, *next;
	struct_context context;
	const struct_format_field *field;
	ssize_t field_size;
	const uint8_t *p;
	va_list vl;

	if (buffer == NULL || format == NULL)
		return -1;

	memset(&context, 0, sizeof(context));
	c = struct_parse_prefix(format, &context);
	p = buffer;
	va_start(vl, format);

	while (*c != '\0')
	{
		next = struct_parse_field(c, &context, &field);
		if (field == NULL)
			break;

		field_size = field->calcsize(&context);
		if ((const uint8_t *) buffer + size - p < field_size)
			break;

		if (field->unpack(p, &context, &vl) != field_size)
			break;

		p += field_size;
		context.offset += field_size;
		c = next;
	}

	va_end(vl);

	// not parse whole format string
	if (*c != '\0')
		return -1;

	return p - (uint8_t *) buffer;
}

ssize_t struct_calcsize(const char *format)
{
	const char *c, *next;
	struct_context context;
	const struct_format_field *field;
	ssize_t field_size;
	ssize_t result;

	if (format == NULL)
		return -1;

	memset(&context, 0, sizeof(context));
	c = struct_parse_prefix(format, &context);
	result = 0;

	while (*c != '\0')
	{
		next = struct_parse_field(c, &context, &field);
		if (field == NULL)
			break;

		field_size = field->calcsize(&context);
		if (field_size <= 0)
			break;

		result += field_size;
		context.offset += field_size;
		c = next;
	}

	// not parse whole format string
	if (*c != '\0')
		return -1;

	return result;
}

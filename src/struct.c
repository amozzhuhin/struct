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
	int repeat;
} struct_context;

/**
 * Definition of packer function for basic type
 * @param buffer Buffer for store value, must be not NULL and have enough space
 */
typedef ssize_t (*struct_pack_basic)(void *buffer, struct_context *context, va_list *vl);

/** Definition of unpacker function for basic type */
typedef ssize_t (*struct_unpack_basic)(const void *buffer, size_t size,
		struct_context *context, va_list *vl);

/** Definition of function for basic type size calculation */
typedef ssize_t (*struct_calcsize_basic)(struct_context *context);

typedef struct _struct_format_item
{
	char format;
	struct_pack_basic pack;
	struct_unpack_basic unpack;
	struct_calcsize_basic calcsize;
} struct_format_item;

typedef ssize_t (*struct_parse_helper)(struct_context *context, const struct_format_item *item,
		 void *helper_data);

//
// Forward Declarations
//

static ssize_t struct_pack_pad(void *buffer, struct_context *context, va_list *vl);
static ssize_t struct_calcsize_pad(struct_context *context);

static ssize_t struct_pack_byte(void *buffer, struct_context *context, va_list *vl);
static ssize_t struct_calcsize_byte(struct_context *context);

static ssize_t struct_pack_bool(void *buffer, struct_context *context, va_list *vl);
static ssize_t struct_calcsize_bool(struct_context *context);

static ssize_t struct_pack_short(void *buffer, struct_context *context, va_list *vl);
static ssize_t struct_calcsize_short(struct_context *context);

static ssize_t struct_pack_int(void *buffer, struct_context *context, va_list *vl);
static ssize_t struct_calcsize_int(struct_context *context);

static ssize_t struct_pack_quad(void *buffer, struct_context *context, va_list *vl);
static ssize_t struct_calcsize_quad(struct_context *context);

static ssize_t struct_pack_float(void *buffer, struct_context *context, va_list *vl);
static ssize_t struct_calcsize_float(struct_context *context);

static ssize_t struct_pack_double(void *buffer, struct_context *context, va_list *vl);
static ssize_t struct_calcsize_double(struct_context *context);

static ssize_t struct_pack_str(void *buffer, struct_context *context, va_list *vl);
static ssize_t struct_calcsize_str(struct_context *context);


//
// Private Variables
//

/** Format definition table */
static const struct_format_item struct_format_table[] = {
		{ 'x', struct_pack_pad, NULL, struct_calcsize_pad },
		{ 'c', struct_pack_byte, NULL, struct_calcsize_byte },
		{ 'b', struct_pack_byte, NULL, struct_calcsize_byte },
		{ 'B', struct_pack_byte, NULL, struct_calcsize_byte },
		{ '?', struct_pack_bool, NULL, struct_calcsize_bool },
		{ 'h', struct_pack_short, NULL, struct_calcsize_short },
		{ 'H', struct_pack_short, NULL, struct_calcsize_short },
		{ 'i', struct_pack_int, NULL, struct_calcsize_int },
		{ 'I', struct_pack_int, NULL, struct_calcsize_int },
		{ 'l', struct_pack_int, NULL, struct_calcsize_int },
		{ 'L', struct_pack_int, NULL, struct_calcsize_int },
		{ 'q', struct_pack_quad, NULL, struct_calcsize_quad },
		{ 'Q', struct_pack_quad, NULL, struct_calcsize_quad },
		{ 'f', struct_pack_float, NULL, struct_calcsize_float },
		{ 'd', struct_pack_double, NULL, struct_calcsize_double },
		{ 's', struct_pack_str, NULL, struct_calcsize_str },
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

static ssize_t struct_calcsize_pad(struct_context *context)
{
	return context->repeat * sizeof(uint8_t);
}

static ssize_t struct_pack_byte(void *buffer, struct_context *context, va_list *vl)
{
	uint8_t *p = buffer;
	int i;

	for (i = 0; i < context->repeat; i++)
		*p++ = va_arg(*vl, int);

	return context->repeat * sizeof(uint8_t);
}

static ssize_t struct_calcsize_byte(struct_context *context)
{
	return context->repeat * sizeof(uint8_t);
}

static ssize_t struct_pack_bool(void *buffer, struct_context *context, va_list *vl)
{
	uint8_t *p = buffer;
	int i;

	for (i = 0; i < context->repeat; i++)
		*p++ = va_arg(*vl, int) != 0;

	return context->repeat * sizeof(uint8_t);
}

static ssize_t struct_calcsize_bool(struct_context *context)
{
	return context->repeat * sizeof(uint8_t);
}

static ssize_t struct_pack_short(void *buffer, struct_context *context, va_list *vl)
{
	uint16_t *p = buffer;
	int i;

	for (i = 0; i < context->repeat; i++)
		*p++ = va_arg(*vl, int);

	return context->repeat * sizeof(int16_t);
}

static ssize_t struct_calcsize_short(struct_context *context)
{
	return context->repeat * sizeof(int16_t);
}

static ssize_t struct_pack_int(void *buffer, struct_context *context, va_list *vl)
{
	int32_t *p = buffer;
	int i;

	for (i = 0; i < context->repeat; i++)
		*p++ = va_arg(*vl, int32_t);

	return context->repeat * sizeof(int32_t);
}

static ssize_t struct_calcsize_int(struct_context *context)
{
	return context->repeat * sizeof(int32_t);
}

static ssize_t struct_pack_quad(void *buffer, struct_context *context, va_list *vl)
{
	int64_t *p = buffer;
	int i;

	for (i = 0; i < context->repeat; i++)
		*p++ = va_arg(*vl, int64_t);

	return context->repeat * sizeof(int64_t);
}

static ssize_t struct_calcsize_quad(struct_context *context)
{
	return context->repeat * sizeof(int64_t);
}

static ssize_t struct_pack_float(void *buffer, struct_context *context, va_list *vl)
{
	ssize_t result = -1;
	float *p = buffer;
	int i;

	for (i = 0; i < context->repeat; i++)
		*p++ = va_arg(*vl, double);

	return context->repeat * sizeof(float);
}

static ssize_t struct_calcsize_float(struct_context *context)
{
	return context->repeat * sizeof(float);
}

static ssize_t struct_pack_double(void *buffer, struct_context *context, va_list *vl)
{
	double *p = buffer;
	int i;

	for (i = 0; i < context->repeat; i++)
		*p++ = va_arg(*vl, double);

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
	int i;

	for (i = 0; i < context->repeat; i++)
	{
		if (str != NULL && *str != '0')
			*p++ = *str++;
		else
			*p++ = 0;
	}

	return context->repeat * sizeof(char);
}

static ssize_t struct_calcsize_str(struct_context *context)
{
	return context->repeat * sizeof(char);
}

//
// Public Services
//

ssize_t struct_pack(void *buffer, size_t size, const char *format, ...)
{
	struct_context context;
	const char *c;
	uint8_t *p;
	va_list vl;
	int i;

	if (buffer == NULL || format == NULL)
		return -1;

	context.byte_order = __BYTE_ORDER;
	context.native_alignment = 1;
	context.native_size = 1;

	c = format;
	p = buffer;
	va_start(vl, format);

	while ((*c != 0) && (p < (uint8_t *) buffer + size))
	{
		// whitespace characters between formats are ignored
		if (*c == ' ' || *c == '\t')
		{
			c++;
			continue;
		}
		// read integral repeat count
		if (*c >= '0' && *c <= '9')
		{
			context.repeat = 0;
			do {
				context.repeat = context.repeat * 10 + *c - '0';
				c++;
			} while (*c >= '0' && *c <= '9');
		}
		else
		{
			context.repeat = 1;
		}
		// find packer for current basic type
		i = 0;
		while (struct_format_table[i].format != '\0'
				&& struct_format_table[i].format != *c)
			i++;
		// process format character
		if (struct_format_table[i].format != '\0'
				|| (uint8_t *) buffer + size - p >= struct_format_table[i].calcsize(&context))
		{
			p += struct_format_table[i].pack(p, &context, &vl);
		}
		else
		{
			break;
		}

		c++;
	}

	va_end(vl);

	// not processed all format string
	if (*c != 0)
		return -1;

	return p - (uint8_t *) buffer;
}

ssize_t struct_unpack(const void *buffer, size_t size, const char *format, ...)
{
	const char *c;
	const uint8_t *p;
	va_list vl;
	int repeat;

	if (buffer == NULL || format == NULL)
		return -1;

	c = format;
	p = buffer;
	va_start(vl, format);

	while ((*c != 0) && (p - (uint8_t *) buffer < size))
	{
		// whitespace characters between formats are ignored
		if (*c == ' ' || *c == '\t')
		{
			c++;
			continue;
		}
		// read integral repeat count
		if (*c >= '0' && *c <= '9')
		{
			repeat = 0;
			do {
				repeat = repeat * 10 + *c - '0';
				c++;
			} while (*c >= '0' && *c <= '9');
		}
		else
		{
			repeat = 1;
		}
		// process format character
		switch (*c)
		{
		case 'x':
			while (repeat-- > 0) p++;
			break;
		case 'c':
		case 'b':
		case 'B':
			while (repeat-- > 0) *va_arg(vl, uint8_t*) = *p++;
			break;
		case '?':
			while (repeat-- > 0) *va_arg(vl, uint8_t*) = *p++ != 0;
			break;
		case 'h':
		case 'H':
			while (repeat-- > 0)
			{
				*va_arg(vl, int*) = *(int16_t*)p;
				p += sizeof(int16_t);
			}
			break;
		case 'i':
		case 'I':
		case 'l':
		case 'L':
			while (repeat-- > 0)
			{
				*va_arg(vl, int32_t*) = *(int32_t*) p;
				p += sizeof(int32_t);
			}
			break;
		case 'q':
		case 'Q':
			while (repeat-- > 0)
			{
				*va_arg(vl, uint64_t*) = *(int64_t*)p;
				p += sizeof(int64_t);
			}
			break;
		case 'f':
			while (repeat-- > 0)
			{
				*va_arg(vl, float*) = *(float*)p;
				p += sizeof(float);
			}
			break;
		case 'd':
			while (repeat-- > 0)
			{
				*va_arg(vl, double*) = *(double*)p;
				p += sizeof(double);
			}
			break;
		case 's':
			{
				char *str = va_arg(vl, char *);
				size_t str_size = va_arg(vl, size_t);

				while (repeat-- > 0)
				{
					if (str_size > 0)
					{
						*str++ = *p++;
						str_size--;
					}
					else
					{
						p++;
					}
				}

				if (str_size <= 0)
					str--;
				*str = '\0';
			}
			break;
		default:
			// invalid character
			break;
		}
		c++;
	}

	va_end(vl);

	// not processed all format string
	if (*c != 0)
		return -1;

	return p - (uint8_t *) buffer;
}

ssize_t struct_calcsize(const char *format)
{
	return -1;
}

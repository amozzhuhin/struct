/**
 * struct.h
 * Structure packing like python 'struct' module.
 * Python struct format definitions: http://docs.python.org/2/library/struct.html#format-strings
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

#ifndef STRUCT_H_
#define STRUCT_H_

#include <stdlib.h>

//
// Public Services
//

/**
 * Pack binary data to buffer
 * @param buffer Destination buffer
 * @param size Size of destination buffer
 * @param format Format pattern string
 * @param ... Fields to pack
 * @return Size of packed data or negative when failed
 */
ssize_t struct_pack(void *buffer, size_t size, const char *format, ...);

/**
 * Unpack binary data from buffer
 * @param buffer Source buffer
 * @param size Size of destination buffer
 * @param format Format pattern string
 * @param ... Fields to unpack
 * @return Size of unpacked data or negative when failed
 */
ssize_t struct_unpack(const void *buffer, size_t size, const char *format, ...);

/**
 * Calculate size of buffer for givven format pattern
 * @param format Format pattern string
 * @return Calculated data size or negative when failed
 */
ssize_t struct_calcsize(const char *format);

#endif /* STRUCT_H_ */

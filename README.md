struct
======

This is plain C library implementing same functionality as Python [struct](http://docs.python.org/2/library/struct.html) module.

Usage
======

```
// >>> from struct import *
#include <struct.h>

// >>> pack('hhl', 1, 2, 3)
uint8_t buf[100];
ssize_t size;
size = struct_pack(buf, sizeof(buf), "hhl", 1, 2, 3);

// >>> unpack('hhl', '\x00\x01\x00\x02\x00\x00\x00\x03')
short a, b;
long c;
uint8_t buf[] = { 0x01, 0x00, 0x02, 0x00, 0x03, 0x00, 0x00, 0x00 };
size = struct_unpack(buf, sizeof(buf), "hhl", &a, &b, &c);

// >>> calcsize('hhl')
ssize_t size = struct_calcsize("hhl");
```

For more exampes see src/tests.c.

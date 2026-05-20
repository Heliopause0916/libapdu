# libapdu

A lightweight, platform-independent C library for APDU (Application Protocol Data Unit) parsing and assembling, extracted from [OpenSC](https://github.com/OpenSC/OpenSC).

## Features

- **Pure APDU encoding/decoding** — No smartcard reader or transport layer dependencies
- **Supports all ISO 7816-4 APDU cases**:
  - Case 1: No data (CLA, INS, P1, P2 only)
  - Case 2 Short/Extended: Receive data only
  - Case 3 Short/Extended: Send data only
  - Case 4 Short/Extended: Send and receive data
- **T0 and T1 protocol encoding** — Correctly handles length byte formats for both protocols
- **Short and Extended APDUs** — Automatic detection during parsing, manual or auto mode during encoding
- **No external dependencies** — Requires only a C99 standard library
- **Small footprint** — Compiled static library ≈ 13 KB

## API Overview

```c
#include <libapdu/apdu.h>

// Calculate the encoded length of an APDU
size_t apdu_get_length(const apdu_t *apdu, unsigned int proto);

// Encode an APDU structure into a byte string
int apdu_encode(const apdu_t *apdu, unsigned int proto, u8 *out, size_t outlen);

// Allocate a buffer and encode an APDU into it (caller must free())
int apdu_alloc_and_encode(const apdu_t *apdu, u8 **buf, size_t *len, unsigned int proto);

// Decode a byte string into an APDU structure
int apdu_decode(const u8 *buf, size_t len, apdu_t *apdu);

// Set response data and status words (SW1, SW2)
int apdu_set_response(apdu_t *apdu, const u8 *buf, size_t len);
```

## Example

```c
#include <stdio.h>
#include <libapdu/apdu.h>

int main(void) {
    /* Build a SELECT FILE APDU: 00 A4 04 00 02 3F 00 */
    apdu_t apdu;
    u8     encoded[64];
    size_t len;
    u8     file_id[] = { 0x3F, 0x00 };

    apdu.cse     = APDU_CASE_4_SHORT;
    apdu.cla     = 0x00;
    apdu.ins     = 0xA4;
    apdu.p1      = 0x04;
    apdu.p2      = 0x00;
    apdu.lc      = sizeof(file_id);
    apdu.le      = 0;
    apdu.data    = file_id;
    apdu.datalen = sizeof(file_id);

    len = apdu_get_length(&apdu, APDU_PROTO_T0);
    apdu_encode(&apdu, APDU_PROTO_T0, encoded, sizeof(encoded));

    /* encoded[0..len-1] = { 0x00, 0xA4, 0x04, 0x00, 0x02, 0x3F, 0x00 } */

    /* Decode back */
    apdu_t decoded;
    apdu_decode(encoded, len, &decoded);
    printf("CLA=%02X, INS=%02X, P1=%02X, P2=%02X\n",
           decoded.cla, decoded.ins, decoded.p1, decoded.p2);

    return 0;
}
```

## Building

### With CMake

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

### With MSVC (without CMake)

```batch
cl /c /Iinclude src/apdu.c /Fo:apdu.obj
lib apdu.obj /OUT:libapdu.lib
```

### With GCC

```bash
gcc -c -Iinclude src/apdu.c -o apdu.o
ar rcs libapdu.a apdu.o
```

## Integration

Add the following to your CMakeLists.txt:

```cmake
# Option A: FetchContent
include(FetchContent)
FetchContent_Declare(libapdu
    GIT_REPOSITORY https://github.com/your-org/libapdu.git
    GIT_TAG        main
)
FetchContent_MakeAvailable(libapdu)

target_link_libraries(your_target PRIVATE libapdu::libapdu)

# Option B: find_package (after installation)
find_package(libapdu REQUIRED)
target_link_libraries(your_target PRIVATE libapdu::libapdu)
```

## Origin

This library is extracted from [OpenSC](https://github.com/OpenSC/OpenSC) `src/libopensc/apdu.c`. The original code was written by Nils Larsch and is licensed under LGPL 2.1+.

The extraction removes all OpenSC-specific dependencies (`sc_card_t`, `sc_context_t`, logging, secure messaging, reader transport, etc.), keeping only the pure APDU parsing and assembling logic.

## License

LGPL 2.1+ — see [LICENSE](LICENSE) for details.

# libapdu Test Suite Documentation

## Introduction

libapdu includes a comprehensive test suite located in `tests/test_apdu.c`. The test suite is written in pure C99 with no external test framework dependencies, providing thorough coverage of all 8 API functions across various APDU cases and protocol scenarios.

This document provides a detailed analysis of the test suite structure, test functions, and coverage points.

---

## Table of Contents

- [libapdu Test Suite Documentation](#libapdu-test-suite-documentation)
  - [Introduction](#introduction)
  - [Table of Contents](#table-of-contents)
  - [1. Test Framework Overview](#1-test-framework-overview)
    - [Lightweight Design](#lightweight-design)
    - [Framework Features](#framework-features)
    - [Helper Functions](#helper-functions)
  - [2. Test File Location](#2-test-file-location)
  - [3. Test Coverage Summary](#3-test-coverage-summary)
    - [APIs Covered](#apis-covered)
    - [Coverage Categories](#coverage-categories)
  - [4. Test Function Details](#4-test-function-details)
    - [4.1 `test_get_length()`](#41-test_get_length)
    - [4.2 `test_encode()`](#42-test_encode)
    - [4.3 `test_alloc_and_encode()`](#43-test_alloc_and_encode)
    - [4.4 `test_decode()`](#44-test_decode)
    - [4.5 `test_set_response()`](#45-test_set_response)
    - [4.6 `test_get_response_length()`](#46-test_get_response_length)
    - [4.7 `test_encode_response()`](#47-test_encode_response)
    - [4.8 `test_alloc_and_encode_response()`](#48-test_alloc_and_encode_response)
  - [5. Running Tests](#5-running-tests)
    - [Build and Run](#build-and-run)
    - [Expected Output](#expected-output)
    - [Exit Codes](#exit-codes)
  - [References](#references)

---

## 1. Test Framework Overview

### Lightweight Design

The test framework is implemented directly in the test file without any external dependencies:

```c
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_START(name)       do { printf("  - %s ... ", name); } while (0)
#define TEST_PASS()            do { tests_passed++; printf("PASS\n"); } while (0)
#define TEST_FAIL(msg)         do { tests_failed++; printf("FAIL: %s\n", msg); } while (0)
#define TEST_ASSERT(cond, msg) do {                                    \
        if (!(cond)) { TEST_FAIL(msg); return; }                       \
    } while (0)
```

### Framework Features

| Component | Description |
|-----------|-------------|
| `tests_passed` | Global counter for passed tests |
| `tests_failed` | Global counter for failed tests |
| `TEST_START(name)` | Prints test name with formatting |
| `TEST_PASS()` | Increments passed counter and prints "PASS" |
| `TEST_FAIL(msg)` | Increments failed counter and prints failure message |
| `TEST_ASSERT(cond, msg)` | Validates condition; on failure, records and returns early |

### Helper Functions

```c
static void setup_apdu(apdu_t *apdu, int cse, u8 cla, u8 ins, u8 p1, u8 p2,
                       size_t lc, const u8 *data, size_t le);
static int u8cmp(const u8 *a, const u8 *b, size_t n);
```

- `setup_apdu()`: Initializes an APDU structure with specified parameters
- `u8cmp()`: Compares byte arrays using `memcmp()`

---

## 2. Test File Location

| Item | Path |
|------|------|
| Test source | `tests/test_apdu.c` |
| Test lines | 1004 lines |
| Test functions | 8 functions |
| Total test cases | 92 assertions |

---

## 3. Test Coverage Summary

### APIs Covered

| API Function | Test Function | Test Count |
|--------------|---------------|------------|
| `apdu_get_length()` | `test_get_length()` | 15 |
| `apdu_encode()` | `test_encode()` | 18 |
| `apdu_alloc_and_encode()` | `test_alloc_and_encode()` | 8 |
| `apdu_decode()` | `test_decode()` | 20 |
| `apdu_set_response()` | `test_set_response()` | 8 |
| `apdu_get_response_length()` | `test_get_response_length()` | 8 |
| `apdu_encode_response()` | `test_encode_response()` | 7 |
| `apdu_alloc_and_encode_response()` | `test_alloc_and_encode_response()` | 8 |
| **Total** | | **92** |

### Coverage Categories

| Category | Covered |
|----------|---------|
| APDU Cases (1, 2, 3, 4) | ✅ |
| Short format | ✅ |
| Extended format | ✅ |
| T=0 protocol | ✅ |
| T=1 protocol | ✅ |
| Error handling | ✅ |
| Boundary conditions | ✅ |
| NULL parameter validation | ✅ |
| Memory allocation | ✅ |

---

## 4. Test Function Details

### 4.1 `test_get_length()`

**Purpose:** Test `apdu_get_length()` for calculating encoded byte length.

**Test Count:** 15

**Coverage Points:**

| Test Name | Description | Protocol |
|-----------|-------------|----------|
| Case1 T0 -> 5 | Case 1 with T=0 protocol returns 5 bytes | T0 |
| Case1 T1 -> 4 | Case 1 with T=1 protocol returns 4 bytes | T1 |
| Case2Short Le=255 -> 5 | Case 2 Short with Le=255 returns 5 bytes | - |
| Case2Ext T0 -> 5 | Case 2 Extended with T=0 protocol | T0 |
| Case2Ext T1 -> 7 | Case 2 Extended with T=1 protocol returns 7 bytes | T1 |
| Case3Short lc=10 -> 15 | Case 3 Short with 10-byte data | - |
| Case3Ext T0 lc=10 -> 15 | Case 3 Extended with T=0 protocol | T0 |
| Case3Ext T1 lc=10 -> 17 | Case 3 Extended with T=1 protocol | T1 |
| Case4Short lc=5 T0 -> 10 | Case 4 Short with T=0 protocol | T0 |
| Case4Short lc=5 T1 -> 11 | Case 4 Short with T=1 protocol | T1 |
| Case4Ext lc=5 T0 -> 10 | Case 4 Extended with T=0 protocol | T0 |
| Case4Ext lc=5 T1 -> 14 | Case 4 Extended with T=1 protocol | T1 |
| Invalid cse -> 0 | Invalid `cse` value returns 0 | - |
| Unknown cse 0x99 -> 0 | Unknown `cse` value returns 0 | - |

**Key Validations:**

- All four APDU cases (1, 2, 3, 4)
- Both Short and Extended formats
- T=0 and T=1 protocol differences
- Invalid/unknown `cse` handling

---

### 4.2 `test_encode()`

**Purpose:** Test `apdu_encode()` for encoding APDU structures to byte sequences.

**Test Count:** 18

**Coverage Points:**

| Test Name | Description | Expected Result |
|-----------|-------------|-----------------|
| Case1 T0 encode | Case 1 with T=0 protocol | `{0x00, 0xA4, 0x00, 0x00, 0x00}` |
| Case1 T1 encode | Case 1 with T=1 protocol | `{0x00, 0xA4, 0x00, 0x00}` |
| Case2Short Le=255 encode | Case 2 Short Le=255 | `{0x00, 0xC0, 0x00, 0x00, 0xFF}` |
| Case2Ext T0 encode | Case 2 Extended T=0 | `{0x00, 0xC0, 0x00, 0x00, 0x00}` |
| Case2Ext T1 encode | Case 2 Extended T=1 | `{0x00, 0xC0, 0x00, 0x00, 0x00, 0x01, 0x00}` |
| Case3Short lc=3 encode | Case 3 Short with 3-byte data | `{0x00, 0xDA, 0x00, 0x00, 0x03, 0x11, 0x22, 0x33}` |
| Case3Ext T0 encode | Case 3 Extended T=0 | `{0x00, 0xDA, 0x00, 0x00, 0x03, 0x11, 0x22, 0x33}` |
| Case3Ext T1 encode | Case 3 Extended T=1 | `{0x00, 0xDA, 0x00, 0x00, 0x00, 0x00, 0x03, 0x11, 0x22, 0x33}` |
| Case4Short lc=2 Le=128 T0 encode | Case 4 Short T=0 | `{0x00, 0xC0, 0x00, 0x01, 0x02, 0xAA, 0xBB}` |
| Case4Short lc=2 Le=128 T1 encode | Case 4 Short T=1 | `{0x00, 0xC0, 0x00, 0x01, 0x02, 0xAA, 0xBB, 0x80}` |
| Case4Ext lc=2 Le=256 T0 encode | Case 4 Extended T=0 | `{0x00, 0xC0, 0x00, 0x01, 0x02, 0xAA, 0xBB}` |
| Case4Ext lc=2 Le=256 T1 encode | Case 4 Extended T=1 | `{0x00, 0xC0, 0x00, 0x01, 0x00, 0x00, 0x02, 0xAA, 0xBB, 0x01, 0x00}` |
| Buffer too small | Insufficient buffer size | `APDU_ERROR_INVALID_ARGUMENTS` |
| NULL out buffer | NULL output buffer | `APDU_ERROR_INVALID_ARGUMENTS` |

**Key Validations:**

- Correct byte sequence generation for all cases
- T=0 vs T=1 protocol encoding differences
- Buffer size validation
- NULL parameter handling

---

### 4.3 `test_alloc_and_encode()`

**Purpose:** Test `apdu_alloc_and_encode()` for memory allocation and encoding.

**Test Count:** 8

**Coverage Points:**

| Test Name | Description | Expected Result |
|-----------|-------------|-----------------|
| Alloc and encode Case3Short | Normal allocation | Valid buffer, correct encoding |
| Alloc and encode Case4Ext T1 | Extended format allocation | Valid buffer, correct encoding |
| NULL apdu returns INVALID_ARGUMENTS | NULL APDU parameter | `APDU_ERROR_INVALID_ARGUMENTS` |
| NULL buf returns INVALID_ARGUMENTS | NULL buffer pointer | `APDU_ERROR_INVALID_ARGUMENTS` |
| NULL len returns INVALID_ARGUMENTS | NULL length pointer | `APDU_ERROR_INVALID_ARGUMENTS` |
| Invalid cse returns INTERNAL | Invalid `cse` value | `APDU_ERROR_INTERNAL` |

**Key Validations:**

- Memory allocation success
- Correct encoding after allocation
- Caller responsibility to `free()` the buffer
- All NULL parameter combinations
- Invalid `cse` handling

---

### 4.4 `test_decode()`

**Purpose:** Test `apdu_decode()` for decoding byte sequences to APDU structures.

**Test Count:** 20

**Coverage Points:**

| Test Name | Description | Key Assertions |
|-----------|-------------|----------------|
| Decode Case1 | 4-byte input | `cse == APDU_CASE_1` |
| Decode Case2Short Le=0 -> 256 | Le=0 interpreted as 256 | `le == 256` |
| Decode Case2Short Le=128 | Le=0x80 | `le == 128` |
| Decode Case2Ext Le=256 | Extended format | `le == 256` |
| Decode Case2Ext Le=0 -> 65536 | Le=0 interpreted as 65536 | `le == 65536` |
| Decode Case3Short lc=3 | Lc + data parsing | `lc == 3`, `data == buf + 5` |
| Decode Case3Ext lc=3 | Extended Lc + data | `lc == 3`, `data == buf + 7` |
| Decode Case4Short lc=2 Le=128 | Lc + data + Le | `lc == 2`, `le == 128` |
| Decode Case4Ext lc=2 Le=256 | Extended full format | `lc == 2`, `le == 256` |
| Input too short (3 bytes) | Length < 4 | `APDU_ERROR_INVALID_DATA` |
| Input empty (0 bytes) | Zero length | `APDU_ERROR_INVALID_DATA` |
| NULL buf -> INVALID_ARGUMENTS | NULL buffer | `APDU_ERROR_INVALID_ARGUMENTS` |
| NULL apdu -> INVALID_ARGUMENTS | NULL APDU structure | `APDU_ERROR_INVALID_ARGUMENTS` |
| len < lc -> INVALID_DATA | Insufficient data | `APDU_ERROR_INVALID_DATA` |
| Trailing garbage -> INVALID_DATA | Extra bytes after parse | `APDU_ERROR_INVALID_DATA` |

**Key Validations:**

- All four APDU cases in short and extended formats
- Le=0 → 256 (short) / 65536 (extended) conversion
- `apdu->data` points into input buffer (no copy)
- NULL parameter handling
- Input validation (length, data completeness)

---

### 4.5 `test_set_response()`

**Purpose:** Test `apdu_set_response()` for setting response data and status words.

**Test Count:** 8

**Coverage Points:**

| Test Name | Description | Key Assertions |
|-----------|-------------|----------------|
| Set response: 6 bytes, resplen=3 | Data truncation | `sw1 == 'E'`, `sw2 == 'F'`, `resplen == 3` |
| Set response: resplen bigger than data | Full data copy | `resplen == 3` (all data) |
| Set response: only SW (len=2) | No response data | `resplen == 0` |
| Set response: resp=NULL, resplen updated | NULL buffer handling | `resplen == 3`, no copy |
| Set response: len=1 -> INTERNAL error | Insufficient length | `APDU_ERROR_INTERNAL` |
| Set response: len=0 -> INTERNAL error | Zero length | `APDU_ERROR_INTERNAL` |

**Key Validations:**

- Response data copying
- Status word extraction (last 2 bytes)
- `resplen` truncation behavior
- NULL `resp` buffer handling
- Minimum length validation (len >= 2)

---

### 4.6 `test_get_response_length()`

**Purpose:** Test `apdu_get_response_length()` for calculating R-APDU encoded length.

**Test Count:** 8

**Coverage Points:**

| Test Name | Description | Expected Result |
|-----------|-------------|-----------------|
| resplen=0 -> returns 2 | No response data | 2 bytes (SW1+SW2) |
| resplen=10 -> returns 12 | Normal response | 12 bytes |
| resplen=255 -> returns 257 | Maximum short response | 257 bytes |
| NULL apdu -> returns 0 | NULL parameter | 0 |
| resplen=SIZE_MAX -> returns 0 | Overflow protection | 0 |
| resplen=SIZE_MAX-2 -> returns SIZE_MAX | Boundary case | SIZE_MAX |
| resplen=SIZE_MAX-1 -> returns 0 | Overflow case | 0 |

**Key Validations:**

- Length calculation: `resplen + 2`
- NULL parameter handling
- Overflow protection for large `resplen`

---

### 4.7 `test_encode_response()`

**Purpose:** Test `apdu_encode_response()` for encoding R-APDU to caller buffer.

**Test Count:** 7

**Coverage Points:**

| Test Name | Description | Expected Result |
|-----------|-------------|-----------------|
| Normal encode: resp data + SW | Normal encoding | `{0x01, 0x02, 0x03, 0x90, 0x00}` |
| Only SW: resplen=0 | Status words only | `{0x61, 0x05}` |
| Buffer too small (need 7, give 5) | Insufficient buffer | `APDU_ERROR_INVALID_ARGUMENTS` |
| NULL apdu -> INVALID_ARGUMENTS | NULL APDU parameter | `APDU_ERROR_INVALID_ARGUMENTS` |
| NULL out -> INVALID_ARGUMENTS | NULL output buffer | `APDU_ERROR_INVALID_ARGUMENTS` |
| resplen>0 but resp=NULL -> INVALID_ARGUMENTS | NULL response data | `APDU_ERROR_INVALID_ARGUMENTS` |

**Key Validations:**

- Response data + SW encoding
- Status words only encoding
- Buffer size validation
- NULL parameter handling
- Inconsistent `resp`/`resplen` validation

---

### 4.8 `test_alloc_and_encode_response()`

**Purpose:** Test `apdu_alloc_and_encode_response()` for memory allocation and R-APDU encoding.

**Test Count:** 8

**Coverage Points:**

| Test Name | Description | Expected Result |
|-----------|-------------|-----------------|
| Normal alloc and encode | Normal allocation | Valid buffer, correct encoding |
| Alloc and encode: only SW (no data) | Status words only | `{0x61, 0x03}` |
| NULL apdu -> INVALID_ARGUMENTS | NULL APDU parameter | `APDU_ERROR_INVALID_ARGUMENTS` |
| NULL buf -> INVALID_ARGUMENTS | NULL buffer pointer | `APDU_ERROR_INVALID_ARGUMENTS` |
| NULL len -> INVALID_ARGUMENTS | NULL length pointer | `APDU_ERROR_INVALID_ARGUMENTS` |
| resplen>0 but resp=NULL -> INTERNAL | Inconsistent state | `APDU_ERROR_INTERNAL` |

**Key Validations:**

- Memory allocation success
- Correct encoding after allocation
- Caller responsibility to `free()` the buffer
- All NULL parameter combinations
- Inconsistent `resp`/`resplen` handling

---

## 5. Running Tests

### Build and Run

```bash
# Create build directory
mkdir build && cd build

# Configure with CMake
cmake ..

# Build
cmake --build .

# Run tests
./test_apdu  # Linux/macOS
test_apdu.exe  # Windows
```

### Expected Output

```
========================================
  libapdu API Test Suite
========================================

[apdu_get_length]
  - Case1 T0 -> 5 ... PASS
  - Case1 T1 -> 4 ... PASS
  ...
  - Unknown cse 0x99 -> 0 ... PASS

[apdu_encode]
  - Case1 T0 encode ... PASS
  - Case1 T1 encode ... PASS
  ...
  - NULL out buffer ... PASS

[apdu_alloc_and_encode]
  - Alloc and encode Case3Short ... PASS
  ...
  - Invalid cse returns INTERNAL ... PASS

[apdu_decode]
  - Decode Case1 ... PASS
  ...
  - Trailing garbage -> INVALID_DATA ... PASS

[apdu_set_response]
  ...
  - Set response: len=0 -> INTERNAL error ... PASS

[apdu_get_response_length]
  ...
  - resplen=SIZE_MAX-1 -> returns 0 (overflow) ... PASS

[apdu_encode_response]
  ...
  - resplen>0 but resp=NULL -> INVALID_ARGUMENTS ... PASS

[apdu_alloc_and_encode_response]
  ...
  - resplen>0 but resp=NULL -> INTERNAL (via encode_response) ... PASS

========================================
  Results: 92 passed, 0 failed
========================================
```

### Exit Codes

| Exit Code | Meaning |
|-----------|---------|
| 0 | All tests passed |
| 1 | One or more tests failed |

---

## References

- [libapdu Library Principles](./LIBAPDU_PRINCIPLES.md)
- ISO/IEC 7816-4: Organization, security and commands for interchange
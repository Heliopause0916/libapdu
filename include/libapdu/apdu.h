/* SPDX-License-Identifier: LGPL-2.1-or-later
 * apdu.h: Public API for APDU parsing and assembling library
 *
 * Originally derived from OpenSC's apdu.c / types.h
 * Copyright (C) 2005 Nils Larsch <nils@larsch.net>
 * Copyright (C) 2025 Steven Weng <stevenw0916@mail.ustc.edu.cn>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef LIBAPDU_APDU_H
#define LIBAPDU_APDU_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ================ Type Aliases ================ */

typedef uint8_t u8;

/* ================ Error Codes ================ */

#define APDU_SUCCESS                 0
#define APDU_ERROR_INVALID_ARGUMENTS -1300
#define APDU_ERROR_INTERNAL          -1400
#define APDU_ERROR_OUT_OF_MEMORY     -1404
#define APDU_ERROR_INVALID_DATA      -1305

/* ================ Protocol Constants ================ */

#define APDU_PROTO_T0  0x00000001
#define APDU_PROTO_T1  0x00000002

/* ================ APDU Case Macros ================ */

#define APDU_CASE_NONE       0x00
#define APDU_CASE_1          0x01
#define APDU_CASE_2_SHORT    0x02
#define APDU_CASE_3_SHORT    0x03
#define APDU_CASE_4_SHORT    0x04
#define APDU_SHORT_MASK      0x0f
#define APDU_EXT             0x10
#define APDU_CASE_2_EXT      (APDU_CASE_2_SHORT | APDU_EXT)
#define APDU_CASE_3_EXT      (APDU_CASE_3_SHORT | APDU_EXT)
#define APDU_CASE_4_EXT      (APDU_CASE_4_SHORT | APDU_EXT)
/* Auto-detect short vs extended */
#define APDU_CASE_2          0x22
#define APDU_CASE_3          0x23
#define APDU_CASE_4          0x24

/* ================ APDU Flags ================ */

#define APDU_FLAGS_CHAINING        0x00000001UL
#define APDU_FLAGS_NO_GET_RESP     0x00000002UL
#define APDU_FLAGS_NO_RETRY_WL     0x00000004UL

/* ================ Buffer Size Limits ================ */

#define APDU_MAX_SHORT_BUFFER_SIZE   261   /* CLA INS P1 P2 Lc [255 data] Le */
#define APDU_MAX_SHORT_DATA_SIZE     0xFF
#define APDU_MAX_SHORT_RESP_SIZE     (0xFF + 1)
#define APDU_MAX_EXT_BUFFER_SIZE     65538
#define APDU_MAX_EXT_DATA_SIZE       0xFFFF
#define APDU_MAX_EXT_RESP_SIZE       (0xFFFF + 1)

/* ================ APDU Structure ================ */

typedef struct apdu {
    int      cse;            /* APDU case (APDU_CASE_*) */
    u8       cla, ins, p1, p2;  /* CLA, INS, P1, P2 bytes */
    size_t   lc, le;         /* Lc (command data length) and Le (expected response length) */
    const u8 *data;          /* C-APDU command data */
    size_t   datalen;        /* Length of command data */
    u8       *resp;          /* R-APDU response buffer */
    size_t   resplen;        /* In: buffer size; Out: returned data length */
    u8       control;        /* Reader control flag */
    unsigned sw1, sw2;       /* Status words from R-APDU */
    u8       mac[8];         /* MAC for secure messaging */
    size_t   mac_len;        /* MAC length */
    unsigned long flags;     /* APDU flags (APDU_FLAGS_*) */
    struct apdu *next;       /* Linked list for command chaining */
} apdu_t;

/* ================ API Functions ================ */

/**
 * Calculate the encoded length of an APDU in octets.
 *
 * @param apdu   The APDU structure
 * @param proto  Protocol (APDU_PROTO_T0 or APDU_PROTO_T1)
 * @return Encoded length in bytes, or 0 on invalid input
 */
size_t apdu_get_length(const apdu_t *apdu, unsigned int proto);

/**
 * Encode an APDU structure into a byte string.
 *
 * @param apdu    APDU to encode
 * @param proto   Protocol (APDU_PROTO_T0 or APDU_PROTO_T1)
 * @param out     Output buffer
 * @param outlen  Size of output buffer
 * @return APDU_SUCCESS on success, APDU_ERROR_INVALID_ARGUMENTS if buffer too small
 */
int apdu_encode(const apdu_t *apdu, unsigned int proto, u8 *out, size_t outlen);

/**
 * Allocate a buffer and encode an APDU into it.
 * The caller must free() the returned buffer.
 *
 * @param apdu   APDU to encode
 * @param buf    Output pointer to allocated buffer
 * @param len    Output length of encoded APDU
 * @param proto  Protocol (APDU_PROTO_T0 or APDU_PROTO_T1)
 * @return APDU_SUCCESS on success, or an error code
 */
int apdu_alloc_and_encode(const apdu_t *apdu, u8 **buf, size_t *len, unsigned int proto);

/**
 * Decode a byte string into an APDU structure.
 *
 * On success, apdu->data will point into the input buffer at the
 * appropriate offset. The caller must not free buf while apdu->data is in use.
 *
 * @param buf   Input byte string
 * @param len   Length of input
 * @param apdu  Output APDU structure (will be zero-initialized then filled)
 * @return APDU_SUCCESS on success, or an error code
 */
int apdu_decode(const u8 *buf, size_t len, apdu_t *apdu);

/**
 * Set response data and status words (SW1, SW2) on an APDU structure.
 *
 * The last two bytes of buf are interpreted as SW1 and SW2.
 * The remaining bytes are copied into apdu->resp.
 *
 * @param apdu  APDU structure to update
 * @param buf   Response data (last 2 bytes = SW1, SW2)
 * @param len   Length of response data (must be >= 2)
 * @return APDU_SUCCESS on success, APDU_ERROR_INTERNAL if len < 2
 */
int apdu_set_response(apdu_t *apdu, const u8 *buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* LIBAPDU_APDU_H */

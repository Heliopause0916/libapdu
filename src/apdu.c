/* SPDX-License-Identifier: LGPL-2.1-or-later
 * apdu.c: Basic APDU parsing and assembling functions
 *
 * Originally derived from OpenSC's libopensc/apdu.c
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

#include <stdlib.h>
#include <string.h>

#include "libapdu/apdu.h"

/* ================================================================ */
/*  Low-level APDU length calculation                               */
/* ================================================================ */

size_t apdu_get_length(const apdu_t *apdu, unsigned int proto)
{
    size_t ret = 4;

    switch (apdu->cse) {
    case APDU_CASE_1:
        if (proto == APDU_PROTO_T0)
            ret++;
        break;
    case APDU_CASE_2_SHORT:
        ret++;
        break;
    case APDU_CASE_2_EXT:
        ret += (proto == APDU_PROTO_T0 ? 1 : 3);
        break;
    case APDU_CASE_3_SHORT:
        ret += 1 + apdu->lc;
        break;
    case APDU_CASE_3_EXT:
        ret += apdu->lc + (proto == APDU_PROTO_T0 ? 1 : 3);
        break;
    case APDU_CASE_4_SHORT:
        ret += apdu->lc + (proto != APDU_PROTO_T0 ? 2 : 1);
        break;
    case APDU_CASE_4_EXT:
        ret += apdu->lc + (proto == APDU_PROTO_T0 ? 1 : 5);
        break;
    default:
        return 0;
    }
    return ret;
}

/* ================================================================ */
/*  APDU encoding (structure -> byte string)                         */
/* ================================================================ */

int apdu_encode(const apdu_t *apdu, unsigned int proto, u8 *out, size_t outlen)
{
    u8     *p = out;
    size_t len = apdu_get_length(apdu, proto);

    if (out == NULL || outlen < len)
        return APDU_ERROR_INVALID_ARGUMENTS;

    /* CLA, INS, P1 and P2 */
    *p++ = apdu->cla;
    *p++ = apdu->ins;
    *p++ = apdu->p1;
    *p++ = apdu->p2;

    /* Case-dependent part */
    switch (apdu->cse) {
    case APDU_CASE_1:
        /* T0 needs an additional 0x00 byte */
        if (proto == APDU_PROTO_T0)
            *p = (u8)0x00;
        break;

    case APDU_CASE_2_SHORT:
        *p = (u8)apdu->le;
        break;

    case APDU_CASE_2_EXT:
        if (proto == APDU_PROTO_T0) {
            /* T0 extended APDUs look just like short APDUs */
            *p = (u8)apdu->le;
        } else {
            /* T1 always uses 3 bytes for length */
            *p++ = (u8)0x00;
            *p++ = (u8)(apdu->le >> 8);
            *p   = (u8)apdu->le;
        }
        break;

    case APDU_CASE_3_SHORT:
        *p++ = (u8)apdu->lc;
        memcpy(p, apdu->data, apdu->lc);
        break;

    case APDU_CASE_3_EXT:
        if (proto == APDU_PROTO_T0) {
            /* For T0, Lc > 255 requires ENVELOPE, which is handled
             * at a higher level. Here we just reject. */
            if (apdu->lc > 255)
                return APDU_ERROR_INVALID_ARGUMENTS;
        } else {
            /* T1 uses 3 bytes for length */
            *p++ = (u8)0x00;
            *p++ = (u8)(apdu->lc >> 8);
            *p++ = (u8)apdu->lc;
        }
        memcpy(p, apdu->data, apdu->lc);
        break;

    case APDU_CASE_4_SHORT:
        *p++ = (u8)apdu->lc;
        memcpy(p, apdu->data, apdu->lc);
        p += apdu->lc;
        if (proto != APDU_PROTO_T0)
            *p = (u8)apdu->le;
        break;

    case APDU_CASE_4_EXT:
        if (proto == APDU_PROTO_T0) {
            /* T0 extended case 4 looks like short APDU */
            *p++ = (u8)apdu->lc;
            memcpy(p, apdu->data, apdu->lc);
        } else {
            *p++ = (u8)0x00;
            *p++ = (u8)(apdu->lc >> 8);
            *p++ = (u8)apdu->lc;
            memcpy(p, apdu->data, apdu->lc);
            p += apdu->lc;
            *p++ = (u8)(apdu->le >> 8);
            *p   = (u8)apdu->le;
        }
        break;
    }

    return APDU_SUCCESS;
}

/* ================================================================ */
/*  APDU encode with allocation                                      */
/* ================================================================ */

int apdu_alloc_and_encode(const apdu_t *apdu, u8 **buf, size_t *len, unsigned int proto)
{
    size_t nlen;
    u8    *nbuf;

    if (apdu == NULL || buf == NULL || len == NULL)
        return APDU_ERROR_INVALID_ARGUMENTS;

    nlen = apdu_get_length(apdu, proto);
    if (nlen == 0)
        return APDU_ERROR_INTERNAL;

    nbuf = (u8 *)malloc(nlen);
    if (nbuf == NULL)
        return APDU_ERROR_OUT_OF_MEMORY;

    if (apdu_encode(apdu, proto, nbuf, nlen) != APDU_SUCCESS) {
        free(nbuf);
        return APDU_ERROR_INTERNAL;
    }

    *buf = nbuf;
    *len = nlen;
    return APDU_SUCCESS;
}

/* ================================================================ */
/*  APDU decoding (byte string -> structure)                         */
/* ================================================================ */

int apdu_decode(const u8 *buf, size_t len, apdu_t *apdu)
{
    const u8 *p;
    size_t len0;

    if (!buf || !apdu)
        return APDU_ERROR_INVALID_ARGUMENTS;

    len0 = len;
    if (len < 4)
        return APDU_ERROR_INVALID_DATA;

    memset(apdu, 0, sizeof(*apdu));
    p = buf;
    apdu->cla = *p++;
    apdu->ins = *p++;
    apdu->p1  = *p++;
    apdu->p2  = *p++;
    len -= 4;

    if (!len) {
        apdu->cse = APDU_CASE_1;
        return APDU_SUCCESS;
    }

    if (*p == 0 && len >= 3) {
        /* Extended APDU */
        p++;
        if (len == 3) {
            apdu->le  = (size_t)(*p++) << 8;
            apdu->le += *p++;
            if (apdu->le == 0)
                apdu->le = 0xFFFF + 1;
            len -= 3;
            apdu->cse = APDU_CASE_2_EXT;
        } else {
            /* len > 3 */
            apdu->lc  = (size_t)(*p++) << 8;
            apdu->lc += *p++;
            len -= 3;
            if (len < apdu->lc)
                return APDU_ERROR_INVALID_DATA;

            apdu->data    = p;
            apdu->datalen = apdu->lc;
            len -= apdu->lc;
            p += apdu->lc;

            if (!len) {
                apdu->cse = APDU_CASE_3_EXT;
            } else {
                if (len < 2)
                    return APDU_ERROR_INVALID_DATA;

                apdu->le  = (size_t)(*p++) << 8;
                apdu->le += *p++;
                if (apdu->le == 0)
                    apdu->le = 0xFFFF + 1;
                len -= 2;
                apdu->cse = APDU_CASE_4_EXT;
            }
        }
    } else {
        /* Short APDU */
        if (len == 1) {
            apdu->le = *p++;
            if (apdu->le == 0)
                apdu->le = 0xFF + 1;
            len--;
            apdu->cse = APDU_CASE_2_SHORT;
        } else {
            apdu->lc = *p++;
            len--;
            if (len < apdu->lc)
                return APDU_ERROR_INVALID_DATA;

            apdu->data    = p;
            apdu->datalen = apdu->lc;
            len -= apdu->lc;
            p += apdu->lc;

            if (!len) {
                apdu->cse = APDU_CASE_3_SHORT;
            } else {
                apdu->le = *p++;
                if (apdu->le == 0)
                    apdu->le = 0xFF + 1;
                len--;
                apdu->cse = APDU_CASE_4_SHORT;
            }
        }
    }

    if (len)
        return APDU_ERROR_INVALID_DATA;

    return APDU_SUCCESS;
}

/* ================================================================ */
/*  Set response status and data                                     */
/* ================================================================ */

int apdu_set_response(apdu_t *apdu, const u8 *buf, size_t len)
{
    if (len < 2)
        return APDU_ERROR_INTERNAL;

    /* Last two bytes are SW1, SW2 */
    apdu->sw1 = (unsigned)buf[len - 2];
    apdu->sw2 = (unsigned)buf[len - 1];
    len -= 2;

    /* Copy returned data if buffer is large enough */
    if (len <= apdu->resplen)
        apdu->resplen = len;

    if (apdu->resplen != 0 && apdu->resp != NULL)
        memcpy(apdu->resp, buf, apdu->resplen);

    return APDU_SUCCESS;
}

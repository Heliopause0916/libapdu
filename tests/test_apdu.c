/* SPDX-License-Identifier: LGPL-2.1-or-later
 * test_apdu.c: Comprehensive tests for all libapdu API functions
 *
 * Pure C99, no external test framework dependencies.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "libapdu/apdu.h"

/* ================ Test Framework Macros ================ */

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_START(name)       do { printf("  - %s ... ", name); } while (0)
#define TEST_PASS()            do { tests_passed++; printf("PASS\n"); } while (0)
#define TEST_FAIL(msg)         do { tests_failed++; printf("FAIL: %s\n", msg); } while (0)
#define TEST_ASSERT(cond, msg) do {                                    \
        if (!(cond)) { TEST_FAIL(msg); return; }                       \
    } while (0)

/* ================ Helpers ================ */

static void setup_apdu(apdu_t *apdu, int cse, u8 cla, u8 ins, u8 p1, u8 p2,
                       size_t lc, const u8 *data, size_t le)
{
    memset(apdu, 0, sizeof(*apdu));
    apdu->cse     = cse;
    apdu->cla     = cla;
    apdu->ins     = ins;
    apdu->p1      = p1;
    apdu->p2      = p2;
    apdu->lc      = lc;
    apdu->datalen = lc;
    apdu->data    = data;
    apdu->le      = le;
}

static int u8cmp(const u8 *a, const u8 *b, size_t n)
{
    return memcmp(a, b, n);
}

/* ================ Test: apdu_get_length ================ */

static void test_get_length(void)
{
    printf("[apdu_get_length]\n");

    apdu_t ap;
    memset(&ap, 0, sizeof(ap));
    u8 dummy = 0;

    /* Case 1 */
    setup_apdu(&ap, APDU_CASE_1, 0, 0, 0, 0, 0, NULL, 0);
    TEST_START("Case1 T0 -> 5");
    TEST_ASSERT(apdu_get_length(&ap, APDU_PROTO_T0) == 5, "expected 5");
    TEST_PASS();

    TEST_START("Case1 T1 -> 4");
    TEST_ASSERT(apdu_get_length(&ap, APDU_PROTO_T1) == 4, "expected 4");
    TEST_PASS();

    /* Case 2 Short: Le=255 */
    setup_apdu(&ap, APDU_CASE_2_SHORT, 0, 0, 0, 0, 0, NULL, 255);
    TEST_START("Case2Short Le=255 -> 5");
    TEST_ASSERT(apdu_get_length(&ap, APDU_PROTO_T0) == 5, "expected 5");
    TEST_PASS();

    /* Case 2 Ext */
    setup_apdu(&ap, APDU_CASE_2_EXT, 0, 0, 0, 0, 0, NULL, 256);
    TEST_START("Case2Ext T0 -> 5");
    TEST_ASSERT(apdu_get_length(&ap, APDU_PROTO_T0) == 5, "expected 5");
    TEST_PASS();

    TEST_START("Case2Ext T1 -> 7");
    TEST_ASSERT(apdu_get_length(&ap, APDU_PROTO_T1) == 7, "expected 7");
    TEST_PASS();

    /* Case 3 Short: lc=10 */
    {
        u8 data10[10] = {1,2,3,4,5,6,7,8,9,10};
        setup_apdu(&ap, APDU_CASE_3_SHORT, 0, 0, 0, 0, 10, data10, 0);
        TEST_START("Case3Short lc=10 -> 15");
        TEST_ASSERT(apdu_get_length(&ap, APDU_PROTO_T0) == 15, "expected 15");
        TEST_PASS();
    }

    /* Case 3 Ext: lc=10 */
    {
        u8 data10[10] = {1,2,3,4,5,6,7,8,9,10};
        setup_apdu(&ap, APDU_CASE_3_EXT, 0, 0, 0, 0, 10, data10, 0);
        TEST_START("Case3Ext T0 lc=10 -> 15");
        TEST_ASSERT(apdu_get_length(&ap, APDU_PROTO_T0) == 15, "expected 15");
        TEST_PASS();

        TEST_START("Case3Ext T1 lc=10 -> 17");
        TEST_ASSERT(apdu_get_length(&ap, APDU_PROTO_T1) == 17, "expected 17");
        TEST_PASS();
    }

    /* Case 4 Short: lc=5 */
    {
        u8 data5[5] = {0x10,0x20,0x30,0x40,0x50};
        setup_apdu(&ap, APDU_CASE_4_SHORT, 0, 0, 0, 0, 5, data5, 256);
        TEST_START("Case4Short lc=5 T0 -> 10");
        TEST_ASSERT(apdu_get_length(&ap, APDU_PROTO_T0) == 10, "expected 10");
        TEST_PASS();

        TEST_START("Case4Short lc=5 T1 -> 11");
        TEST_ASSERT(apdu_get_length(&ap, APDU_PROTO_T1) == 11, "expected 11");
        TEST_PASS();
    }

    /* Case 4 Ext: lc=5 */
    {
        u8 data5[5] = {0x10,0x20,0x30,0x40,0x50};
        setup_apdu(&ap, APDU_CASE_4_EXT, 0, 0, 0, 0, 5, data5, 256);
        TEST_START("Case4Ext lc=5 T0 -> 10");
        TEST_ASSERT(apdu_get_length(&ap, APDU_PROTO_T0) == 10, "expected 10");
        TEST_PASS();

        TEST_START("Case4Ext lc=5 T1 -> 14");
        TEST_ASSERT(apdu_get_length(&ap, APDU_PROTO_T1) == 14, "expected 14");
        TEST_PASS();
    }

    /* Invalid cse */
    setup_apdu(&ap, 0, 0, 0, 0, 0, 0, NULL, 0);
    TEST_START("Invalid cse -> 0");
    TEST_ASSERT(apdu_get_length(&ap, APDU_PROTO_T0) == 0, "expected 0");
    TEST_PASS();

    setup_apdu(&ap, 0x99, 0, 0, 0, 0, 0, NULL, 0);
    TEST_START("Unknown cse 0x99 -> 0");
    TEST_ASSERT(apdu_get_length(&ap, APDU_PROTO_T1) == 0, "expected 0");
    TEST_PASS();
}

/* ================ Test: apdu_encode ================ */

static void test_encode(void)
{
    printf("[apdu_encode]\n");

    u8 out[32];
    int rc;

    /* Case 1 */
    {
        apdu_t ap;
        u8 expected_t0[] = {0x00, 0xA4, 0x00, 0x00, 0x00};
        u8 expected_t1[] = {0x00, 0xA4, 0x00, 0x00};
        setup_apdu(&ap, APDU_CASE_1, 0x00, 0xA4, 0x00, 0x00, 0, NULL, 0);

        TEST_START("Case1 T0 encode");
        memset(out, 0xAA, sizeof(out));
        rc = apdu_encode(&ap, APDU_PROTO_T0, out, sizeof(out));
        TEST_ASSERT(rc == APDU_SUCCESS, "expected SUCCESS");
        TEST_ASSERT(u8cmp(out, expected_t0, 5) == 0, "bytes mismatch");
        TEST_PASS();

        TEST_START("Case1 T1 encode");
        memset(out, 0xAA, sizeof(out));
        rc = apdu_encode(&ap, APDU_PROTO_T1, out, sizeof(out));
        TEST_ASSERT(rc == APDU_SUCCESS, "expected SUCCESS");
        TEST_ASSERT(u8cmp(out, expected_t1, 4) == 0, "bytes mismatch");
        TEST_PASS();
    }

    /* Case 2 Short: Le=255 */
    {
        apdu_t ap;
        u8 expected[] = {0x00, 0xC0, 0x00, 0x00, 0xFF};
        setup_apdu(&ap, APDU_CASE_2_SHORT, 0x00, 0xC0, 0x00, 0x00, 0, NULL, 255);

        TEST_START("Case2Short Le=255 encode");
        memset(out, 0xAA, sizeof(out));
        rc = apdu_encode(&ap, APDU_PROTO_T0, out, sizeof(out));
        TEST_ASSERT(rc == APDU_SUCCESS, "expected SUCCESS");
        TEST_ASSERT(u8cmp(out, expected, 5) == 0, "bytes mismatch");
        TEST_PASS();
    }

    /* Case 2 Ext: Le=0x0100 */
    {
        apdu_t ap;
        u8 expected_t0[] = {0x00, 0xC0, 0x00, 0x00, 0x00}; /* T0: Le=0 written as 0x00 */
        u8 expected_t1[] = {0x00, 0xC0, 0x00, 0x00, 0x00, 0x01, 0x00};
        setup_apdu(&ap, APDU_CASE_2_EXT, 0x00, 0xC0, 0x00, 0x00, 0, NULL, 0x0100);

        TEST_START("Case2Ext T0 encode");
        memset(out, 0xAA, sizeof(out));
        rc = apdu_encode(&ap, APDU_PROTO_T0, out, sizeof(out));
        TEST_ASSERT(rc == APDU_SUCCESS, "expected SUCCESS");
        /* T0: case2_ext encoded as short: just the low byte of le */
        TEST_ASSERT(u8cmp(out, expected_t0, 5) == 0, "bytes mismatch");
        TEST_PASS();

        TEST_START("Case2Ext T1 encode");
        memset(out, 0xAA, sizeof(out));
        rc = apdu_encode(&ap, APDU_PROTO_T1, out, sizeof(out));
        TEST_ASSERT(rc == APDU_SUCCESS, "expected SUCCESS");
        TEST_ASSERT(u8cmp(out, expected_t1, 7) == 0, "bytes mismatch");
        TEST_PASS();
    }

    /* Case 3 Short: lc=3, data={0x11,0x22,0x33} */
    {
        apdu_t ap;
        u8 data[] = {0x11, 0x22, 0x33};
        u8 expected[] = {0x00, 0xDA, 0x00, 0x00, 0x03, 0x11, 0x22, 0x33};
        setup_apdu(&ap, APDU_CASE_3_SHORT, 0x00, 0xDA, 0x00, 0x00, 3, data, 0);

        TEST_START("Case3Short lc=3 encode");
        memset(out, 0xAA, sizeof(out));
        rc = apdu_encode(&ap, APDU_PROTO_T0, out, sizeof(out));
        TEST_ASSERT(rc == APDU_SUCCESS, "expected SUCCESS");
        TEST_ASSERT(out[4] == 0x03, "Lc mismatch");
        TEST_ASSERT(u8cmp(out, expected, 8) == 0, "bytes mismatch");
        TEST_PASS();
    }

    /* Case 3 Ext: lc=3, data={0x11,0x22,0x33} */
    {
        apdu_t ap;
        u8 data[] = {0x11, 0x22, 0x33};
        u8 expected_t0[] = {0x00, 0xDA, 0x00, 0x00, 0x03, 0x11, 0x22, 0x33};
        u8 expected_t1[] = {0x00, 0xDA, 0x00, 0x00, 0x00, 0x00, 0x03, 0x11, 0x22, 0x33};
        setup_apdu(&ap, APDU_CASE_3_EXT, 0x00, 0xDA, 0x00, 0x00, 3, data, 0);

        TEST_START("Case3Ext T0 encode");
        memset(out, 0xAA, sizeof(out));
        rc = apdu_encode(&ap, APDU_PROTO_T0, out, sizeof(out));
        TEST_ASSERT(rc == APDU_SUCCESS, "expected SUCCESS");
        TEST_ASSERT(u8cmp(out, expected_t0, 8) == 0, "bytes mismatch");
        TEST_PASS();

        TEST_START("Case3Ext T1 encode");
        memset(out, 0xAA, sizeof(out));
        rc = apdu_encode(&ap, APDU_PROTO_T1, out, sizeof(out));
        TEST_ASSERT(rc == APDU_SUCCESS, "expected SUCCESS");
        TEST_ASSERT(u8cmp(out, expected_t1, 10) == 0, "bytes mismatch");
        TEST_PASS();
    }

    /* Case 4 Short: lc=2, data={0xAA,0xBB}, Le=128 */
    {
        apdu_t ap;
        u8 data[] = {0xAA, 0xBB};
        u8 expected_t0[] = {0x00, 0xC0, 0x00, 0x01, 0x02, 0xAA, 0xBB};
        u8 expected_t1[] = {0x00, 0xC0, 0x00, 0x01, 0x02, 0xAA, 0xBB, 0x80};
        setup_apdu(&ap, APDU_CASE_4_SHORT, 0x00, 0xC0, 0x00, 0x01, 2, data, 128);

        TEST_START("Case4Short lc=2 Le=128 T0 encode");
        memset(out, 0xAA, sizeof(out));
        rc = apdu_encode(&ap, APDU_PROTO_T0, out, sizeof(out));
        TEST_ASSERT(rc == APDU_SUCCESS, "expected SUCCESS");
        TEST_ASSERT(u8cmp(out, expected_t0, 7) == 0, "bytes mismatch");
        TEST_PASS();

        TEST_START("Case4Short lc=2 Le=128 T1 encode");
        memset(out, 0xAA, sizeof(out));
        rc = apdu_encode(&ap, APDU_PROTO_T1, out, sizeof(out));
        TEST_ASSERT(rc == APDU_SUCCESS, "expected SUCCESS");
        TEST_ASSERT(u8cmp(out, expected_t1, 8) == 0, "bytes mismatch");
        TEST_PASS();
    }

    /* Case 4 Ext: lc=2, data={0xAA,0xBB}, Le=0x0100 */
    {
        apdu_t ap;
        u8 data[] = {0xAA, 0xBB};
        u8 expected_t0[] = {0x00, 0xC0, 0x00, 0x01, 0x02, 0xAA, 0xBB};
        u8 expected_t1[] = {0x00, 0xC0, 0x00, 0x01, 0x00, 0x00, 0x02, 0xAA, 0xBB, 0x01, 0x00};
        setup_apdu(&ap, APDU_CASE_4_EXT, 0x00, 0xC0, 0x00, 0x01, 2, data, 0x0100);

        TEST_START("Case4Ext lc=2 Le=256 T0 encode");
        memset(out, 0xAA, sizeof(out));
        rc = apdu_encode(&ap, APDU_PROTO_T0, out, sizeof(out));
        TEST_ASSERT(rc == APDU_SUCCESS, "expected SUCCESS");
        TEST_ASSERT(u8cmp(out, expected_t0, 7) == 0, "bytes mismatch");
        TEST_PASS();

        TEST_START("Case4Ext lc=2 Le=256 T1 encode");
        memset(out, 0xAA, sizeof(out));
        rc = apdu_encode(&ap, APDU_PROTO_T1, out, sizeof(out));
        TEST_ASSERT(rc == APDU_SUCCESS, "expected SUCCESS");
        TEST_ASSERT(u8cmp(out, expected_t1, 11) == 0, "bytes mismatch");
        TEST_PASS();
    }

    /* Buffer too small */
    {
        apdu_t ap;
        setup_apdu(&ap, APDU_CASE_1, 0, 0, 0, 0, 0, NULL, 0);

        TEST_START("Buffer too small (T0 Case1 -> need 5, give 4)");
        memset(out, 0xAA, sizeof(out));
        rc = apdu_encode(&ap, APDU_PROTO_T0, out, 4);
        TEST_ASSERT(rc == APDU_ERROR_INVALID_ARGUMENTS, "expected INVALID_ARGUMENTS");
        TEST_PASS();
    }

    /* NULL out */
    {
        apdu_t ap;
        setup_apdu(&ap, APDU_CASE_1, 0, 0, 0, 0, 0, NULL, 0);

        TEST_START("NULL out buffer");
        rc = apdu_encode(&ap, APDU_PROTO_T0, NULL, 10);
        TEST_ASSERT(rc == APDU_ERROR_INVALID_ARGUMENTS, "expected INVALID_ARGUMENTS");
        TEST_PASS();
    }
}

/* ================ Test: apdu_alloc_and_encode ================ */

static void test_alloc_and_encode(void)
{
    printf("[apdu_alloc_and_encode]\n");

    u8 *buf = NULL;
    size_t len = 0;
    int rc;

    /* Normal allocation: Case3Short lc=3 */
    {
        apdu_t ap;
        u8 data[] = {0x11, 0x22, 0x33};
        u8 expected[] = {0x00, 0xDA, 0x00, 0x00, 0x03, 0x11, 0x22, 0x33};
        setup_apdu(&ap, APDU_CASE_3_SHORT, 0x00, 0xDA, 0x00, 0x00, 3, data, 0);

        TEST_START("Alloc and encode Case3Short");
        buf = NULL;
        len = 0;
        rc = apdu_alloc_and_encode(&ap, &buf, &len, APDU_PROTO_T0);
        TEST_ASSERT(rc == APDU_SUCCESS, "expected SUCCESS");
        TEST_ASSERT(buf != NULL, "buf should not be NULL");
        TEST_ASSERT(len == 8, "expected length 8");
        TEST_ASSERT(u8cmp(buf, expected, 8) == 0, "bytes mismatch");
        free(buf);
        TEST_PASS();
    }

    /* Normal allocation: Case4Ext T1 */
    {
        apdu_t ap;
        u8 data[] = {0xAA, 0xBB};
        u8 expected[] = {0x00, 0xC0, 0x00, 0x01, 0x00, 0x00, 0x02, 0xAA, 0xBB, 0x01, 0x00};
        setup_apdu(&ap, APDU_CASE_4_EXT, 0x00, 0xC0, 0x00, 0x01, 2, data, 0x0100);

        TEST_START("Alloc and encode Case4Ext T1");
        buf = NULL;
        len = 0;
        rc = apdu_alloc_and_encode(&ap, &buf, &len, APDU_PROTO_T1);
        TEST_ASSERT(rc == APDU_SUCCESS, "expected SUCCESS");
        TEST_ASSERT(buf != NULL, "buf should not be NULL");
        TEST_ASSERT(len == 11, "expected length 11");
        TEST_ASSERT(u8cmp(buf, expected, 11) == 0, "bytes mismatch");
        free(buf);
        TEST_PASS();
    }

    /* NULL apdu */
    TEST_START("NULL apdu returns INVALID_ARGUMENTS");
    rc = apdu_alloc_and_encode(NULL, &buf, &len, APDU_PROTO_T0);
    TEST_ASSERT(rc == APDU_ERROR_INVALID_ARGUMENTS, "expected INVALID_ARGUMENTS");
    TEST_PASS();

    /* NULL buf */
    {
        apdu_t ap;
        setup_apdu(&ap, APDU_CASE_1, 0, 0, 0, 0, 0, NULL, 0);
        TEST_START("NULL buf returns INVALID_ARGUMENTS");
        rc = apdu_alloc_and_encode(&ap, NULL, &len, APDU_PROTO_T0);
        TEST_ASSERT(rc == APDU_ERROR_INVALID_ARGUMENTS, "expected INVALID_ARGUMENTS");
        TEST_PASS();
    }

    /* NULL len */
    {
        apdu_t ap;
        setup_apdu(&ap, APDU_CASE_1, 0, 0, 0, 0, 0, NULL, 0);
        TEST_START("NULL len returns INVALID_ARGUMENTS");
        rc = apdu_alloc_and_encode(&ap, &buf, NULL, APDU_PROTO_T0);
        TEST_ASSERT(rc == APDU_ERROR_INVALID_ARGUMENTS, "expected INVALID_ARGUMENTS");
        TEST_PASS();
    }

    /* Invalid cse -> INTERNAL error */
    {
        apdu_t ap;
        setup_apdu(&ap, 0, 0, 0, 0, 0, 0, NULL, 0);
        TEST_START("Invalid cse returns INTERNAL");
        rc = apdu_alloc_and_encode(&ap, &buf, &len, APDU_PROTO_T0);
        TEST_ASSERT(rc == APDU_ERROR_INTERNAL, "expected INTERNAL");
        TEST_PASS();
    }
}

/* ================ Test: apdu_decode ================ */

static void test_decode(void)
{
    printf("[apdu_decode]\n");

    apdu_t ap;
    int rc;

    /* Case 1: 4 bytes, no extra */
    {
        u8 buf[] = {0x00, 0xA4, 0x00, 0x00};
        TEST_START("Decode Case1");
        rc = apdu_decode(buf, 4, &ap);
        TEST_ASSERT(rc == APDU_SUCCESS, "expected SUCCESS");
        TEST_ASSERT(ap.cse == APDU_CASE_1, "cse should be CASE_1");
        TEST_ASSERT(ap.cla == 0x00, "cla mismatch");
        TEST_ASSERT(ap.ins == 0xA4, "ins mismatch");
        TEST_ASSERT(ap.p1 == 0x00, "p1 mismatch");
        TEST_ASSERT(ap.p2 == 0x00, "p2 mismatch");
        TEST_ASSERT(ap.lc == 0, "lc should be 0");
        TEST_ASSERT(ap.le == 0, "le should be 0");
        TEST_PASS();
    }

    /* Case 2 Short: Le=0 -> decoded as 256 */
    {
        u8 buf[] = {0x00, 0xC0, 0x00, 0x00, 0x00};
        TEST_START("Decode Case2Short Le=0 -> 256");
        rc = apdu_decode(buf, 5, &ap);
        TEST_ASSERT(rc == APDU_SUCCESS, "expected SUCCESS");
        TEST_ASSERT(ap.cse == APDU_CASE_2_SHORT, "cse should be CASE_2_SHORT");
        TEST_ASSERT(ap.le == 256, "le should be 256 (0xFF+1)");
        TEST_PASS();
    }

    /* Case 2 Short: Le=0x80 */
    {
        u8 buf[] = {0x00, 0xC0, 0x00, 0x00, 0x80};
        TEST_START("Decode Case2Short Le=128");
        rc = apdu_decode(buf, 5, &ap);
        TEST_ASSERT(rc == APDU_SUCCESS, "expected SUCCESS");
        TEST_ASSERT(ap.cse == APDU_CASE_2_SHORT, "cse should be CASE_2_SHORT");
        TEST_ASSERT(ap.le == 128, "le should be 128");
        TEST_PASS();
    }

    /* Case 2 Ext: 00 Le_hi Le_lo */
    {
        u8 buf[] = {0x00, 0xC0, 0x00, 0x00, 0x00, 0x01, 0x00};
        TEST_START("Decode Case2Ext Le=256");
        rc = apdu_decode(buf, 7, &ap);
        TEST_ASSERT(rc == APDU_SUCCESS, "expected SUCCESS");
        TEST_ASSERT(ap.cse == APDU_CASE_2_EXT, "cse should be CASE_2_EXT");
        TEST_ASSERT(ap.le == 256, "le should be 256");
        TEST_PASS();
    }

    /* Case 2 Ext: Le=0 -> decoded as 65536 */
    {
        u8 buf[] = {0x00, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00};
        TEST_START("Decode Case2Ext Le=0 -> 65536");
        rc = apdu_decode(buf, 7, &ap);
        TEST_ASSERT(rc == APDU_SUCCESS, "expected SUCCESS");
        TEST_ASSERT(ap.cse == APDU_CASE_2_EXT, "cse should be CASE_2_EXT");
        TEST_ASSERT(ap.le == 65536, "le should be 65536 (0xFFFF+1)");
        TEST_PASS();
    }

    /* Case 3 Short: Lc + data */
    {
        u8 buf[] = {0x00, 0xDA, 0x00, 0x00, 0x03, 0x11, 0x22, 0x33};
        TEST_START("Decode Case3Short lc=3");
        rc = apdu_decode(buf, 8, &ap);
        TEST_ASSERT(rc == APDU_SUCCESS, "expected SUCCESS");
        TEST_ASSERT(ap.cse == APDU_CASE_3_SHORT, "cse should be CASE_3_SHORT");
        TEST_ASSERT(ap.cla == 0x00, "cla mismatch");
        TEST_ASSERT(ap.ins == 0xDA, "ins mismatch");
        TEST_ASSERT(ap.lc == 3, "lc should be 3");
        TEST_ASSERT(ap.datalen == 3, "datalen should be 3");
        TEST_ASSERT(ap.data != NULL, "data should not be NULL");
        TEST_ASSERT(ap.data[0] == 0x11 && ap.data[1] == 0x22 && ap.data[2] == 0x33,
                    "data content mismatch");
        /* data should point into original buf */
        TEST_ASSERT(ap.data == buf + 5, "data should point into input buffer");
        TEST_PASS();
    }

    /* Case 3 Ext T1: 00 Lc_hi Lc_lo + data */
    {
        u8 buf[] = {0x00, 0xDA, 0x00, 0x00, 0x00, 0x00, 0x03, 0x11, 0x22, 0x33};
        TEST_START("Decode Case3Ext lc=3");
        rc = apdu_decode(buf, 10, &ap);
        TEST_ASSERT(rc == APDU_SUCCESS, "expected SUCCESS");
        TEST_ASSERT(ap.cse == APDU_CASE_3_EXT, "cse should be CASE_3_EXT");
        TEST_ASSERT(ap.lc == 3, "lc should be 3");
        TEST_ASSERT(ap.datalen == 3, "datalen should be 3");
        TEST_ASSERT(ap.data == buf + 7, "data should point into input buffer");
        TEST_PASS();
    }

    /* Case 4 Short T1: Lc data Le */
    {
        u8 buf[] = {0x00, 0xC0, 0x00, 0x01, 0x02, 0xAA, 0xBB, 0x80};
        TEST_START("Decode Case4Short lc=2 Le=128");
        rc = apdu_decode(buf, 8, &ap);
        TEST_ASSERT(rc == APDU_SUCCESS, "expected SUCCESS");
        TEST_ASSERT(ap.cse == APDU_CASE_4_SHORT, "cse should be CASE_4_SHORT");
        TEST_ASSERT(ap.lc == 2, "lc should be 2");
        TEST_ASSERT(ap.le == 128, "le should be 128");
        TEST_ASSERT(ap.data == buf + 5, "data should point into input buffer");
        TEST_PASS();
    }

    /* Case 4 Ext T1: 00 Lc_hi Lc_lo data Le_hi Le_lo */
    {
        u8 buf[] = {0x00, 0xC0, 0x00, 0x01, 0x00, 0x00, 0x02, 0xAA, 0xBB, 0x01, 0x00};
        TEST_START("Decode Case4Ext lc=2 Le=256");
        rc = apdu_decode(buf, 11, &ap);
        TEST_ASSERT(rc == APDU_SUCCESS, "expected SUCCESS");
        TEST_ASSERT(ap.cse == APDU_CASE_4_EXT, "cse should be CASE_4_EXT");
        TEST_ASSERT(ap.lc == 2, "lc should be 2");
        TEST_ASSERT(ap.le == 256, "le should be 256");
        TEST_ASSERT(ap.data == buf + 7, "data should point into input buffer");
        TEST_PASS();
    }

    /* Input < 4 bytes */
    {
        u8 buf[] = {0x00, 0xA4, 0x00};
        TEST_START("Input too short (3 bytes)");
        rc = apdu_decode(buf, 3, &ap);
        TEST_ASSERT(rc == APDU_ERROR_INVALID_DATA, "expected INVALID_DATA");
        TEST_PASS();

        TEST_START("Input empty (0 bytes)");
        rc = apdu_decode(buf, 0, &ap);
        TEST_ASSERT(rc == APDU_ERROR_INVALID_DATA, "expected INVALID_DATA");
        TEST_PASS();
    }

    /* NULL arguments */
    {
        u8 buf[] = {0x00, 0xA4, 0x00, 0x00};
        TEST_START("NULL buf -> INVALID_ARGUMENTS");
        rc = apdu_decode(NULL, 4, &ap);
        TEST_ASSERT(rc == APDU_ERROR_INVALID_ARGUMENTS, "expected INVALID_ARGUMENTS");
        TEST_PASS();

        TEST_START("NULL apdu -> INVALID_ARGUMENTS");
        rc = apdu_decode(buf, 4, NULL);
        TEST_ASSERT(rc == APDU_ERROR_INVALID_ARGUMENTS, "expected INVALID_ARGUMENTS");
        TEST_PASS();
    }

    /* Invalid data: len < lc */
    {
        u8 buf[] = {0x00, 0xDA, 0x00, 0x00, 0x05, 0x11, 0x22}; /* lc=5 but only 2 data bytes */
        TEST_START("len < lc -> INVALID_DATA");
        rc = apdu_decode(buf, 7, &ap);
        TEST_ASSERT(rc == APDU_ERROR_INVALID_DATA, "expected INVALID_DATA");
        TEST_PASS();
    }

    /* Trailing garbage bytes: {0x00,0xA4,0x00,0x00,0xFF} is actually a valid
     * Case 2 Short APDU with Le=255, NOT garbage. Use a truly trailing
     * case: extra byte after an already-complete short APDU.
     *
     * {0x00,0xA4,0x00,0x00,0xFF,0xFF} = header(4) + Le_byte(1) + garbage(1).
     * After header, len=2. *p=0xFF !=0 -> short path, len=2 !=1 -> lc=0xFF,
     * then len=1 < lc=0xFF -> returns INVALID_DATA.
     *
     * But {0x00,0xA4,0x00,0x00,0xFF} (5 bytes) is simply a valid
     * Case 2 Short with Le=255.
     */
    {
        u8 buf[] = {0x00, 0xA4, 0x00, 0x00, 0xFF};
        TEST_START("Trailing garbage? No - valid Case2Short Le=255");
        rc = apdu_decode(buf, 5, &ap);
        TEST_ASSERT(rc == APDU_SUCCESS, "expected SUCCESS");
        TEST_ASSERT(ap.cse == APDU_CASE_2_SHORT, "cse should be CASE_2_SHORT");
        TEST_ASSERT(ap.le == 255, "le should be 255");
        TEST_PASS();
    }

    /* Truly trailing garbage: extra bytes that make len > expected */
    {
        u8 buf[] = {0x00, 0xA4, 0x00, 0x00, 0xFF, 0xFF};
        TEST_START("Trailing garbage -> INVALID_DATA");
        rc = apdu_decode(buf, 6, &ap);
        TEST_ASSERT(rc == APDU_ERROR_INVALID_DATA, "expected INVALID_DATA");
        TEST_PASS();
    }
}

/* ================ Test: apdu_set_response ================ */

static void test_set_response(void)
{
    printf("[apdu_set_response]\n");

    apdu_t ap;
    int rc;

    /* Normal: buf=6 bytes, last 2 are SW, set resplen=3 to copy first 3 bytes */
    {
        u8 resp_buf[10];
        u8 buf[] = {'A', 'B', 'C', 'D', 'E', 'F'}; /* 6 bytes */
        memset(&ap, 0, sizeof(ap));
        ap.resp    = resp_buf;
        ap.resplen = 3; /* buffer capacity = 3 */

        TEST_START("Set response: 6 bytes, resplen=3 -> copies 3 bytes, SW=0x45,0x46");
        rc = apdu_set_response(&ap, buf, 6);
        TEST_ASSERT(rc == APDU_SUCCESS, "expected SUCCESS");
        TEST_ASSERT(ap.sw1 == (unsigned)'E', "sw1 should be 'E' (0x45)");
        TEST_ASSERT(ap.sw2 == (unsigned)'F', "sw2 should be 'F' (0x46)");
        TEST_ASSERT(ap.resplen == 3, "resplen should be 3");
        TEST_ASSERT(resp_buf[0] == 'A' && resp_buf[1] == 'B' && resp_buf[2] == 'C',
                    "resp data mismatch");
        TEST_PASS();
    }

    /* Normal: resplen larger than data, copies all data */
    {
        u8 resp_buf[10];
        u8 buf[] = {'X', 'Y', 'Z', 0x90, 0x00}; /* 5 bytes, SW=0x9000 */
        memset(&ap, 0, sizeof(ap));
        ap.resp    = resp_buf;
        ap.resplen = 10;

        TEST_START("Set response: resplen bigger than data, copies all data");
        rc = apdu_set_response(&ap, buf, 5);
        TEST_ASSERT(rc == APDU_SUCCESS, "expected SUCCESS");
        TEST_ASSERT(ap.sw1 == 0x90, "sw1 should be 0x90");
        TEST_ASSERT(ap.sw2 == 0x00, "sw2 should be 0x00");
        TEST_ASSERT(ap.resplen == 3, "resplen should be 3 (all data)");
        TEST_ASSERT(resp_buf[0] == 'X' && resp_buf[1] == 'Y' && resp_buf[2] == 'Z',
                    "resp data mismatch");
        TEST_PASS();
    }

    /* Normal: only SW, no data (len=2) */
    {
        u8 resp_buf[10];
        u8 buf[] = {0x90, 0x00};
        memset(&ap, 0, sizeof(ap));
        ap.resp    = resp_buf;
        ap.resplen = 10;

        TEST_START("Set response: only SW (len=2), no data");
        rc = apdu_set_response(&ap, buf, 2);
        TEST_ASSERT(rc == APDU_SUCCESS, "expected SUCCESS");
        TEST_ASSERT(ap.sw1 == 0x90, "sw1 should be 0x90");
        TEST_ASSERT(ap.sw2 == 0x00, "sw2 should be 0x00");
        TEST_ASSERT(ap.resplen == 0, "resplen should be 0");
        TEST_PASS();
    }

    /* Normal: resp=NULL, resplen>0 -> no copy, but SW set and resplen updated */
    {
        u8 buf[] = {'A', 'B', 'C', 0x90, 0x00};
        memset(&ap, 0, sizeof(ap));
        ap.resp    = NULL;
        ap.resplen = 10;

        TEST_START("Set response: resp=NULL, resplen updated but no copy");
        rc = apdu_set_response(&ap, buf, 5);
        TEST_ASSERT(rc == APDU_SUCCESS, "expected SUCCESS");
        TEST_ASSERT(ap.sw1 == 0x90, "sw1 should be 0x90");
        TEST_ASSERT(ap.sw2 == 0x00, "sw2 should be 0x00");
        TEST_ASSERT(ap.resplen == 3, "resplen should be 3");
        TEST_PASS();
    }

    /* Error: len < 2 */
    {
        u8 buf[] = {0x90};
        memset(&ap, 0, sizeof(ap));

        TEST_START("Set response: len=1 -> INTERNAL error");
        rc = apdu_set_response(&ap, buf, 1);
        TEST_ASSERT(rc == APDU_ERROR_INTERNAL, "expected INTERNAL");
        TEST_PASS();

        TEST_START("Set response: len=0 -> INTERNAL error");
        rc = apdu_set_response(&ap, buf, 0);
        TEST_ASSERT(rc == APDU_ERROR_INTERNAL, "expected INTERNAL");
        TEST_PASS();
    }
}

/* ================ Main ================ */

int main(void)
{
    printf("========================================\n");
    printf("  libapdu API Test Suite\n");
    printf("========================================\n\n");

    test_get_length();
    printf("\n");
    test_encode();
    printf("\n");
    test_alloc_and_encode();
    printf("\n");
    test_decode();
    printf("\n");
    test_set_response();

    printf("\n========================================\n");
    printf("  Results: %d passed, %d failed\n", tests_passed, tests_failed);
    printf("========================================\n");

    return tests_failed != 0 ? 1 : 0;
}

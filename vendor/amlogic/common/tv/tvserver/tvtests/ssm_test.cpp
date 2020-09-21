/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: c++ file
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <android/log.h>

#include "ssm_api.h"

#define LOG_TAG "ssm_test"
#include "CTvLog.h"

// The follow is R/W test struct declare.
// The size of it is 11 when it aligned with 1 Byte
// and is 16 when it aligned with 4 Bytes.
// You should use the 1 Byte aligned mode when R/W ssm by using struct.

#if 1   // memory aligned with 1 Bytes
typedef struct tagS_TEST_STRUCT {
    char tmp_ch0;
    char tmp_ch1;
    int tmp_val0;
    unsigned char tmp_ch2;
    unsigned char tmp_ch3;
    unsigned char tmp_ch4;
    short tmp_short0;
} __attribute__((packed)) S_TEST_STRUCT;
#else   // memory aligned with 4 Bytes
typedef struct tagS_TEST_STRUCT {
    char tmp_ch0;
    char tmp_ch1;
    int tmp_val0;
    unsigned char tmp_ch2;
    unsigned char tmp_ch3;
    unsigned char tmp_ch4;
    short tmp_short0;
} S_TEST_STRUCT;
#endif

static void TestRWOneByte(int tmp_rw_offset, int tmp_w_val, unsigned char tmp_w_ch);
static void TestRWNBytes(int tmp_rw_offset);
static void TestRWOneStruct(int tmp_rw_offset);

int main()
{
    TestRWOneByte(0, 1, -1);
    TestRWOneByte(1, 2, -2);
    TestRWOneByte(30, 3, -3);
    TestRWOneByte(31, -1, 1);
    TestRWOneByte(32, -2, 2);
    TestRWOneByte(33, -3, 3);

    TestRWNBytes(31);

    TestRWOneStruct(0);
}

static void TestRWOneByte(int tmp_rw_offset, int tmp_w_val, unsigned char tmp_w_ch)
{
    int tmp_r_val = 0;
    unsigned char tmp_r_ch = 0;

    LOGD("\n\n");
    LOGD("**************Test R/W One Byte **************\n\n");

    LOGD("tmp_rw_offset = %d.\n", tmp_rw_offset);

    SSMWriteOneByte(tmp_rw_offset, tmp_w_ch);
    SSMReadOneByte(tmp_rw_offset, &tmp_r_ch);
    LOGD("tmp_w_ch = %d, tmp_r_ch = %d.\n", tmp_w_ch, tmp_r_ch);

    SSMWriteOneByte(tmp_rw_offset, tmp_w_val);
    SSMReadOneByte(tmp_rw_offset, &tmp_r_val);
    LOGD("tmp_w_val = %d, tmp_r_val = %d.\n", tmp_w_val, tmp_r_val);
}

static void TestRWNBytes(int tmp_rw_offset)
{
    int i = 0, tmp_op_buf_size = 0;
    int device_size = 0, tmp_w_page_size = 0, tmp_r_page_size = 0;
    int *tmp_op_int_w_buf = NULL, *tmp_op_int_r_buf = NULL;
    unsigned char *tmp_op_char_w_buf = NULL, *tmp_op_char_r_buf = NULL;

    LOGD("\n\n");
    LOGD("**************Test R/W N Bytes **************\n\n");

    SSMGetDeviceTotalSize(&device_size);
    SSMGetDeviceWritePageSize(&tmp_w_page_size);
    SSMGetDeviceReadPageSize(&tmp_r_page_size);

    if (tmp_w_page_size < tmp_r_page_size) {
        tmp_op_buf_size = tmp_w_page_size * 2 / 3;
    } else if (tmp_r_page_size < tmp_w_page_size) {
        tmp_op_buf_size = tmp_r_page_size * 2 / 3;
    } else {
        tmp_op_buf_size = tmp_w_page_size;
    }

    if (tmp_op_buf_size > device_size) {
        tmp_op_buf_size = device_size;
    }

    if (tmp_op_buf_size > 0) {
        LOGD("tmp_rw_offset = %d.\n", tmp_rw_offset);

        tmp_op_char_w_buf = new unsigned char[tmp_op_buf_size];
        if (tmp_op_char_w_buf != NULL) {
            tmp_op_char_r_buf = new unsigned char[tmp_op_buf_size];
            if (tmp_op_char_r_buf != NULL) {
                for (i = 0; i < tmp_op_buf_size; i++) {
                    tmp_op_char_w_buf[i] = (tmp_op_buf_size / 2) - i;
                    LOGD("tmp_op_char_w_buf[%d] = %d\n", i, tmp_op_char_w_buf[i]);
                }
                SSMWriteNTypes(tmp_rw_offset, tmp_op_buf_size, tmp_op_char_w_buf);

                for (i = 0; i < tmp_op_buf_size; i++) {
                    tmp_op_char_r_buf[i] = 0;
                }
                SSMReadNTypes(tmp_rw_offset, tmp_op_buf_size, tmp_op_char_r_buf);

                for (i = 0; i < tmp_op_buf_size; i++) {
                    LOGD("tmp_op_char_r_buf[%d] = %d\n", i, tmp_op_char_r_buf[i]);
                }

                delete tmp_op_char_r_buf;
                tmp_op_char_r_buf = NULL;
            }

            delete tmp_op_char_w_buf;
            tmp_op_char_w_buf = NULL;
        }

        tmp_op_int_w_buf = new int[tmp_op_buf_size];
        if (tmp_op_int_w_buf != NULL) {
            tmp_op_int_r_buf = new int[tmp_op_buf_size];
            if (tmp_op_int_r_buf != NULL) {
                for (i = 0; i < tmp_op_buf_size; i++) {
                    tmp_op_int_w_buf[i] = (tmp_op_buf_size / 2) - i;
                    LOGD("tmp_op_int_w_buf[%d] = %d\n", i, tmp_op_int_w_buf[i]);
                }
                SSMWriteNTypes(tmp_rw_offset, tmp_op_buf_size, tmp_op_int_w_buf);

                for (i = 0; i < tmp_op_buf_size; i++) {
                    tmp_op_int_r_buf[i] = 0;
                }
                SSMReadNTypes(tmp_rw_offset, tmp_op_buf_size, tmp_op_int_r_buf);

                for (i = 0; i < tmp_op_buf_size; i++) {
                    LOGD("tmp_op_int_r_buf[%d] = %d\n", i, tmp_op_int_r_buf[i]);
                }

                delete tmp_op_int_r_buf;
                tmp_op_int_r_buf = NULL;
            }

            delete tmp_op_int_w_buf;
            tmp_op_int_w_buf = NULL;
        }
    }
}

static void TestRWOneStruct(int tmp_rw_offset)
{
    S_TEST_STRUCT TestWriteStruct, TestReadStruct;

    LOGD("\n\n");
    LOGD("**************Test R/W One Struct **************\n\n");

    LOGD("tmp_rw_offset = %d.\n", tmp_rw_offset);

    TestWriteStruct.tmp_ch0 = -9;
    TestWriteStruct.tmp_ch1 = -8;
    TestWriteStruct.tmp_val0 = 9;
    TestWriteStruct.tmp_ch2 = 255;
    TestWriteStruct.tmp_ch3 = 254;
    TestWriteStruct.tmp_ch4 = 250;
    TestWriteStruct.tmp_short0 = -9;

    SSMWriteNTypes(tmp_rw_offset, sizeof(S_TEST_STRUCT), (unsigned char *) &TestWriteStruct);

    LOGD("\n\n");
    LOGD("write struct length = %d.\n", sizeof(S_TEST_STRUCT));
    LOGD("TestWriteStruct.tmp_ch0 = %d.\n", TestWriteStruct.tmp_ch0);
    LOGD("TestWriteStruct.tmp_ch1 = %d.\n", TestWriteStruct.tmp_ch1);
    LOGD("TestWriteStruct.tmp_val0 = %d.\n", TestWriteStruct.tmp_val0);
    LOGD("TestWriteStruct.tmp_ch2 = %d.\n", TestWriteStruct.tmp_ch2);
    LOGD("TestWriteStruct.tmp_ch3 = %d.\n", TestWriteStruct.tmp_ch3);
    LOGD("TestWriteStruct.tmp_ch4 = %d.\n", TestWriteStruct.tmp_ch4);
    LOGD("TestWriteStruct.tmp_short0 = %d.\n", TestWriteStruct.tmp_short0);

    TestReadStruct.tmp_ch0 = 0;
    TestReadStruct.tmp_ch1 = 0;
    TestReadStruct.tmp_val0 = 0;
    TestReadStruct.tmp_ch2 = 0;
    TestReadStruct.tmp_ch3 = 0;
    TestWriteStruct.tmp_ch4 = 0;
    TestWriteStruct.tmp_short0 = 0;

    SSMReadNTypes(tmp_rw_offset, sizeof(S_TEST_STRUCT), (unsigned char *) &TestReadStruct);

    LOGD("\n\n");
    LOGD("read struct length = %d.\n", sizeof(S_TEST_STRUCT));
    LOGD("TestReadStruct.tmp_ch0 = %d.\n", TestReadStruct.tmp_ch0);
    LOGD("TestReadStruct.tmp_ch1 = %d.\n", TestReadStruct.tmp_ch1);
    LOGD("TestReadStruct.tmp_val0 = %d.\n", TestReadStruct.tmp_val0);
    LOGD("TestReadStruct.tmp_ch2 = %d.\n", TestReadStruct.tmp_ch2);
    LOGD("TestReadStruct.tmp_ch3 = %d.\n", TestReadStruct.tmp_ch3);
    LOGD("TestReadStruct.tmp_ch4 = %d.\n", TestReadStruct.tmp_ch4);
    LOGD("TestReadStruct.tmp_short0 = %d.\n", TestReadStruct.tmp_short0);
}

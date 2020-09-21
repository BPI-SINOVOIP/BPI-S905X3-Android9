/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "AmlKeymaster"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include <log/log.h>

#include "keymaster_ipc.h"
#include "aml_keymaster_ipc.h"

TEEC_Result aml_keymaster_connect(TEEC_Context *c, TEEC_Session *s) {
    TEEC_Result result = TEEC_SUCCESS;
    TEEC_UUID svc_id = TA_KEYMASTER_UUID;
    TEEC_Operation operation;
    uint32_t err_origin;
    struct timespec time;
    uint64_t millis = 0;

    memset(&operation, 0, sizeof(operation));

    /* Initialize Context */
    result = TEEC_InitializeContext(NULL, c);

    if (result != TEEC_SUCCESS) {
        ALOGD("TEEC_InitializeContext failed with error = %x\n", result);
        return result;
    }
    /* Open Session */
    result = TEEC_OpenSession(c, s, &svc_id,
            TEEC_LOGIN_PUBLIC,
            NULL, NULL,
            &err_origin);

    if (result != TEEC_SUCCESS) {
        ALOGD("TEEC_Opensession failed with code 0x%x origin 0x%x",result, err_origin);
        TEEC_FinalizeContext(c);
        return result;
    }

    int res = clock_gettime(CLOCK_BOOTTIME, &time);
    if (res < 0)
        millis = 0;
    else
        millis = (time.tv_sec * 1000) + (time.tv_nsec / 1000 / 1000);

    /* Init TA */
    operation.paramTypes = TEEC_PARAM_TYPES(
            TEEC_VALUE_INPUT, TEEC_NONE,
            TEEC_NONE, TEEC_NONE);

    operation.params[0].value.a = (millis >> 32);
    operation.params[0].value.b = (millis & 0xffffffff);

    result = TEEC_InvokeCommand(s,
            KM_TA_INIT,
            &operation,
            NULL);

	ALOGE("create id: %d, ctx: %p, ctx: %p\n", s->session_id, s->ctx, c);
    return result;
}

TEEC_Result aml_keymaster_call(TEEC_Session *s, uint32_t cmd, void* in, uint32_t in_size, uint8_t* out,
                          uint32_t* out_size) {
    TEEC_Result res = TEEC_SUCCESS;
    TEEC_Operation op;
    uint32_t ret_orig;

    memset(&op, 0, sizeof(op));

    op.params[0].tmpref.buffer = in;
    op.params[0].tmpref.size = in_size;
    op.params[1].tmpref.buffer = out;
    op.params[1].tmpref.size = *out_size;
    op.paramTypes = TEEC_PARAM_TYPES(
            TEEC_MEMREF_TEMP_INPUT,
            TEEC_MEMREF_TEMP_OUTPUT,
            TEEC_VALUE_OUTPUT,
            TEEC_NONE);

	ALOGE("id: %d, ctx: %p, cmd: %d\n", s->session_id, s->ctx, cmd);
    res = TEEC_InvokeCommand(s, cmd, &op, &ret_orig);
    if (res != TEEC_SUCCESS) {
        ALOGE("Invoke cmd: %u failed with res(%x), ret_orig(%x), return(%d)\n",
                cmd, res, ret_orig, op.params[2].value.a);
    } else {
        *out_size = op.params[2].value.b;
    }

    return res;
}

TEEC_Result aml_keymaster_disconnect(TEEC_Context *c, TEEC_Session *s) {
    TEEC_Operation operation;
    TEEC_Result result = TEEC_SUCCESS;

    operation.paramTypes = TEEC_PARAM_TYPES(
            TEEC_NONE, TEEC_NONE,
            TEEC_NONE, TEEC_NONE);

    result = TEEC_InvokeCommand(s,
            KM_TA_TERM,
            &operation,
            NULL);

    TEEC_CloseSession(s);
    TEEC_FinalizeContext(c);

    return result;
}

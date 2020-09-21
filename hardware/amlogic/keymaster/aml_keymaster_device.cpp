/*
 * Copyright 2014 The Android Open Source Project
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

#include <assert.h>
#include <openssl/evp.h>
#include <openssl/x509.h>
#include <openssl/sha.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fstream>
#include <iostream>
#include <inttypes.h>
#include <unistd.h>

#include <algorithm>
#include <type_traits>

#include <hardware/keymaster2.h>
#include <keymaster/authorization_set.h>
#include <log/log.h>
#include <utils/String8.h>

#include "keymaster_ipc.h"
#include "aml_keymaster_device.h"
#include "aml_keymaster_ipc.h"

#ifndef KEYMASTER_TEMP_FAILURE_RETRY
#define KEYMASTER_TEMP_FAILURE_RETRY(exp, retry)            \
	  ({                                       \
	       __typeof__(exp) _rc;                   \
	       int count = 0;                         \
	       do {                                   \
	         _rc = (exp);                         \
	         count ++;                            \
	       } while (_rc != TEEC_SUCCESS && count < retry); \
	       _rc;                                   \
	     })
#endif

const uint32_t RECV_BUF_SIZE = 66 * 1024;
const uint32_t SEND_BUF_SIZE = (66 * 1024 - sizeof(struct keymaster_message) - 16 /* tipc header */);

const size_t kMaximumAttestationChallengeLength = 128;
const size_t kMaximumFinishInputLength = 64 * 1024;

namespace keymaster {

static keymaster_error_t translate_error(TEEC_Result err) {
    switch (err) {
        case TEEC_SUCCESS:
            return KM_ERROR_OK;
        case TEEC_ERROR_ACCESS_DENIED:
            return KM_ERROR_SECURE_HW_ACCESS_DENIED;

        case TEEC_ERROR_CANCEL:
            return KM_ERROR_OPERATION_CANCELLED;

        case TEEC_ERROR_NOT_IMPLEMENTED:
            return KM_ERROR_UNIMPLEMENTED;

        case TEEC_ERROR_OUT_OF_MEMORY:
            return KM_ERROR_MEMORY_ALLOCATION_FAILED;

        case TEEC_ERROR_BUSY:
            return KM_ERROR_SECURE_HW_BUSY;

        case TEEC_ERROR_COMMUNICATION:
            return KM_ERROR_SECURE_HW_COMMUNICATION_FAILED;

        case TEEC_ERROR_SHORT_BUFFER:
            return KM_ERROR_INVALID_INPUT_LENGTH;

        default:
            return KM_ERROR_UNKNOWN_ERROR;
    }
}

AmlKeymasterDevice::AmlKeymasterDevice(const hw_module_t* module) {
    static_assert(std::is_standard_layout<AmlKeymasterDevice>::value,
                  "AmlKeymasterDevice must be standard layout");
    static_assert(offsetof(AmlKeymasterDevice, device_) == 0,
                  "device_ must be the first member of AmlKeymasterDevice");
    static_assert(offsetof(AmlKeymasterDevice, device_.common) == 0,
                  "common must be the first member of keymaster2_device");

    ALOGI("Creating device");
    ALOGD("Device address: %p", this);

    device_ = {};

    device_.common.tag = HARDWARE_DEVICE_TAG;
    device_.common.version = 1;
    device_.common.module = const_cast<hw_module_t*>(module);
    device_.common.close = close_device;

    device_.flags = KEYMASTER_SUPPORTS_EC;

    device_.configure = configure;
    device_.add_rng_entropy = add_rng_entropy;
    device_.generate_key = generate_key;
    device_.get_key_characteristics = get_key_characteristics;
    device_.import_key = import_key;
    device_.export_key = export_key;
    device_.attest_key = attest_key;
    device_.upgrade_key = upgrade_key;
    device_.delete_key = delete_key;
    device_.delete_all_keys = nullptr;
    device_.begin = begin;
    device_.update = update;
    device_.finish = finish;
    device_.abort = abort;

    KM_context.fd = 0;
    KM_session.ctx = NULL;
    KM_session.session_id = 0;


    TEEC_Result rc = KEYMASTER_TEMP_FAILURE_RETRY(aml_keymaster_connect(&KM_context, &KM_session), 100);
    error_ = translate_error(rc);
    if (rc != TEEC_SUCCESS) {
        ALOGE("failed to connect to keymaster (0x%x)", rc);
        error_ = KM_ERROR_SECURE_HW_COMMUNICATION_FAILED;
        return;
    }

    GetVersionRequest version_request;
    GetVersionResponse version_response;
    error_ = Send(KM_GET_VERSION, version_request, &version_response);
    if (error_ == KM_ERROR_INVALID_ARGUMENT || error_ == KM_ERROR_UNIMPLEMENTED) {
        ALOGE("\"Bad parameters\" error on GetVersion call.  Version 0 is not supported.");
        error_ = KM_ERROR_VERSION_MISMATCH;
        return;
    }
    message_version_ = MessageVersion(version_response.major_ver, version_response.minor_ver,
                                      version_response.subminor_ver);
    if (message_version_ < 0) {
        // Can't translate version?  Keymaster implementation must be newer.
        ALOGE("Keymaster version %d.%d.%d not supported.", version_response.major_ver,
              version_response.minor_ver, version_response.subminor_ver);
        error_ = KM_ERROR_VERSION_MISMATCH;
    }
}

AmlKeymasterDevice::~AmlKeymasterDevice() {
   if (KM_session.ctx != NULL)
       aml_keymaster_disconnect(&KM_context, &KM_session);
}

namespace {

// Allocates a new buffer with malloc and copies the contents of |buffer| to it. Caller takes
// ownership of the returned buffer.
uint8_t* DuplicateBuffer(const uint8_t* buffer, size_t size) {
    uint8_t* tmp = reinterpret_cast<uint8_t*>(malloc(size));
    if (tmp) {
        memcpy(tmp, buffer, size);
    }
    return tmp;
}

template <typename RequestType>
void AddClientAndAppData(const keymaster_blob_t* client_id, const keymaster_blob_t* app_data,
                         RequestType* request) {
    request->additional_params.Clear();
    if (client_id && client_id->data_length) {
        request->additional_params.push_back(TAG_APPLICATION_ID, *client_id);
    }
    if (app_data && app_data->data_length) {
        request->additional_params.push_back(TAG_APPLICATION_DATA, *app_data);
    }
}

}  //  unnamed namespace

struct tag_table_entry {
    const char *name;
    keymaster_tag_t tag;
};

static struct tag_table_entry tag_table[] =
{
    {"KM_TAG_PURPOSE", KM_TAG_PURPOSE},
    {"KM_TAG_ALGORITHM", KM_TAG_ALGORITHM},
    {"KM_TAG_KEY_SIZE", KM_TAG_KEY_SIZE},
    {"KM_TAG_BLOCK_MODE", KM_TAG_BLOCK_MODE},
    {"KM_TAG_DIGEST", KM_TAG_DIGEST},
    {"KM_TAG_PADDING", KM_TAG_PADDING},
    {"KM_TAG_CALLER_NONCE", KM_TAG_CALLER_NONCE},
    {"KM_TAG_MIN_MAC_LENGTH", KM_TAG_MIN_MAC_LENGTH},
    {"KM_TAG_RSA_PUBLIC_EXPONENT", KM_TAG_RSA_PUBLIC_EXPONENT},
    {"KM_TAG_BLOB_USAGE_REQUIREMENTS", KM_TAG_BLOB_USAGE_REQUIREMENTS},
    {"KM_TAG_BOOTLOADER_ONLY", KM_TAG_BOOTLOADER_ONLY},
    {"KM_TAG_ACTIVE_DATETIME", KM_TAG_ACTIVE_DATETIME},
    {"KM_TAG_ORIGINATION_EXPIRE_DATETIME", KM_TAG_ORIGINATION_EXPIRE_DATETIME},
    {"KM_TAG_USAGE_EXPIRE_DATETIME",KM_TAG_USAGE_EXPIRE_DATETIME},
    {"KM_TAG_MIN_SECONDS_BETWEEN_OPS",KM_TAG_MIN_SECONDS_BETWEEN_OPS},
    {"KM_TAG_MAX_USES_PER_BOOT",KM_TAG_MAX_USES_PER_BOOT},
    {"KM_TAG_ALL_USERS", KM_TAG_ALL_USERS},
    {"KM_TAG_USER_ID", KM_TAG_USER_ID},
    {"KM_TAG_USER_SECURE_ID",KM_TAG_USER_SECURE_ID},
    {"KM_TAG_NO_AUTH_REQUIRED",KM_TAG_NO_AUTH_REQUIRED},
    {"KM_TAG_USER_AUTH_TYPE ", KM_TAG_USER_AUTH_TYPE},
    {"KM_TAG_AUTH_TIMEOUT ",KM_TAG_AUTH_TIMEOUT },
    {"KM_TAG_ALL_APPLICATIONS ", KM_TAG_ALL_APPLICATIONS },
    {"KM_TAG_APPLICATION_ID", KM_TAG_APPLICATION_ID},
    {"KM_TAG_APPLICATION_DATA ",KM_TAG_APPLICATION_DATA },
    {"KM_TAG_CREATION_DATETIME ",KM_TAG_CREATION_DATETIME },
    {"KM_TAG_ORIGIN ", KM_TAG_ORIGIN },
    {"KM_TAG_ROLLBACK_RESISTANT ", KM_TAG_ROLLBACK_RESISTANT },
    {"KM_TAG_ROOT_OF_TRUST",  KM_TAG_ROOT_OF_TRUST},
    {"KM_TAG_ASSOCIATED_DATA ",KM_TAG_ASSOCIATED_DATA},
    {"KM_TAG_NONCE", KM_TAG_NONCE},
    {"KM_TAG_AUTH_TOKEN",KM_TAG_AUTH_TOKEN},
    {"KM_TAG_MAC_LENGTH", KM_TAG_MAC_LENGTH},
};

const size_t tag_table_size = sizeof(tag_table)/sizeof(struct tag_table_entry);

void AmlKeymasterDevice::dump_tag_item_value(const char *name, const keymaster_key_param_t* item)
{
    keymaster_tag_type_t type = KM_INVALID;

    if (item) {
        type = keymaster_tag_get_type(item->tag);
        switch (type) {
            case KM_ULONG:
            case KM_ULONG_REP:
                ALOGI("%s: %" PRIx64 "\n", name, item->long_integer);
                //printf("%s: %" PRIx64 "\n", name, item->long_integer);
                break;
            case KM_DATE:
                ALOGI("%s: %" PRIx64 "\n", name, item->date_time);
                //printf("%s: %" PRIx64 "\n", name, item->date_time);
                break;
            case KM_BYTES:
            case KM_BIGNUM:
                ALOGI("%s: blob data: %p, len: 0x%zx\n", name, item->blob.data, item->blob.data_length);
                //printf("%s: blob data: %p, len: 0x%zx\n", name, item->blob.data, item->blob.data_length);
                break;
            case KM_ENUM:
            case KM_ENUM_REP:
                ALOGI("%s: 0x%x\n", name, item->enumerated);
                //printf("%s: 0x%x\n", name, item->enumerated);
                break;
            case KM_BOOL:
                ALOGI("%s: 0x%x\n", name, item->boolean);
                //printf("%s: 0x%x\n", name, item->boolean);
                break;
            case KM_UINT:
            case KM_UINT_REP:
                ALOGI("%s: 0x%x\n", name, item->integer);
                //printf("%s: 0x%x\n", name, item->integer);
                break;
            default:
                ALOGI("%s: invalid type: %d\n", name, type);
                //printf("%s: invalid type: %d\n", name, type);
                break;
        }
    }
}

void AmlKeymasterDevice::dump_tags(const char *name, const keymaster_key_param_set_t *params)
{
    size_t i = 0, j =0;
    keymaster_key_param_t* item = params->params;

    ALOGI("==== start dump %s, length (%zu)\n", name, params->length);
    //printf("==== start dump %s, length (%zu)\n", name, params->length);
    for (i = 0; i < params->length; i++) {
        for (j = 0; j < tag_table_size; j++) {
            if (tag_table[j].tag == item[i].tag) {
                dump_tag_item_value(tag_table[j].name, &item[i]);
                break;
            }
        }
    }
    ALOGI("==== end dump %s\n", name);
    //printf("==== end dump %s\n", name);
}

keymaster_error_t AmlKeymasterDevice::configure(const keymaster_key_param_set_t* params) {
    ALOGD("Device received configure\n");

    if (error_ != KM_ERROR_OK) {
        return error_;
    }
    if (!params) {
        return KM_ERROR_UNEXPECTED_NULL_POINTER;
    }

    AuthorizationSet params_copy(*params);
    ConfigureRequest request;
    if (!params_copy.GetTagValue(TAG_OS_VERSION, &request.os_version) ||
        !params_copy.GetTagValue(TAG_OS_PATCHLEVEL, &request.os_patchlevel)) {
        ALOGD("Configuration parameters must contain OS version and patch level");
        return KM_ERROR_INVALID_ARGUMENT;
    }

    ConfigureResponse response;
    keymaster_error_t err = Send(KM_CONFIGURE, request, &response);
    if (err != KM_ERROR_OK) {
        return err;
    }

    return KM_ERROR_OK;
}

keymaster_error_t AmlKeymasterDevice::add_rng_entropy(const uint8_t* data, size_t data_length) {
    ALOGD("Device received add_rng_entropy");

    if (error_ != KM_ERROR_OK) {
        return error_;
    }

    AddEntropyRequest request;
    request.random_data.Reinitialize(data, data_length);
    AddEntropyResponse response;
    return Send(KM_ADD_RNG_ENTROPY, request, &response);
}

keymaster_error_t AmlKeymasterDevice::simple_bin2ascii(uint8_t *data, size_t data_length, char *out) {
    for (size_t i = 0; i < data_length; i++) {
        if (((data[i] & 0xf0) >> 4) < 0xa)
            out[i * 2] = ((data[i] & 0xf0) >> 4) + 48;
        else
            out[i * 2] = ((data[i] & 0xf0) >> 4) + 87;
        if ((data[i] & 0xf) < 0xa)
            out[i * 2 + 1] = (data[i] & 0xf) + 48;
        else
            out[i * 2 + 1] = (data[i] & 0xf) + 87;
    }

    return KM_ERROR_OK;
}

keymaster_error_t AmlKeymasterDevice::store_encrypted_key(keymaster_key_blob_t* key_blob) {
    SHA256_CTX sha256_ctx;
    UniquePtr<uint8_t[]> hash_buf(new (std::nothrow) uint8_t[SHA256_DIGEST_LENGTH + 1]);
    UniquePtr<char[]> name_buf(new (std::nothrow) char [SHA256_DIGEST_LENGTH * 2 + 1]);
    std::ofstream out;
    char name[256];

    // Hash key data to create filename.
    Eraser sha256_ctx_eraser(sha256_ctx);
    memset(name_buf.get(), 0, SHA256_DIGEST_LENGTH * 2 + 1);
    SHA256_Init(&sha256_ctx);
    SHA256_Update(&sha256_ctx, key_blob->key_material, key_blob->key_material_size);
    SHA256_Final(hash_buf.get(), &sha256_ctx);

    simple_bin2ascii(hash_buf.get(), SHA256_DIGEST_LENGTH, name_buf.get());
    name_buf[SHA256_DIGEST_LENGTH * 2] = '\0';
    sprintf(name, "/data/tee/%s", name_buf.get());
    out.open(name, std::ofstream::out | std::ofstream::binary);
    if (out.is_open()) {
        out.write((const char *)key_blob->key_material, key_blob->key_material_size);
        out.close();
    } else {
        ALOGE("error opening key files\n");
    }

    return KM_ERROR_OK;
}

keymaster_error_t AmlKeymasterDevice::delete_encrypted_key(const keymaster_key_blob_t* key_blob) {
    SHA256_CTX sha256_ctx;
    UniquePtr<uint8_t[]> hash_buf(new (std::nothrow) uint8_t[SHA256_DIGEST_LENGTH + 1]);
    UniquePtr<char[]> name_buf(new (std::nothrow) char [SHA256_DIGEST_LENGTH * 2 + 1]);
    std::ofstream out;
    char name[256];
    int result = -1;

    // Hash key data to get filename.
    Eraser sha256_ctx_eraser(sha256_ctx);
    memset(name_buf.get(), 0, SHA256_DIGEST_LENGTH * 2 + 1);
    SHA256_Init(&sha256_ctx);
    SHA256_Update(&sha256_ctx, key_blob->key_material, key_blob->key_material_size);
    SHA256_Final(hash_buf.get(), &sha256_ctx);

    simple_bin2ascii(hash_buf.get(), SHA256_DIGEST_LENGTH, name_buf.get());
    name_buf[SHA256_DIGEST_LENGTH * 2] = '\0';
    sprintf(name, "/data/tee/%s", name_buf.get());
    out.open(name, std::ofstream::out | std::ofstream::binary);
    result = unlink(name);

    if (!result) {
        return KM_ERROR_OK;
    } else {
        ALOGE("cannot locate %s\n", name);
        return  KM_ERROR_INVALID_OPERATION_HANDLE;
    }
}

keymaster_error_t AmlKeymasterDevice::generate_key(
    const keymaster_key_param_set_t* params, keymaster_key_blob_t* key_blob,
    keymaster_key_characteristics_t* characteristics) {
    ALOGD("Device received generate_key");

    if (error_ != KM_ERROR_OK) {
        return error_;
    }
    if (!params) {
        return KM_ERROR_UNEXPECTED_NULL_POINTER;
    }
    if (!key_blob) {
        return KM_ERROR_OUTPUT_PARAMETER_NULL;
    }

    GenerateKeyRequest request(message_version_);
    request.key_description.Reinitialize(*params);
    request.key_description.push_back(TAG_CREATION_DATETIME, java_time(time(NULL)));

    GenerateKeyResponse response(message_version_);
    keymaster_error_t err = Send(KM_GENERATE_KEY, request, &response);
    if (err != KM_ERROR_OK) {
        return err;
    }

    key_blob->key_material_size = response.key_blob.key_material_size;
    key_blob->key_material =
        DuplicateBuffer(response.key_blob.key_material, response.key_blob.key_material_size);
    if (!key_blob->key_material) {
        return KM_ERROR_MEMORY_ALLOCATION_FAILED;
    }

    if (characteristics) {
        response.enforced.CopyToParamSet(&characteristics->hw_enforced);
        response.unenforced.CopyToParamSet(&characteristics->sw_enforced);
    }

    return KM_ERROR_OK;
}

keymaster_error_t AmlKeymasterDevice::get_key_characteristics(
    const keymaster_key_blob_t* key_blob, const keymaster_blob_t* client_id,
    const keymaster_blob_t* app_data, keymaster_key_characteristics_t* characteristics) {
    ALOGD("Device received get_key_characteristics");

    if (error_ != KM_ERROR_OK) {
        return error_;
    }
    if (!key_blob || !key_blob->key_material) {
        return KM_ERROR_UNEXPECTED_NULL_POINTER;
    }
    if (!characteristics) {
        return KM_ERROR_OUTPUT_PARAMETER_NULL;
    }

    GetKeyCharacteristicsRequest request;
    request.SetKeyMaterial(*key_blob);
    AddClientAndAppData(client_id, app_data, &request);

    GetKeyCharacteristicsResponse response;
    keymaster_error_t err = Send(KM_GET_KEY_CHARACTERISTICS, request, &response);
    if (err != KM_ERROR_OK) {
        return err;
    }

    response.enforced.CopyToParamSet(&characteristics->hw_enforced);
    response.unenforced.CopyToParamSet(&characteristics->sw_enforced);

    return KM_ERROR_OK;
}

keymaster_error_t AmlKeymasterDevice::import_key(
    const keymaster_key_param_set_t* params, keymaster_key_format_t key_format,
    const keymaster_blob_t* key_data, keymaster_key_blob_t* key_blob,
    keymaster_key_characteristics_t* characteristics) {
    ALOGD("Device received import_key");

    if (error_ != KM_ERROR_OK) {
        return error_;
    }
    if (!params || !key_data) {
        return KM_ERROR_UNEXPECTED_NULL_POINTER;
    }
    if (!key_blob) {
        return KM_ERROR_OUTPUT_PARAMETER_NULL;
    }

    ImportKeyRequest request(message_version_);
    request.key_description.Reinitialize(*params);
    request.key_description.push_back(TAG_CREATION_DATETIME, java_time(time(NULL)));

    dump_tags("import", &request.key_description);
    request.key_format = key_format;
    request.SetKeyMaterial(key_data->data, key_data->data_length);

    ImportKeyResponse response(message_version_);
    keymaster_error_t err = Send(KM_IMPORT_KEY, request, &response);
    if (err != KM_ERROR_OK) {
        return err;
    }

    key_blob->key_material_size = response.key_blob.key_material_size;
    key_blob->key_material =
        DuplicateBuffer(response.key_blob.key_material, response.key_blob.key_material_size);
    if (!key_blob->key_material) {
        return KM_ERROR_MEMORY_ALLOCATION_FAILED;
    }

    dump_tags("hw", &response.enforced);
    dump_tags("sw", &response.unenforced);
    if (characteristics) {
        response.enforced.CopyToParamSet(&characteristics->hw_enforced);
        response.unenforced.CopyToParamSet(&characteristics->sw_enforced);
    }

    return KM_ERROR_OK;
}

keymaster_error_t AmlKeymasterDevice::export_key(keymaster_key_format_t export_format,
                                                    const keymaster_key_blob_t* key_to_export,
                                                    const keymaster_blob_t* client_id,
                                                    const keymaster_blob_t* app_data,
                                                    keymaster_blob_t* export_data) {
    ALOGD("Device received export_key");

    if (error_ != KM_ERROR_OK) {
        return error_;
    }
    if (!key_to_export || !key_to_export->key_material) {
        return KM_ERROR_UNEXPECTED_NULL_POINTER;
    }
    if (!export_data) {
        return KM_ERROR_OUTPUT_PARAMETER_NULL;
    }

    export_data->data = nullptr;
    export_data->data_length = 0;

    ExportKeyRequest request(message_version_);
    request.key_format = export_format;
    request.SetKeyMaterial(*key_to_export);
    AddClientAndAppData(client_id, app_data, &request);

    ExportKeyResponse response(message_version_);
    keymaster_error_t err = Send(KM_EXPORT_KEY, request, &response);
    if (err != KM_ERROR_OK) {
        return err;
    }

    export_data->data_length = response.key_data_length;
    export_data->data = DuplicateBuffer(response.key_data, response.key_data_length);
    if (!export_data->data) {
        return KM_ERROR_MEMORY_ALLOCATION_FAILED;
    }

    return KM_ERROR_OK;
}

keymaster_error_t AmlKeymasterDevice::attest_key(const keymaster_key_blob_t* key_to_attest,
                                                    const keymaster_key_param_set_t* attest_params,
                                                    keymaster_cert_chain_t* cert_chain) {
    ALOGD("Device received attest_key");

    if (error_ != KM_ERROR_OK) {
        return error_;
    }
    if (!key_to_attest || !attest_params) {
        return KM_ERROR_UNEXPECTED_NULL_POINTER;
    }
    if (!cert_chain) {
        return KM_ERROR_OUTPUT_PARAMETER_NULL;
    }

    cert_chain->entry_count = 0;
    cert_chain->entries = nullptr;

    AttestKeyRequest request;
    request.SetKeyMaterial(*key_to_attest);
    request.attest_params.Reinitialize(*attest_params);

    keymaster_blob_t attestation_challenge = {};
    request.attest_params.GetTagValue(TAG_ATTESTATION_CHALLENGE, &attestation_challenge);
    if (attestation_challenge.data_length > kMaximumAttestationChallengeLength) {
        ALOGE("%zu-byte attestation challenge; only %zu bytes allowed",
              attestation_challenge.data_length, kMaximumAttestationChallengeLength);
        return KM_ERROR_INVALID_INPUT_LENGTH;
    }

    AttestKeyResponse response;
    keymaster_error_t err = Send(KM_ATTEST_KEY, request, &response);
    if (err != KM_ERROR_OK) {
        return err;
    }

    // Allocate and clear storage for cert_chain.
    keymaster_cert_chain_t& rsp_chain = response.certificate_chain;
    cert_chain->entries = reinterpret_cast<keymaster_blob_t*>(
        malloc(rsp_chain.entry_count * sizeof(*cert_chain->entries)));
    if (!cert_chain->entries) {
        return KM_ERROR_MEMORY_ALLOCATION_FAILED;
    }
    cert_chain->entry_count = rsp_chain.entry_count;
    for (keymaster_blob_t& entry : array_range(cert_chain->entries, cert_chain->entry_count)) {
        entry = {};
    }

    // Copy cert_chain contents
    size_t i = 0;
    for (keymaster_blob_t& entry : array_range(rsp_chain.entries, rsp_chain.entry_count)) {
        cert_chain->entries[i].data = DuplicateBuffer(entry.data, entry.data_length);
        if (!cert_chain->entries[i].data) {
            keymaster_free_cert_chain(cert_chain);
            return KM_ERROR_MEMORY_ALLOCATION_FAILED;
        }
        cert_chain->entries[i].data_length = entry.data_length;
        ++i;
    }

    return KM_ERROR_OK;
}

keymaster_error_t AmlKeymasterDevice::upgrade_key(const keymaster_key_blob_t* key_to_upgrade,
                                                     const keymaster_key_param_set_t* upgrade_params,
                                                     keymaster_key_blob_t* upgraded_key) {
    ALOGD("Device received upgrade_key");

    if (error_ != KM_ERROR_OK) {
        return error_;
    }
    if (!key_to_upgrade || !upgrade_params) {
        return KM_ERROR_UNEXPECTED_NULL_POINTER;
    }
    if (!upgraded_key) {
        return KM_ERROR_OUTPUT_PARAMETER_NULL;
    }

    UpgradeKeyRequest request;
    request.SetKeyMaterial(*key_to_upgrade);
    request.upgrade_params.Reinitialize(*upgrade_params);

    UpgradeKeyResponse response;
    keymaster_error_t err = Send(KM_UPGRADE_KEY, request, &response);
    if (err != KM_ERROR_OK) {
        return err;
    }

    upgraded_key->key_material_size = response.upgraded_key.key_material_size;
    upgraded_key->key_material = DuplicateBuffer(response.upgraded_key.key_material,
                                                 response.upgraded_key.key_material_size);
    if (!upgraded_key->key_material) {
        return KM_ERROR_MEMORY_ALLOCATION_FAILED;
    }

    return KM_ERROR_OK;
}

keymaster_error_t AmlKeymasterDevice::delete_key(const keymaster_key_blob_t* key) {
    (void)key;
    return KM_ERROR_OK;
}

keymaster_error_t AmlKeymasterDevice::begin(keymaster_purpose_t purpose,
                                               const keymaster_key_blob_t* key,
                                               const keymaster_key_param_set_t* in_params,
                                               keymaster_key_param_set_t* out_params,
                                               keymaster_operation_handle_t* operation_handle) {
    ALOGD("Device received begin");

    if (error_ != KM_ERROR_OK) {
        return error_;
    }
    if (!key || !key->key_material) {
        return KM_ERROR_UNEXPECTED_NULL_POINTER;
    }
    if (!operation_handle) {
        return KM_ERROR_OUTPUT_PARAMETER_NULL;
    }

    if (out_params) {
        *out_params = {};
    }

    BeginOperationRequest request;
    request.purpose = purpose;
    request.SetKeyMaterial(*key);
    request.additional_params.Reinitialize(*in_params);

    BeginOperationResponse response;
    keymaster_error_t err = Send(KM_BEGIN_OPERATION, request, &response);
    if (err != KM_ERROR_OK) {
        return err;
    }

    if (response.output_params.size() > 0) {
        if (out_params) {
            response.output_params.CopyToParamSet(out_params);
        } else {
            return KM_ERROR_OUTPUT_PARAMETER_NULL;
        }
    }
    *operation_handle = response.op_handle;

    return KM_ERROR_OK;
}

keymaster_error_t AmlKeymasterDevice::update(keymaster_operation_handle_t operation_handle,
                                                const keymaster_key_param_set_t* in_params,
                                                const keymaster_blob_t* input,
                                                size_t* input_consumed,
                                                keymaster_key_param_set_t* out_params,
                                                keymaster_blob_t* output) {
    ALOGD("Device received update");

    if (error_ != KM_ERROR_OK) {
        return error_;
    }
    if (!input) {
        return KM_ERROR_UNEXPECTED_NULL_POINTER;
    }
    if (!input_consumed) {
        return KM_ERROR_OUTPUT_PARAMETER_NULL;
    }

    if (out_params) {
        *out_params = {};
    }
    if (output) {
        *output = {};
    }

    UpdateOperationRequest request;
    request.op_handle = operation_handle;
    if (in_params) {
        request.additional_params.Reinitialize(*in_params);
    }
    if (input && input->data_length > 0) {
        size_t max_input_size = SEND_BUF_SIZE - request.SerializedSize();
        request.input.Reinitialize(input->data, std::min(input->data_length, max_input_size));
    }

    UpdateOperationResponse response;
    keymaster_error_t err = Send(KM_UPDATE_OPERATION, request, &response);
    if (err != KM_ERROR_OK) {
        return err;
    }

    if (response.output_params.size() > 0) {
        if (out_params) {
            response.output_params.CopyToParamSet(out_params);
        } else {
            return KM_ERROR_OUTPUT_PARAMETER_NULL;
        }
    }
    *input_consumed = response.input_consumed;
    if (output) {
        output->data_length = response.output.available_read();
        output->data = DuplicateBuffer(response.output.peek_read(), output->data_length);
        if (!output->data) {
            return KM_ERROR_MEMORY_ALLOCATION_FAILED;
        }
    } else if (response.output.available_read() > 0) {
        return KM_ERROR_OUTPUT_PARAMETER_NULL;
    }

    return KM_ERROR_OK;
}

keymaster_error_t AmlKeymasterDevice::finish(keymaster_operation_handle_t operation_handle,
                                                const keymaster_key_param_set_t* in_params,
                                                const keymaster_blob_t* input,
                                                const keymaster_blob_t* signature,
                                                keymaster_key_param_set_t* out_params,
                                                keymaster_blob_t* output) {
    ALOGD("Device received finish");

    bool size_exceeded = false;
    if (error_ != KM_ERROR_OK) {
        return error_;
    }
    if (input && input->data_length > kMaximumFinishInputLength) {
        ALOGE("%zu-byte input to finish; only %zu bytes allowed",
                input->data_length, kMaximumFinishInputLength);
        size_exceeded = true;
    }

    if (out_params) {
        *out_params = {};
    }
    if (output) {
        *output = {};
    }

    FinishOperationRequest request;
    request.op_handle = operation_handle;
    if (signature && signature->data && signature->data_length > 0) {
        request.signature.Reinitialize(signature->data, signature->data_length);
    }
    if (input && input->data && input->data_length) {
        /* sending fake request to close operation handle */
        if (size_exceeded)
            request.input.Reinitialize(input->data, 1);
        else
            request.input.Reinitialize(input->data, input->data_length);
    }
    if (in_params) {
        request.additional_params.Reinitialize(*in_params);
    }

    FinishOperationResponse response;
    keymaster_error_t err = Send(KM_FINISH_OPERATION, request, &response);
    /* drop result in case of fake request */
    if (size_exceeded)
        return KM_ERROR_INVALID_INPUT_LENGTH;
    if (err != KM_ERROR_OK) {
        return err;
    }

    if (response.output_params.size() > 0) {
        if (out_params) {
            response.output_params.CopyToParamSet(out_params);
        } else {
            return KM_ERROR_OUTPUT_PARAMETER_NULL;
        }
    }
    if (output) {
        output->data_length = response.output.available_read();
        output->data = DuplicateBuffer(response.output.peek_read(), output->data_length);
        if (!output->data) {
            return KM_ERROR_MEMORY_ALLOCATION_FAILED;
        }
    } else if (response.output.available_read() > 0) {
        return KM_ERROR_OUTPUT_PARAMETER_NULL;
    }

    return KM_ERROR_OK;
}

keymaster_error_t AmlKeymasterDevice::abort(keymaster_operation_handle_t operation_handle) {
    ALOGD("Device received abort");

    if (error_ != KM_ERROR_OK) {
        return error_;
    }

    AbortOperationRequest request;
    request.op_handle = operation_handle;
    AbortOperationResponse response;
    return Send(KM_ABORT_OPERATION, request, &response);
}

hw_device_t* AmlKeymasterDevice::hw_device() {
    return &device_.common;
}

static inline AmlKeymasterDevice* convert_device(const keymaster2_device_t* dev) {
    return reinterpret_cast<AmlKeymasterDevice*>(const_cast<keymaster2_device_t*>(dev));
}

/* static */
int AmlKeymasterDevice::close_device(hw_device_t* dev) {
    delete reinterpret_cast<AmlKeymasterDevice*>(dev);
    return 0;
}

/* static */
keymaster_error_t AmlKeymasterDevice::configure(const keymaster2_device_t* dev,
                                                   const keymaster_key_param_set_t* params) {
    return convert_device(dev)->configure(params);
}

/* static */
keymaster_error_t AmlKeymasterDevice::add_rng_entropy(const keymaster2_device_t* dev,
                                                         const uint8_t* data, size_t data_length) {
    return convert_device(dev)->add_rng_entropy(data, data_length);
}

/* static */
keymaster_error_t AmlKeymasterDevice::generate_key(
    const keymaster2_device_t* dev, const keymaster_key_param_set_t* params,
    keymaster_key_blob_t* key_blob, keymaster_key_characteristics_t* characteristics) {
    return convert_device(dev)->generate_key(params, key_blob, characteristics);
}

/* static */
keymaster_error_t AmlKeymasterDevice::get_key_characteristics(
    const keymaster2_device_t* dev, const keymaster_key_blob_t* key_blob,
    const keymaster_blob_t* client_id, const keymaster_blob_t* app_data,
    keymaster_key_characteristics_t* characteristics) {
    return convert_device(dev)->get_key_characteristics(key_blob, client_id, app_data,
                                                        characteristics);
}

/* static */
keymaster_error_t AmlKeymasterDevice::import_key(
    const keymaster2_device_t* dev, const keymaster_key_param_set_t* params,
    keymaster_key_format_t key_format, const keymaster_blob_t* key_data,
    keymaster_key_blob_t* key_blob, keymaster_key_characteristics_t* characteristics) {
    return convert_device(dev)->import_key(params, key_format, key_data, key_blob, characteristics);
}

/* static */
keymaster_error_t AmlKeymasterDevice::export_key(const keymaster2_device_t* dev,
                                                    keymaster_key_format_t export_format,
                                                    const keymaster_key_blob_t* key_to_export,
                                                    const keymaster_blob_t* client_id,
                                                    const keymaster_blob_t* app_data,
                                                    keymaster_blob_t* export_data) {
    return convert_device(dev)->export_key(export_format, key_to_export, client_id, app_data,
                                           export_data);
}

/* static */
keymaster_error_t AmlKeymasterDevice::attest_key(const keymaster2_device_t* dev,
                                                    const keymaster_key_blob_t* key_to_attest,
                                                    const keymaster_key_param_set_t* attest_params,
                                                    keymaster_cert_chain_t* cert_chain) {
    return convert_device(dev)->attest_key(key_to_attest, attest_params, cert_chain);
}

/* static */
keymaster_error_t AmlKeymasterDevice::upgrade_key(const keymaster2_device_t* dev,
                                                     const keymaster_key_blob_t* key_to_upgrade,
                                                     const keymaster_key_param_set_t* upgrade_params,
                                                     keymaster_key_blob_t* upgraded_key) {
    return convert_device(dev)->upgrade_key(key_to_upgrade, upgrade_params, upgraded_key);
}

/* static */
keymaster_error_t AmlKeymasterDevice::delete_key(const keymaster2_device_t* dev,
                                               const keymaster_key_blob_t* key_blob) {
    return convert_device(dev)->delete_key(key_blob);
}

/* static */
keymaster_error_t AmlKeymasterDevice::begin(const keymaster2_device_t* dev,
                                               keymaster_purpose_t purpose,
                                               const keymaster_key_blob_t* key,
                                               const keymaster_key_param_set_t* in_params,
                                               keymaster_key_param_set_t* out_params,
                                               keymaster_operation_handle_t* operation_handle) {
    return convert_device(dev)->begin(purpose, key, in_params, out_params, operation_handle);
}

/* static */
keymaster_error_t AmlKeymasterDevice::update(
    const keymaster2_device_t* dev, keymaster_operation_handle_t operation_handle,
    const keymaster_key_param_set_t* in_params, const keymaster_blob_t* input,
    size_t* input_consumed, keymaster_key_param_set_t* out_params, keymaster_blob_t* output) {
    return convert_device(dev)->update(operation_handle, in_params, input, input_consumed,
                                       out_params, output);
}

/* static */
keymaster_error_t AmlKeymasterDevice::finish(const keymaster2_device_t* dev,
                                                keymaster_operation_handle_t operation_handle,
                                                const keymaster_key_param_set_t* in_params,
                                                const keymaster_blob_t* input,
                                                const keymaster_blob_t* signature,
                                                keymaster_key_param_set_t* out_params,
                                                keymaster_blob_t* output) {
    return convert_device(dev)->finish(operation_handle, in_params, input, signature, out_params,
                                       output);
}

/* static */
keymaster_error_t AmlKeymasterDevice::abort(const keymaster2_device_t* dev,
                                               keymaster_operation_handle_t operation_handle) {
    return convert_device(dev)->abort(operation_handle);
}

keymaster_error_t AmlKeymasterDevice::Send(uint32_t command, const Serializable& req,
                                              KeymasterResponse* rsp) {
    uint32_t req_size = req.SerializedSize();

    if (req_size > SEND_BUF_SIZE) {
        return KM_ERROR_MEMORY_ALLOCATION_FAILED;
    }
    //uint8_t send_buf[SEND_BUF_SIZE];
    UniquePtr<uint8_t[]> send_buf (new (std::nothrow) uint8_t[SEND_BUF_SIZE]);
    if (!send_buf.get())
        return KM_ERROR_MEMORY_ALLOCATION_FAILED;
    Eraser send_buf_eraser(send_buf.get(), SEND_BUF_SIZE);
    req.Serialize(send_buf.get(), send_buf.get() + req_size);

    // Send it
    //uint8_t recv_buf[RECV_BUF_SIZE];
    UniquePtr<uint8_t[]> recv_buf (new (std::nothrow) uint8_t[RECV_BUF_SIZE]);
    if (!recv_buf.get())
        return KM_ERROR_MEMORY_ALLOCATION_FAILED;
    Eraser recv_buf_eraser(recv_buf.get(), RECV_BUF_SIZE);
    uint32_t rsp_size = RECV_BUF_SIZE;
    ALOGD("Sending cmd: %u with %d byte request\n", command, (int)req.SerializedSize());
    TEEC_Result rc = aml_keymaster_call(&KM_session, command, send_buf.get(), req_size, recv_buf.get(), &rsp_size);
    if (rc != TEEC_SUCCESS) {
        return translate_error(rc);
    } else {
        ALOGD("Received %d byte response\n", rsp_size);
    }

    const keymaster_message* msg = (keymaster_message*)recv_buf.get();
    const uint8_t* p = msg->payload;
    if (!rsp->Deserialize(&p, p + rsp_size)) {
        ALOGE("Error deserializing response of size %d\n", (int)rsp_size);
        return KM_ERROR_UNKNOWN_ERROR;
    } else if (rsp->error != KM_ERROR_OK) {
        ALOGE("Response of size %d contained error code %d\n", (int)rsp_size, (int)rsp->error);
        return rsp->error;
    }
    return rsp->error;
}

}  // namespace keymaster

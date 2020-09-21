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

#ifndef AML_KEYMASTER_AML_KEYMASTER_DEVICE_H_
#define AML_KEYMASTER_AML_KEYMASTER_DEVICE_H_

#include <hardware/keymaster2.h>
#include <keymaster/android_keymaster_messages.h>

extern "C" {
#include <tee_client_api.h>
}

namespace keymaster {

/**
 * Aml Keymaster device.
 *
 * IMPORTANT MAINTAINER NOTE: Pointers to instances of this class must be castable to hw_device_t
 * and keymaster_device. This means it must remain a standard layout class (no virtual functions and
 * no data members which aren't standard layout), and device_ must be the first data member.
 * Assertions in the constructor validate compliance with those constraints.
 */
class AmlKeymasterDevice {
  public:
    /*
     * These are the only symbols that will be exported by libamlkeymaster.  All functionality
     * can be reached via the function pointers in device_.
     */
    __attribute__((visibility("default"))) explicit AmlKeymasterDevice(const hw_module_t* module);
    __attribute__((visibility("default"))) hw_device_t* hw_device();

    ~AmlKeymasterDevice();

    keymaster_error_t session_error() { return error_; }

    keymaster_error_t configure(const keymaster_key_param_set_t* params);
    keymaster_error_t add_rng_entropy(const uint8_t* data, size_t data_length);
    keymaster_error_t generate_key(const keymaster_key_param_set_t* params,
                                   keymaster_key_blob_t* key_blob,
                                   keymaster_key_characteristics_t* characteristics);
    keymaster_error_t get_key_characteristics(const keymaster_key_blob_t* key_blob,
                                              const keymaster_blob_t* client_id,
                                              const keymaster_blob_t* app_data,
                                              keymaster_key_characteristics_t* character);
    keymaster_error_t import_key(const keymaster_key_param_set_t* params,
                                 keymaster_key_format_t key_format,
                                 const keymaster_blob_t* key_data, keymaster_key_blob_t* key_blob,
                                 keymaster_key_characteristics_t* characteristics);
    keymaster_error_t export_key(keymaster_key_format_t export_format,
                                 const keymaster_key_blob_t* key_to_export,
                                 const keymaster_blob_t* client_id,
                                 const keymaster_blob_t* app_data, keymaster_blob_t* export_data);
    keymaster_error_t attest_key(const keymaster_key_blob_t* key_to_attest,
                                 const keymaster_key_param_set_t* attest_params,
                                 keymaster_cert_chain_t* cert_chain);
    keymaster_error_t upgrade_key(const keymaster_key_blob_t* key_to_upgrade,
                                  const keymaster_key_param_set_t* upgrade_params,
                                  keymaster_key_blob_t* upgraded_key);
    keymaster_error_t delete_key(const keymaster_key_blob_t* key);
    keymaster_error_t begin(keymaster_purpose_t purpose, const keymaster_key_blob_t* key,
                            const keymaster_key_param_set_t* in_params,
                            keymaster_key_param_set_t* out_params,
                            keymaster_operation_handle_t* operation_handle);
    keymaster_error_t update(keymaster_operation_handle_t operation_handle,
                             const keymaster_key_param_set_t* in_params,
                             const keymaster_blob_t* input, size_t* input_consumed,
                             keymaster_key_param_set_t* out_params, keymaster_blob_t* output);
    keymaster_error_t finish(keymaster_operation_handle_t operation_handle,
                             const keymaster_key_param_set_t* in_params,
                             const keymaster_blob_t* input, const keymaster_blob_t* signature,
                             keymaster_key_param_set_t* out_params, keymaster_blob_t* output);
    keymaster_error_t abort(keymaster_operation_handle_t operation_handle);

    keymaster_error_t store_encrypted_key(keymaster_key_blob_t* key_blob);
    keymaster_error_t delete_encrypted_key(const keymaster_key_blob_t* key_blob);
    keymaster_error_t simple_bin2ascii(uint8_t *data, size_t data_length, char *out);
  private:
    keymaster_error_t Send(uint32_t command, const Serializable& request,
                           KeymasterResponse* response);

    /*
     * These static methods are the functions referenced through the function pointers in
     * keymaster_device.  They're all trivial wrappers.
     */
    static int close_device(hw_device_t* dev);
    static keymaster_error_t configure(const keymaster2_device_t* dev,
                                       const keymaster_key_param_set_t* params);
    static keymaster_error_t add_rng_entropy(const keymaster2_device_t* dev, const uint8_t* data,
                                             size_t data_length);
    static keymaster_error_t generate_key(const keymaster2_device_t* dev,
                                          const keymaster_key_param_set_t* params,
                                          keymaster_key_blob_t* key_blob,
                                          keymaster_key_characteristics_t* characteristics);
    static keymaster_error_t get_key_characteristics(const keymaster2_device_t* dev,
                                                     const keymaster_key_blob_t* key_blob,
                                                     const keymaster_blob_t* client_id,
                                                     const keymaster_blob_t* app_data,
                                                     keymaster_key_characteristics_t* character);
    static keymaster_error_t import_key(const keymaster2_device_t* dev,
                                        const keymaster_key_param_set_t* params,
                                        keymaster_key_format_t key_format,
                                        const keymaster_blob_t* key_data,
                                        keymaster_key_blob_t* key_blob,
                                        keymaster_key_characteristics_t* characteristics);
    static keymaster_error_t export_key(const keymaster2_device_t* dev,
                                        keymaster_key_format_t export_format,
                                        const keymaster_key_blob_t* key_to_export,
                                        const keymaster_blob_t* client_id,
                                        const keymaster_blob_t* app_data,
                                        keymaster_blob_t* export_data);
    static keymaster_error_t attest_key(const keymaster2_device_t* dev,
                                        const keymaster_key_blob_t* key_to_attest,
                                        const keymaster_key_param_set_t* attest_params,
                                        keymaster_cert_chain_t* cert_chain);
    static keymaster_error_t upgrade_key(const keymaster2_device_t* dev,
                                         const keymaster_key_blob_t* key_to_upgrade,
                                         const keymaster_key_param_set_t* upgrade_params,
                                         keymaster_key_blob_t* upgraded_key);
    static keymaster_error_t delete_key(const keymaster2_device_t* dev,
                                        const keymaster_key_blob_t* key);
    static keymaster_error_t delete_all_keys(const keymaster2_device_t* dev);
    static keymaster_error_t begin(const keymaster2_device_t* dev, keymaster_purpose_t purpose,
                                   const keymaster_key_blob_t* key,
                                   const keymaster_key_param_set_t* in_params,
                                   keymaster_key_param_set_t* out_params,
                                   keymaster_operation_handle_t* operation_handle);
    static keymaster_error_t update(const keymaster2_device_t* dev,
                                    keymaster_operation_handle_t operation_handle,
                                    const keymaster_key_param_set_t* in_params,
                                    const keymaster_blob_t* input, size_t* input_consumed,
                                    keymaster_key_param_set_t* out_params, keymaster_blob_t* output);
    static keymaster_error_t finish(const keymaster2_device_t* dev,
                                    keymaster_operation_handle_t operation_handle,
                                    const keymaster_key_param_set_t* in_params,
                                    const keymaster_blob_t* input, const keymaster_blob_t* signature,
                                    keymaster_key_param_set_t* out_params, keymaster_blob_t* output);
    static keymaster_error_t abort(const keymaster2_device_t* dev,
                                   keymaster_operation_handle_t operation_handle);

    void dump_tags(const char *name, const keymaster_key_param_set_t *params);
    void dump_tag_item_value(const char *name, const keymaster_key_param_t* item);

    keymaster2_device_t device_;
    keymaster_error_t error_;
    int32_t message_version_;

    TEEC_Context KM_context;
    TEEC_Session KM_session;
};

#if ANDROID_PLATFORM_SDK_VERSION == 26 //8.0
struct ConfigureRequest : public KeymasterMessage {
    explicit ConfigureRequest(int32_t ver = MAX_MESSAGE_VERSION) : KeymasterMessage(ver) {}

    size_t SerializedSize() const override { return sizeof(os_version) + sizeof(os_patchlevel); }
    uint8_t* Serialize(uint8_t* buf, const uint8_t* end) const override {
        buf = append_uint32_to_buf(buf, end, os_version);
        return append_uint32_to_buf(buf, end, os_patchlevel);
    }
    bool Deserialize(const uint8_t** buf_ptr, const uint8_t* end) override {
        return copy_uint32_from_buf(buf_ptr, end, &os_version) &&
               copy_uint32_from_buf(buf_ptr, end, &os_patchlevel);
    }

    uint32_t os_version;
    uint32_t os_patchlevel;
};

struct ConfigureResponse : public KeymasterResponse {
    explicit ConfigureResponse(int32_t ver = MAX_MESSAGE_VERSION) : KeymasterResponse(ver) {}

    size_t NonErrorSerializedSize() const override { return 0; }
    uint8_t* NonErrorSerialize(uint8_t* buf, const uint8_t*) const override { return buf; }
    bool NonErrorDeserialize(const uint8_t**, const uint8_t*) override { return true; }
};
#endif

}  // namespace keymaster

#endif  // AML_KEYMASTER_AML_KEYMASTER_DEVICE_H_

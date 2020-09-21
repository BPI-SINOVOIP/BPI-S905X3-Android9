/*
 * Copyright (C) 2016 The Android Open Source Project
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

#define LOG_TAG "keymaster_hidl_hal_test"
#include <cutils/log.h>

#include <iostream>

#include <openssl/evp.h>
#include <openssl/mem.h>
#include <openssl/x509.h>

#include <android/hardware/keymaster/3.0/IKeymasterDevice.h>
#include <android/hardware/keymaster/3.0/types.h>

#include <cutils/properties.h>

#include <keymaster/keymaster_configuration.h>

#include "authorization_set.h"
#include "key_param_output.h"

#include <VtsHalHidlTargetTestBase.h>
#include <VtsHalHidlTargetTestEnvBase.h>

#include "attestation_record.h"
#include "openssl_utils.h"
#include "keymaster_hidl_hal_test.h"

using ::android::sp;
using ::std::string;

static bool arm_deleteAllKeys = false;
static bool dump_Attestations = false;

namespace android {
namespace hardware {

template <typename T> bool operator==(const hidl_vec<T>& a, const hidl_vec<T>& b) {
    if (a.size() != b.size()) {
        return false;
    }
    for (size_t i = 0; i < a.size(); ++i) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

namespace keymaster {
namespace V3_0 {

bool operator==(const KeyParameter& a, const KeyParameter& b) {
    if (a.tag != b.tag) {
        return false;
    }

    switch (a.tag) {

    /* Boolean tags */
    case Tag::INVALID:
    case Tag::CALLER_NONCE:
    case Tag::INCLUDE_UNIQUE_ID:
    case Tag::ECIES_SINGLE_HASH_MODE:
    case Tag::BOOTLOADER_ONLY:
    case Tag::NO_AUTH_REQUIRED:
    case Tag::ALLOW_WHILE_ON_BODY:
    case Tag::EXPORTABLE:
    case Tag::ALL_APPLICATIONS:
    case Tag::ROLLBACK_RESISTANT:
    case Tag::RESET_SINCE_ID_ROTATION:
        return true;

    /* Integer tags */
    case Tag::KEY_SIZE:
    case Tag::MIN_MAC_LENGTH:
    case Tag::MIN_SECONDS_BETWEEN_OPS:
    case Tag::MAX_USES_PER_BOOT:
    case Tag::ALL_USERS:
    case Tag::USER_ID:
    case Tag::OS_VERSION:
    case Tag::OS_PATCHLEVEL:
    case Tag::MAC_LENGTH:
    case Tag::AUTH_TIMEOUT:
        return a.f.integer == b.f.integer;

    /* Long integer tags */
    case Tag::RSA_PUBLIC_EXPONENT:
    case Tag::USER_SECURE_ID:
        return a.f.longInteger == b.f.longInteger;

    /* Date-time tags */
    case Tag::ACTIVE_DATETIME:
    case Tag::ORIGINATION_EXPIRE_DATETIME:
    case Tag::USAGE_EXPIRE_DATETIME:
    case Tag::CREATION_DATETIME:
        return a.f.dateTime == b.f.dateTime;

    /* Bytes tags */
    case Tag::APPLICATION_ID:
    case Tag::APPLICATION_DATA:
    case Tag::ROOT_OF_TRUST:
    case Tag::UNIQUE_ID:
    case Tag::ATTESTATION_CHALLENGE:
    case Tag::ATTESTATION_APPLICATION_ID:
    case Tag::ATTESTATION_ID_BRAND:
    case Tag::ATTESTATION_ID_DEVICE:
    case Tag::ATTESTATION_ID_PRODUCT:
    case Tag::ATTESTATION_ID_SERIAL:
    case Tag::ATTESTATION_ID_IMEI:
    case Tag::ATTESTATION_ID_MEID:
    case Tag::ATTESTATION_ID_MANUFACTURER:
    case Tag::ATTESTATION_ID_MODEL:
    case Tag::ASSOCIATED_DATA:
    case Tag::NONCE:
    case Tag::AUTH_TOKEN:
        return a.blob == b.blob;

    /* Enum tags */
    case Tag::PURPOSE:
        return a.f.purpose == b.f.purpose;
    case Tag::ALGORITHM:
        return a.f.algorithm == b.f.algorithm;
    case Tag::BLOCK_MODE:
        return a.f.blockMode == b.f.blockMode;
    case Tag::DIGEST:
        return a.f.digest == b.f.digest;
    case Tag::PADDING:
        return a.f.paddingMode == b.f.paddingMode;
    case Tag::EC_CURVE:
        return a.f.ecCurve == b.f.ecCurve;
    case Tag::BLOB_USAGE_REQUIREMENTS:
        return a.f.keyBlobUsageRequirements == b.f.keyBlobUsageRequirements;
    case Tag::USER_AUTH_TYPE:
        return a.f.integer == b.f.integer;
    case Tag::ORIGIN:
        return a.f.origin == b.f.origin;

    /* Unsupported tags */
    case Tag::KDF:
        return false;
    }
}

bool operator==(const AuthorizationSet& a, const AuthorizationSet& b) {
    return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin());
}

bool operator==(const KeyCharacteristics& a, const KeyCharacteristics& b) {
    // This isn't very efficient. Oh, well.
    AuthorizationSet a_sw(a.softwareEnforced);
    AuthorizationSet b_sw(b.softwareEnforced);
    AuthorizationSet a_tee(b.teeEnforced);
    AuthorizationSet b_tee(b.teeEnforced);

    a_sw.Sort();
    b_sw.Sort();
    a_tee.Sort();
    b_tee.Sort();

    return a_sw == b_sw && a_tee == b_sw;
}

::std::ostream& operator<<(::std::ostream& os, const AuthorizationSet& set) {
    if (set.size() == 0)
        os << "(Empty)" << ::std::endl;
    else {
        os << "\n";
        for (size_t i = 0; i < set.size(); ++i)
            os << set[i] << ::std::endl;
    }
    return os;
}

//namespace test {
//namespace {

template <TagType tag_type, Tag tag, typename ValueT>
bool contains(hidl_vec<KeyParameter>& set, TypedTag<tag_type, tag> ttag, ValueT expected_value) {
    size_t count = std::count_if(set.begin(), set.end(), [&](const KeyParameter& param) {
        return param.tag == tag && accessTagValue(ttag, param) == expected_value;
    });
    return count == 1;
}

template <TagType tag_type, Tag tag>
bool contains(hidl_vec<KeyParameter>& set, TypedTag<tag_type, tag>) {
    size_t count = std::count_if(set.begin(), set.end(),
                                 [&](const KeyParameter& param) { return param.tag == tag; });
    return count > 0;
}

constexpr char hex_value[256] = {0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0,  //
                                 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0,  //
                                 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0,  //
                                 0, 1,  2,  3,  4,  5,  6,  7, 8, 9, 0, 0, 0, 0, 0, 0,  // '0'..'9'
                                 0, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 'A'..'F'
                                 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0,  //
                                 0, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 'a'..'f'
                                 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0,  //
                                 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0,  //
                                 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0,  //
                                 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0,  //
                                 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0,  //
                                 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0,  //
                                 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0,  //
                                 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0,  //
                                 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0};

string hex2str(string a) {
    string b;
    size_t num = a.size() / 2;
    b.resize(num);
    for (size_t i = 0; i < num; i++) {
        b[i] = (hex_value[a[i * 2] & 0xFF] << 4) + (hex_value[a[i * 2 + 1] & 0xFF]);
    }
    return b;
}

char nibble2hex[16] = {'0', '1', '2', '3', '4', '5', '6', '7',
                       '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

string bin2hex(const hidl_vec<uint8_t>& data) {
    string retval;
    retval.reserve(data.size() * 2 + 1);
    for (uint8_t byte : data) {
        retval.push_back(nibble2hex[0x0F & (byte >> 4)]);
        retval.push_back(nibble2hex[0x0F & byte]);
    }
    return retval;
}

string rsa_key = hex2str(
    "30820275020100300d06092a864886f70d01010105000482025f3082025b"
    "02010002818100c6095409047d8634812d5a218176e45c41d60a75b13901"
    "f234226cffe776521c5a77b9e389417b71c0b6a44d13afe4e4a2805d46c9"
    "da2935adb1ff0c1f24ea06e62b20d776430a4d435157233c6f916783c30e"
    "310fcbd89b85c2d56771169785ac12bca244abda72bfb19fc44d27c81e1d"
    "92de284f4061edfd99280745ea6d2502030100010281801be0f04d9cae37"
    "18691f035338308e91564b55899ffb5084d2460e6630257e05b3ceab0297"
    "2dfabcd6ce5f6ee2589eb67911ed0fac16e43a444b8c861e544a05933657"
    "72f8baf6b22fc9e3c5f1024b063ac080a7b2234cf8aee8f6c47bbf4fd3ac"
    "e7240290bef16c0b3f7f3cdd64ce3ab5912cf6e32f39ab188358afcccd80"
    "81024100e4b49ef50f765d3b24dde01aceaaf130f2c76670a91a61ae08af"
    "497b4a82be6dee8fcdd5e3f7ba1cfb1f0c926b88f88c92bfab137fba2285"
    "227b83c342ff7c55024100ddabb5839c4c7f6bf3d4183231f005b31aa58a"
    "ffdda5c79e4cce217f6bc930dbe563d480706c24e9ebfcab28a6cdefd324"
    "b77e1bf7251b709092c24ff501fd91024023d4340eda3445d8cd26c14411"
    "da6fdca63c1ccd4b80a98ad52b78cc8ad8beb2842c1d280405bc2f6c1bea"
    "214a1d742ab996b35b63a82a5e470fa88dbf823cdd02401b7b57449ad30d"
    "1518249a5f56bb98294d4b6ac12ffc86940497a5a5837a6cf946262b4945"
    "26d328c11e1126380fde04c24f916dec250892db09a6d77cdba351024077"
    "62cd8f4d050da56bd591adb515d24d7ccd32cca0d05f866d583514bd7324"
    "d5f33645e8ed8b4a1cb3cc4a1d67987399f2a09f5b3fb68c88d5e5d90ac3"
    "3492d6");

string ec_256_key = hex2str(
    "308187020100301306072a8648ce3d020106082a8648ce3d030107046d30"
    "6b0201010420737c2ecd7b8d1940bf2930aa9b4ed3ff941eed09366bc032"
    "99986481f3a4d859a14403420004bf85d7720d07c25461683bc648b4778a"
    "9a14dd8a024e3bdd8c7ddd9ab2b528bbc7aa1b51f14ebbbb0bd0ce21bcc4"
    "1c6eb00083cf3376d11fd44949e0b2183bfe");

string ec_521_key = hex2str(
    "3081EE020100301006072A8648CE3D020106052B810400230481D63081D3"
    "02010104420011458C586DB5DAA92AFAB03F4FE46AA9D9C3CE9A9B7A006A"
    "8384BEC4C78E8E9D18D7D08B5BCFA0E53C75B064AD51C449BAE0258D54B9"
    "4B1E885DED08ED4FB25CE9A1818903818600040149EC11C6DF0FA122C6A9"
    "AFD9754A4FA9513A627CA329E349535A5629875A8ADFBE27DCB932C05198"
    "6377108D054C28C6F39B6F2C9AF81802F9F326B842FF2E5F3C00AB7635CF"
    "B36157FC0882D574A10D839C1A0C049DC5E0D775E2EE50671A208431BB45"
    "E78E70BEFE930DB34818EE4D5C26259F5C6B8E28A652950F9F88D7B4B2C9"
    "D9");

const uint8_t root_cert[] = {
    0x30,0x82,0x05,0x60,0x30,0x82,0x03,0x48,0xA0,0x03,0x02,0x01,0x02,0x02,0x09,0x00,
    0xE8,0xFA,0x19,0x63,0x14,0xD2,0xFA,0x18,0x30,0x0D,0x06,0x09,0x2A,0x86,0x48,0x86,
    0xF7,0x0D,0x01,0x01,0x0B,0x05,0x00,0x30,0x1B,0x31,0x19,0x30,0x17,0x06,0x03,0x55,
    0x04,0x05,0x13,0x10,0x66,0x39,0x32,0x30,0x30,0x39,0x65,0x38,0x35,0x33,0x62,0x36,
    0x62,0x30,0x34,0x35,0x30,0x1E,0x17,0x0D,0x31,0x36,0x30,0x35,0x32,0x36,0x31,0x36,
    0x32,0x38,0x35,0x32,0x5A,0x17,0x0D,0x32,0x36,0x30,0x35,0x32,0x34,0x31,0x36,0x32,
    0x38,0x35,0x32,0x5A,0x30,0x1B,0x31,0x19,0x30,0x17,0x06,0x03,0x55,0x04,0x05,0x13,
    0x10,0x66,0x39,0x32,0x30,0x30,0x39,0x65,0x38,0x35,0x33,0x62,0x36,0x62,0x30,0x34,
    0x35,0x30,0x82,0x02,0x22,0x30,0x0D,0x06,0x09,0x2A,0x86,0x48,0x86,0xF7,0x0D,0x01,
    0x01,0x01,0x05,0x00,0x03,0x82,0x02,0x0F,0x00,0x30,0x82,0x02,0x0A,0x02,0x82,0x02,
    0x01,0x00,0xAF,0xB6,0xC7,0x82,0x2B,0xB1,0xA7,0x01,0xEC,0x2B,0xB4,0x2E,0x8B,0xCC,
    0x54,0x16,0x63,0xAB,0xEF,0x98,0x2F,0x32,0xC7,0x7F,0x75,0x31,0x03,0x0C,0x97,0x52,
    0x4B,0x1B,0x5F,0xE8,0x09,0xFB,0xC7,0x2A,0xA9,0x45,0x1F,0x74,0x3C,0xBD,0x9A,0x6F,
    0x13,0x35,0x74,0x4A,0xA5,0x5E,0x77,0xF6,0xB6,0xAC,0x35,0x35,0xEE,0x17,0xC2,0x5E,
    0x63,0x95,0x17,0xDD,0x9C,0x92,0xE6,0x37,0x4A,0x53,0xCB,0xFE,0x25,0x8F,0x8F,0xFB,
    0xB6,0xFD,0x12,0x93,0x78,0xA2,0x2A,0x4C,0xA9,0x9C,0x45,0x2D,0x47,0xA5,0x9F,0x32,
    0x01,0xF4,0x41,0x97,0xCA,0x1C,0xCD,0x7E,0x76,0x2F,0xB2,0xF5,0x31,0x51,0xB6,0xFE,
    0xB2,0xFF,0xFD,0x2B,0x6F,0xE4,0xFE,0x5B,0xC6,0xBD,0x9E,0xC3,0x4B,0xFE,0x08,0x23,
    0x9D,0xAA,0xFC,0xEB,0x8E,0xB5,0xA8,0xED,0x2B,0x3A,0xCD,0x9C,0x5E,0x3A,0x77,0x90,
    0xE1,0xB5,0x14,0x42,0x79,0x31,0x59,0x85,0x98,0x11,0xAD,0x9E,0xB2,0xA9,0x6B,0xBD,
    0xD7,0xA5,0x7C,0x93,0xA9,0x1C,0x41,0xFC,0xCD,0x27,0xD6,0x7F,0xD6,0xF6,0x71,0xAA,
    0x0B,0x81,0x52,0x61,0xAD,0x38,0x4F,0xA3,0x79,0x44,0x86,0x46,0x04,0xDD,0xB3,0xD8,
    0xC4,0xF9,0x20,0xA1,0x9B,0x16,0x56,0xC2,0xF1,0x4A,0xD6,0xD0,0x3C,0x56,0xEC,0x06,
    0x08,0x99,0x04,0x1C,0x1E,0xD1,0xA5,0xFE,0x6D,0x34,0x40,0xB5,0x56,0xBA,0xD1,0xD0,
    0xA1,0x52,0x58,0x9C,0x53,0xE5,0x5D,0x37,0x07,0x62,0xF0,0x12,0x2E,0xEF,0x91,0x86,
    0x1B,0x1B,0x0E,0x6C,0x4C,0x80,0x92,0x74,0x99,0xC0,0xE9,0xBE,0xC0,0xB8,0x3E,0x3B,
    0xC1,0xF9,0x3C,0x72,0xC0,0x49,0x60,0x4B,0xBD,0x2F,0x13,0x45,0xE6,0x2C,0x3F,0x8E,
    0x26,0xDB,0xEC,0x06,0xC9,0x47,0x66,0xF3,0xC1,0x28,0x23,0x9D,0x4F,0x43,0x12,0xFA,
    0xD8,0x12,0x38,0x87,0xE0,0x6B,0xEC,0xF5,0x67,0x58,0x3B,0xF8,0x35,0x5A,0x81,0xFE,
    0xEA,0xBA,0xF9,0x9A,0x83,0xC8,0xDF,0x3E,0x2A,0x32,0x2A,0xFC,0x67,0x2B,0xF1,0x20,
    0xB1,0x35,0x15,0x8B,0x68,0x21,0xCE,0xAF,0x30,0x9B,0x6E,0xEE,0x77,0xF9,0x88,0x33,
    0xB0,0x18,0xDA,0xA1,0x0E,0x45,0x1F,0x06,0xA3,0x74,0xD5,0x07,0x81,0xF3,0x59,0x08,
    0x29,0x66,0xBB,0x77,0x8B,0x93,0x08,0x94,0x26,0x98,0xE7,0x4E,0x0B,0xCD,0x24,0x62,
    0x8A,0x01,0xC2,0xCC,0x03,0xE5,0x1F,0x0B,0x3E,0x5B,0x4A,0xC1,0xE4,0xDF,0x9E,0xAF,
    0x9F,0xF6,0xA4,0x92,0xA7,0x7C,0x14,0x83,0x88,0x28,0x85,0x01,0x5B,0x42,0x2C,0xE6,
    0x7B,0x80,0xB8,0x8C,0x9B,0x48,0xE1,0x3B,0x60,0x7A,0xB5,0x45,0xC7,0x23,0xFF,0x8C,
    0x44,0xF8,0xF2,0xD3,0x68,0xB9,0xF6,0x52,0x0D,0x31,0x14,0x5E,0xBF,0x9E,0x86,0x2A,
    0xD7,0x1D,0xF6,0xA3,0xBF,0xD2,0x45,0x09,0x59,0xD6,0x53,0x74,0x0D,0x97,0xA1,0x2F,
    0x36,0x8B,0x13,0xEF,0x66,0xD5,0xD0,0xA5,0x4A,0x6E,0x2F,0x5D,0x9A,0x6F,0xEF,0x44,
    0x68,0x32,0xBC,0x67,0x84,0x47,0x25,0x86,0x1F,0x09,0x3D,0xD0,0xE6,0xF3,0x40,0x5D,
    0xA8,0x96,0x43,0xEF,0x0F,0x4D,0x69,0xB6,0x42,0x00,0x51,0xFD,0xB9,0x30,0x49,0x67,
    0x3E,0x36,0x95,0x05,0x80,0xD3,0xCD,0xF4,0xFB,0xD0,0x8B,0xC5,0x84,0x83,0x95,0x26,
    0x00,0x63,0x02,0x03,0x01,0x00,0x01,0xA3,0x81,0xA6,0x30,0x81,0xA3,0x30,0x1D,0x06,
    0x03,0x55,0x1D,0x0E,0x04,0x16,0x04,0x14,0x36,0x61,0xE1,0x00,0x7C,0x88,0x05,0x09,
    0x51,0x8B,0x44,0x6C,0x47,0xFF,0x1A,0x4C,0xC9,0xEA,0x4F,0x12,0x30,0x1F,0x06,0x03,
    0x55,0x1D,0x23,0x04,0x18,0x30,0x16,0x80,0x14,0x36,0x61,0xE1,0x00,0x7C,0x88,0x05,
    0x09,0x51,0x8B,0x44,0x6C,0x47,0xFF,0x1A,0x4C,0xC9,0xEA,0x4F,0x12,0x30,0x0F,0x06,
    0x03,0x55,0x1D,0x13,0x01,0x01,0xFF,0x04,0x05,0x30,0x03,0x01,0x01,0xFF,0x30,0x0E,
    0x06,0x03,0x55,0x1D,0x0F,0x01,0x01,0xFF,0x04,0x04,0x03,0x02,0x01,0x86,0x30,0x40,
    0x06,0x03,0x55,0x1D,0x1F,0x04,0x39,0x30,0x37,0x30,0x35,0xA0,0x33,0xA0,0x31,0x86,
    0x2F,0x68,0x74,0x74,0x70,0x73,0x3A,0x2F,0x2F,0x61,0x6E,0x64,0x72,0x6F,0x69,0x64,
    0x2E,0x67,0x6F,0x6F,0x67,0x6C,0x65,0x61,0x70,0x69,0x73,0x2E,0x63,0x6F,0x6D,0x2F,
    0x61,0x74,0x74,0x65,0x73,0x74,0x61,0x74,0x69,0x6F,0x6E,0x2F,0x63,0x72,0x6C,0x2F,
    0x30,0x0D,0x06,0x09,0x2A,0x86,0x48,0x86,0xF7,0x0D,0x01,0x01,0x0B,0x05,0x00,0x03,
    0x82,0x02,0x01,0x00,0x20,0xC8,0xC3,0x8D,0x4B,0xDC,0xA9,0x57,0x1B,0x46,0x8C,0x89,
    0x2F,0xFF,0x72,0xAA,0xC6,0xF8,0x44,0xA1,0x1D,0x41,0xA8,0xF0,0x73,0x6C,0xC3,0x7D,
    0x16,0xD6,0x42,0x6D,0x8E,0x7E,0x94,0x07,0x04,0x4C,0xEA,0x39,0xE6,0x8B,0x07,0xC1,
    0x3D,0xBF,0x15,0x03,0xDD,0x5C,0x85,0xBD,0xAF,0xB2,0xC0,0x2D,0x5F,0x6C,0xDB,0x4E,
    0xFA,0x81,0x27,0xDF,0x8B,0x04,0xF1,0x82,0x77,0x0F,0xC4,0xE7,0x74,0x5B,0x7F,0xCE,
    0xAA,0x87,0x12,0x9A,0x88,0x01,0xCE,0x8E,0x9B,0xC0,0xCB,0x96,0x37,0x9B,0x4D,0x26,
    0xA8,0x2D,0x30,0xFD,0x9C,0x2F,0x8E,0xED,0x6D,0xC1,0xBE,0x2F,0x84,0xB6,0x89,0xE4,
    0xD9,0x14,0x25,0x8B,0x14,0x4B,0xBA,0xE6,0x24,0xA1,0xC7,0x06,0x71,0x13,0x2E,0x2F,
    0x06,0x16,0xA8,0x84,0xB2,0xA4,0xD6,0xA4,0x6F,0xFA,0x89,0xB6,0x02,0xBF,0xBA,0xD8,
    0x0C,0x12,0x43,0x71,0x1F,0x56,0xEB,0x60,0x56,0xF6,0x37,0xC8,0xA0,0x14,0x1C,0xC5,
    0x40,0x94,0x26,0x8B,0x8C,0x3C,0x7D,0xB9,0x94,0xB3,0x5C,0x0D,0xCD,0x6C,0xB2,0xAB,
    0xC2,0xDA,0xFE,0xE2,0x52,0x02,0x3D,0x2D,0xEA,0x0C,0xD6,0xC3,0x68,0xBE,0xA3,0xE6,
    0x41,0x48,0x86,0xF6,0xB1,0xE5,0x8B,0x5B,0xD7,0xC7,0x30,0xB2,0x68,0xC4,0xE3,0xC1,
    0xFB,0x64,0x24,0xB9,0x1F,0xEB,0xBD,0xB8,0x0C,0x58,0x6E,0x2A,0xE8,0x36,0x8C,0x84,
    0xD5,0xD1,0x09,0x17,0xBD,0xA2,0x56,0x17,0x89,0xD4,0x68,0x73,0x93,0x34,0x0E,0x2E,
    0x25,0x4F,0x56,0x0E,0xF6,0x4B,0x23,0x58,0xFC,0xDC,0x0F,0xBF,0xC6,0x70,0x09,0x52,
    0xE7,0x08,0xBF,0xFC,0xC6,0x27,0x50,0x0C,0x1F,0x66,0xE8,0x1E,0xA1,0x7C,0x09,0x8D,
    0x7A,0x2E,0x9B,0x18,0x80,0x1B,0x7A,0xB4,0xAC,0x71,0x58,0x7D,0x34,0x5D,0xCC,0x83,
    0x09,0xD5,0xB6,0x2A,0x50,0x42,0x7A,0xA6,0xD0,0x3D,0xCB,0x05,0x99,0x6C,0x96,0xBA,
    0x0C,0x5D,0x71,0xE9,0x21,0x62,0xC0,0x16,0xCA,0x84,0x9F,0xF3,0x5F,0x0D,0x52,0xC6,
    0x5D,0x05,0x60,0x5A,0x47,0xF3,0xAE,0x91,0x7A,0xCD,0x2D,0xF9,0x10,0xEF,0xD2,0x32,
    0x66,0x88,0x59,0x6E,0xF6,0x9B,0x3B,0xF5,0xFE,0x31,0x54,0xF7,0xAE,0xB8,0x80,0xA0,
    0xA7,0x3C,0xA0,0x4D,0x94,0xC2,0xCE,0x83,0x17,0xEE,0xB4,0x3D,0x5E,0xFF,0x58,0x83,
    0xE3,0x36,0xF5,0xF2,0x49,0xDA,0xAC,0xA4,0x89,0x92,0x37,0xBF,0x26,0x7E,0x5C,0x43,
    0xAB,0x02,0xEA,0x44,0x16,0x24,0x03,0x72,0x3B,0xE6,0xAA,0x69,0x2C,0x61,0xBD,0xAE,
    0x9E,0xD4,0x09,0xD4,0x63,0xC4,0xC9,0x7C,0x64,0x30,0x65,0x77,0xEE,0xF2,0xBC,0x75,
    0x60,0xB7,0x57,0x15,0xCC,0x9C,0x7D,0xC6,0x7C,0x86,0x08,0x2D,0xB7,0x51,0xA8,0x9C,
    0x30,0x34,0x97,0x62,0xB0,0x78,0x23,0x85,0x87,0x5C,0xF1,0xA3,0xC6,0x16,0x6E,0x0A,
    0xE3,0xC1,0x2D,0x37,0x4E,0x2D,0x4F,0x18,0x46,0xF3,0x18,0x74,0x4B,0xD8,0x79,0xB5,
    0x87,0x32,0x9B,0xF0,0x18,0x21,0x7A,0x6C,0x0C,0x77,0x24,0x1A,0x48,0x78,0xE4,0x35,
    0xC0,0x30,0x79,0xCB,0x45,0x12,0x89,0xC5,0x77,0x62,0x06,0x06,0x9A,0x2F,0x8D,0x65,
    0xF8,0x40,0xE1,0x44,0x52,0x87,0xBE,0xD8,0x77,0xAB,0xAE,0x24,0xE2,0x44,0x35,0x16,
    0x8D,0x55,0x3C,0xE4,
};
struct RSA_Delete {
    void operator()(RSA* p) { RSA_free(p); }
};

X509* parse_cert_blob(const hidl_vec<uint8_t>& blob) {
    const uint8_t* p = blob.data();
    return d2i_X509(nullptr, &p, blob.size());
}

bool verify_chain(const hidl_vec<hidl_vec<uint8_t>>& chain) {
    for (size_t i = 0; i < chain.size(); ++i) {
        X509_Ptr key_cert(parse_cert_blob(chain[i]));
        X509_Ptr signing_cert;
        if (i < chain.size() - 1) {
            signing_cert.reset(parse_cert_blob(chain[i + 1]));
        } else {
            const uint8_t *tmp = root_cert;
            signing_cert.reset(d2i_X509(nullptr, &tmp, sizeof(root_cert)));
        }
        if (!key_cert.get() || !signing_cert.get()) {
            ALOGE("key_cert signing_cert not valid!");
            return false;
        }

        EVP_PKEY_Ptr signing_pubkey(X509_get_pubkey(signing_cert.get()));
        if (!signing_pubkey.get()) {
            ALOGE("signing_pubkey not valid!");
            return false;
        }

        int verify_result = X509_verify(key_cert.get(), signing_pubkey.get());
        if (verify_result != 1) {
            ALOGE("X509_verify %d not 1 Verification of certificate %d failed!",
                verify_result, i);
            return false;
        }

        char* cert_issuer =  //
            X509_NAME_oneline(X509_get_issuer_name(key_cert.get()), nullptr, 0);
        char* signer_subj =
            X509_NAME_oneline(X509_get_subject_name(signing_cert.get()), nullptr, 0);

        if (strcmp(cert_issuer, signer_subj) != 0) {
            ALOGE("Cert %d has wrong issuer.  (Possibly b/38394614)", i);
            return false;
        }

        if (i == 0) {
            char* cert_sub = X509_NAME_oneline(X509_get_subject_name(key_cert.get()), nullptr, 0);
            if (strcmp("/CN=Android Keystore Key", cert_sub) != 0) {
                ALOGE("Cert %d has wrong subject.  (Possibly b/38394614)", i);
                return false;
            }
            OPENSSL_free(cert_sub);
        }

        OPENSSL_free(cert_issuer);
        OPENSSL_free(signer_subj);

        if (dump_Attestations) std::cout << bin2hex(chain[i]) << std::endl;
    }

    return true;
}

// Extract attestation record from cert. Returned object is still part of cert; don't free it
// separately.
ASN1_OCTET_STRING* get_attestation_record(X509* certificate) {
    ASN1_OBJECT_Ptr oid(OBJ_txt2obj(kAttestionRecordOid, 1 /* dotted string format */));
    EXPECT_TRUE(!!oid.get());
    if (!oid.get()) return nullptr;

    int location = X509_get_ext_by_OBJ(certificate, oid.get(), -1 /* search from beginning */);
    EXPECT_NE(-1, location);
    if (location == -1) return nullptr;

    X509_EXTENSION* attest_rec_ext = X509_get_ext(certificate, location);
    EXPECT_TRUE(!!attest_rec_ext);
    if (!attest_rec_ext) return nullptr;

    ASN1_OCTET_STRING* attest_rec = X509_EXTENSION_get_data(attest_rec_ext);
    EXPECT_TRUE(!!attest_rec);
    return attest_rec;
}

bool tag_in_list(const KeyParameter& entry) {
    // Attestations don't contain everything in key authorization lists, so we need to filter
    // the key lists to produce the lists that we expect to match the attestations.
    auto tag_list = {
        Tag::USER_ID, Tag::INCLUDE_UNIQUE_ID, Tag::BLOB_USAGE_REQUIREMENTS,
        Tag::EC_CURVE /* Tag::EC_CURVE will be included by KM2 implementations */,
    };
    return std::find(tag_list.begin(), tag_list.end(), entry.tag) != tag_list.end();
}

AuthorizationSet filter_tags(const AuthorizationSet& set) {
    AuthorizationSet filtered;
    std::remove_copy_if(set.begin(), set.end(), std::back_inserter(filtered), tag_in_list);
    return filtered;
}

std::string make_string(const uint8_t* data, size_t length) {
    return std::string(reinterpret_cast<const char*>(data), length);
}

template <size_t N> std::string make_string(const uint8_t (&a)[N]) {
    return make_string(a, N);
}

class HidlBuf : public hidl_vec<uint8_t> {
    typedef hidl_vec<uint8_t> super;

  public:
    HidlBuf() {}
    HidlBuf(const super& other) : super(other) {}
    HidlBuf(super&& other) : super(std::move(other)) {}
    explicit HidlBuf(const std::string& other) : HidlBuf() { *this = other; }

    HidlBuf& operator=(const super& other) {
        super::operator=(other);
        return *this;
    }

    HidlBuf& operator=(super&& other) {
        super::operator=(std::move(other));
        return *this;
    }

    HidlBuf& operator=(const string& other) {
        resize(other.size());
        for (size_t i = 0; i < other.size(); ++i) {
            (*this)[i] = static_cast<uint8_t>(other[i]);
        }
        return *this;
    }

    string to_string() const { return string(reinterpret_cast<const char*>(data()), size()); }
};

constexpr uint64_t kOpHandleSentinel = 0xFFFFFFFFFFFFFFFF;

//}  // namespace

// Test environment for Keymaster HIDL HAL.
class KeymasterHidlEnvironment : public ::testing::VtsHalHidlTargetTestEnvBase {
   public:
    // get the test environment singleton
    static KeymasterHidlEnvironment* Instance() {
        static KeymasterHidlEnvironment* instance = new KeymasterHidlEnvironment;
        return instance;
    }

    virtual void registerTestServices() override { registerTestService<IKeymasterDevice>(); }
   private:
    KeymasterHidlEnvironment() {}
};

class KeymasterHidlTest : public ::testing::VtsHalHidlTargetTestBase {
  public:
    void TearDown() override {
        if (key_blob_.size()) {
            CheckedDeleteKey();
        }
        AbortIfNeeded();
    }
    KeymasterHidlTest() {}
    ~KeymasterHidlTest() {}
    KeymasterHidlTest(const KeymasterHidlTest& obj) {}

    // SetUpTestCase runs only once per test case, not once per test.
    static void SetUpTestCase() {
        keymaster_ = ::android::hardware::keymaster::V3_0::IKeymasterDevice::getService();
        if (keymaster_ == NULL) {
            ALOGE("keymaster NULL!");
            return;
        }
        bool isOk =
            keymaster_
                ->getHardwareFeatures([&](bool isSecure, bool supportsEc, bool supportsSymmetric,
                                          bool supportsAttestation, bool supportsAllDigests,
                                          const hidl_string& name, const hidl_string& author) {
                    is_secure_ = isSecure;
                    supports_ec_ = supportsEc;
                    supports_symmetric_ = supportsSymmetric;
                    supports_attestation_ = supportsAttestation;
                    supports_all_digests_ = supportsAllDigests;
                    name_ = name;
                    author_ = author;
                    ALOGI("name %s", name.c_str());
                })
                .isOk();
        if (!isOk) {
            ALOGE("keymaster getHardwareFeatures not ok!");
        }

        os_version_ = ::keymaster::GetOsVersion();
        os_patch_level_ = ::keymaster::GetOsPatchlevel();
    }

    static void TearDownTestCase() { keymaster_.clear(); }

    static IKeymasterDevice& keymaster() { return *keymaster_; }
    static uint32_t os_version() { return os_version_; }
    static uint32_t os_patch_level() { return os_patch_level_; }

    AuthorizationSet UserAuths() { return AuthorizationSetBuilder().Authorization(TAG_USER_ID, 7); }

    ErrorCode GenerateKey(const AuthorizationSet& key_desc, HidlBuf* key_blob,
                          KeyCharacteristics* key_characteristics) {
        ErrorCode error;

        if (key_blob == NULL || key_characteristics == NULL) {
            ALOGE("GenerateKey key_desc key_blob key_characteristics NULL!");
            return ErrorCode::UNKNOWN_ERROR;
        }

        bool isOk = keymaster_
                        ->generateKey(key_desc.hidl_data(),
                                      [&](ErrorCode hidl_error, const HidlBuf& hidl_key_blob,
                                          const KeyCharacteristics& hidl_key_characteristics) {
                                          error = hidl_error;
                                          *key_blob = hidl_key_blob;
                                          *key_characteristics = hidl_key_characteristics;
                                      })
                        .isOk();
        if (!isOk) {
            ALOGE("GenerateKey keymaster_ generateKey not ok!");
            return ErrorCode::UNKNOWN_ERROR;
        }
        ALOGD("GenerateKey errorcode %d ok code %d", error, ErrorCode::OK);
        // On error, blob & characteristics should be empty.
        return error;
    }

    ErrorCode GenerateKey(const AuthorizationSet& key_desc) {
        return GenerateKey(key_desc, &key_blob_, &key_characteristics_);
    }

    ErrorCode ImportKey(const AuthorizationSet& key_desc, KeyFormat format,
                        const string& key_material, HidlBuf* key_blob,
                        KeyCharacteristics* key_characteristics) {
        ErrorCode error;
        EXPECT_TRUE(keymaster_
                        ->importKey(key_desc.hidl_data(), format, HidlBuf(key_material),
                                    [&](ErrorCode hidl_error, const HidlBuf& hidl_key_blob,
                                        const KeyCharacteristics& hidl_key_characteristics) {
                                        error = hidl_error;
                                        *key_blob = hidl_key_blob;
                                        *key_characteristics = hidl_key_characteristics;
                                    })
                        .isOk());
        // On error, blob & characteristics should be empty.
        if (error != ErrorCode::OK) {
            EXPECT_EQ(0U, key_blob->size());
            EXPECT_EQ(0U, (key_characteristics->softwareEnforced.size() +
                           key_characteristics->teeEnforced.size()));
        }
        return error;
    }

    ErrorCode ImportKey(const AuthorizationSet& key_desc, KeyFormat format,
                        const string& key_material) {
        return ImportKey(key_desc, format, key_material, &key_blob_, &key_characteristics_);
    }

    ErrorCode ExportKey(KeyFormat format, const HidlBuf& key_blob, const HidlBuf& client_id,
                        const HidlBuf& app_data, HidlBuf* key_material) {
        ErrorCode error;
        EXPECT_TRUE(
            keymaster_
                ->exportKey(format, key_blob, client_id, app_data,
                            [&](ErrorCode hidl_error_code, const HidlBuf& hidl_key_material) {
                                error = hidl_error_code;
                                *key_material = hidl_key_material;
                            })
                .isOk());
        // On error, blob should be empty.
        if (error != ErrorCode::OK) {
            EXPECT_EQ(0U, key_material->size());
        }
        return error;
    }

    ErrorCode ExportKey(KeyFormat format, HidlBuf* key_material) {
        HidlBuf client_id, app_data;
        return ExportKey(format, key_blob_, client_id, app_data, key_material);
    }

    ErrorCode DeleteKey(HidlBuf* key_blob, bool keep_key_blob = false) {
        auto rc = keymaster_->deleteKey(*key_blob);
        if (!keep_key_blob) *key_blob = HidlBuf();
        if (!rc.isOk()) return ErrorCode::UNKNOWN_ERROR;
        return rc;
    }

    ErrorCode DeleteKey(bool keep_key_blob = false) {
        return DeleteKey(&key_blob_, keep_key_blob);
    }

    ErrorCode DeleteAllKeys() {
        ErrorCode error = keymaster_->deleteAllKeys();
        return error;
    }

    void CheckedDeleteKey(HidlBuf* key_blob, bool keep_key_blob = false) {
        auto rc = DeleteKey(key_blob, keep_key_blob);
        EXPECT_TRUE(rc == ErrorCode::OK || rc == ErrorCode::UNIMPLEMENTED);
    }

    void CheckedDeleteKey() { CheckedDeleteKey(&key_blob_); }

    ErrorCode GetCharacteristics(const HidlBuf& key_blob, const HidlBuf& client_id,
                                 const HidlBuf& app_data, KeyCharacteristics* key_characteristics) {
        ErrorCode error = ErrorCode::UNKNOWN_ERROR;
        EXPECT_TRUE(
            keymaster_
                ->getKeyCharacteristics(
                    key_blob, client_id, app_data,
                    [&](ErrorCode hidl_error, const KeyCharacteristics& hidl_key_characteristics) {
                        error = hidl_error, *key_characteristics = hidl_key_characteristics;
                    })
                .isOk());
        return error;
    }

    ErrorCode GetCharacteristics(const HidlBuf& key_blob, KeyCharacteristics* key_characteristics) {
        HidlBuf client_id, app_data;
        return GetCharacteristics(key_blob, client_id, app_data, key_characteristics);
    }

    ErrorCode Begin(KeyPurpose purpose, const HidlBuf& key_blob, const AuthorizationSet& in_params,
                    AuthorizationSet* out_params, OperationHandle* op_handle) {
        SCOPED_TRACE("Begin");
        ErrorCode error;
        OperationHandle saved_handle = *op_handle;
        EXPECT_TRUE(
            keymaster_
                ->begin(purpose, key_blob, in_params.hidl_data(),
                        [&](ErrorCode hidl_error, const hidl_vec<KeyParameter>& hidl_out_params,
                            uint64_t hidl_op_handle) {
                            error = hidl_error;
                            *out_params = hidl_out_params;
                            *op_handle = hidl_op_handle;
                        })
                .isOk());
        if (error != ErrorCode::OK) {
            // Some implementations may modify *op_handle on error.
            *op_handle = saved_handle;
        }
        return error;
    }

    ErrorCode Begin(KeyPurpose purpose, const AuthorizationSet& in_params,
                    AuthorizationSet* out_params) {
        SCOPED_TRACE("Begin");
        EXPECT_EQ(kOpHandleSentinel, op_handle_);
        return Begin(purpose, key_blob_, in_params, out_params, &op_handle_);
    }

    ErrorCode Begin(KeyPurpose purpose, const AuthorizationSet& in_params) {
        SCOPED_TRACE("Begin");
        AuthorizationSet out_params;
        ErrorCode error = Begin(purpose, in_params, &out_params);
        EXPECT_TRUE(out_params.empty());
        return error;
    }

    ErrorCode Update(OperationHandle op_handle, const AuthorizationSet& in_params,
                     const string& input, AuthorizationSet* out_params, string* output,
                     size_t* input_consumed) {
        SCOPED_TRACE("Update");
        ErrorCode error;
        EXPECT_TRUE(keymaster_
                        ->update(op_handle, in_params.hidl_data(), HidlBuf(input),
                                 [&](ErrorCode hidl_error, uint32_t hidl_input_consumed,
                                     const hidl_vec<KeyParameter>& hidl_out_params,
                                     const HidlBuf& hidl_output) {
                                     error = hidl_error;
                                     out_params->push_back(AuthorizationSet(hidl_out_params));
                                     output->append(hidl_output.to_string());
                                     *input_consumed = hidl_input_consumed;
                                 })
                        .isOk());
        return error;
    }

    ErrorCode Update(const string& input, string* out, size_t* input_consumed) {
        SCOPED_TRACE("Update");
        AuthorizationSet out_params;
        ErrorCode error = Update(op_handle_, AuthorizationSet() /* in_params */, input, &out_params,
                                 out, input_consumed);
        EXPECT_TRUE(out_params.empty());
        return error;
    }

    ErrorCode Finish(OperationHandle op_handle, const AuthorizationSet& in_params,
                     const string& input, const string& signature, AuthorizationSet* out_params,
                     string* output) {
        SCOPED_TRACE("Finish");
        ErrorCode error;
        EXPECT_TRUE(
            keymaster_
                ->finish(op_handle, in_params.hidl_data(), HidlBuf(input), HidlBuf(signature),
                         [&](ErrorCode hidl_error, const hidl_vec<KeyParameter>& hidl_out_params,
                             const HidlBuf& hidl_output) {
                             error = hidl_error;
                             *out_params = hidl_out_params;
                             output->append(hidl_output.to_string());
                         })
                .isOk());
        op_handle_ = kOpHandleSentinel;  // So dtor doesn't Abort().
        return error;
    }

    ErrorCode Finish(const string& message, string* output) {
        SCOPED_TRACE("Finish");
        AuthorizationSet out_params;
        string finish_output;
        ErrorCode error = Finish(op_handle_, AuthorizationSet() /* in_params */, message,
                                 "" /* signature */, &out_params, output);
        if (error != ErrorCode::OK) {
            return error;
        }
        EXPECT_EQ(0U, out_params.size());
        return error;
    }

    ErrorCode Finish(const string& message, const string& signature, string* output) {
        SCOPED_TRACE("Finish");
        AuthorizationSet out_params;
        ErrorCode error = Finish(op_handle_, AuthorizationSet() /* in_params */, message, signature,
                                 &out_params, output);
        op_handle_ = kOpHandleSentinel;  // So dtor doesn't Abort().
        if (error != ErrorCode::OK) {
            return error;
        }
        EXPECT_EQ(0U, out_params.size());
        return error;
    }

    ErrorCode Abort(OperationHandle op_handle) {
        SCOPED_TRACE("Abort");
        auto retval = keymaster_->abort(op_handle);
        EXPECT_TRUE(retval.isOk());
        return retval;
    }

    void AbortIfNeeded() {
        SCOPED_TRACE("AbortIfNeeded");
        if (op_handle_ != kOpHandleSentinel) {
            EXPECT_EQ(ErrorCode::OK, Abort(op_handle_));
            op_handle_ = kOpHandleSentinel;
        }
    }

    ErrorCode AttestKey(const HidlBuf& key_blob, const AuthorizationSet& attest_params,
                        hidl_vec<hidl_vec<uint8_t>>* cert_chain) {
        ErrorCode error;
        auto rc = keymaster_->attestKey(
            key_blob, attest_params.hidl_data(),
            [&](ErrorCode hidl_error, const hidl_vec<hidl_vec<uint8_t>>& hidl_cert_chain) {
                error = hidl_error;
                *cert_chain = hidl_cert_chain;
            });
        if (!rc.isOk()) {
            ALOGE("AttestKey %s unkown", key_blob.to_string().c_str());
            error = ErrorCode::UNKNOWN_ERROR;
        }

        return error;
    }

    ErrorCode AttestKey(const AuthorizationSet& attest_params,
                        hidl_vec<hidl_vec<uint8_t>>* cert_chain) {
        return AttestKey(key_blob_, attest_params, cert_chain);
    }

    string ProcessMessage(const HidlBuf& key_blob, KeyPurpose operation, const string& message,
                          const AuthorizationSet& in_params, AuthorizationSet* out_params) {
        SCOPED_TRACE("ProcessMessage");
        AuthorizationSet begin_out_params;
        EXPECT_EQ(ErrorCode::OK,
                  Begin(operation, key_blob, in_params, &begin_out_params, &op_handle_));

        string unused;
        AuthorizationSet finish_params;
        AuthorizationSet finish_out_params;
        string output;
        EXPECT_EQ(ErrorCode::OK,
                  Finish(op_handle_, finish_params, message, unused, &finish_out_params, &output));
        op_handle_ = kOpHandleSentinel;

        out_params->push_back(begin_out_params);
        out_params->push_back(finish_out_params);
        return output;
    }

    string SignMessage(const HidlBuf& key_blob, const string& message,
                       const AuthorizationSet& params) {
        SCOPED_TRACE("SignMessage");
        AuthorizationSet out_params;
        string signature = ProcessMessage(key_blob, KeyPurpose::SIGN, message, params, &out_params);
        EXPECT_TRUE(out_params.empty());
        return signature;
    }

    string SignMessage(const string& message, const AuthorizationSet& params) {
        SCOPED_TRACE("SignMessage");
        return SignMessage(key_blob_, message, params);
    }

    string MacMessage(const string& message, Digest digest, size_t mac_length) {
        SCOPED_TRACE("MacMessage");
        return SignMessage(
            key_blob_, message,
            AuthorizationSetBuilder().Digest(digest).Authorization(TAG_MAC_LENGTH, mac_length));
    }

    void CheckHmacTestVector(const string& key, const string& message, Digest digest,
                             const string& expected_mac) {
        SCOPED_TRACE("CheckHmacTestVector");
        ASSERT_EQ(ErrorCode::OK,
                  ImportKey(AuthorizationSetBuilder()
                                .Authorization(TAG_NO_AUTH_REQUIRED)
                                .HmacKey(key.size() * 8)
                                .Authorization(TAG_MIN_MAC_LENGTH, expected_mac.size() * 8)
                                .Digest(digest),
                            KeyFormat::RAW, key));
        string signature = MacMessage(message, digest, expected_mac.size() * 8);
        EXPECT_EQ(expected_mac, signature) << "Test vector didn't match for digest " << (int)digest;
        CheckedDeleteKey();
    }

    void CheckAesCtrTestVector(const string& key, const string& nonce, const string& message,
                               const string& expected_ciphertext) {
        SCOPED_TRACE("CheckAesCtrTestVector");
        ASSERT_EQ(ErrorCode::OK, ImportKey(AuthorizationSetBuilder()
                                               .Authorization(TAG_NO_AUTH_REQUIRED)
                                               .AesEncryptionKey(key.size() * 8)
                                               .BlockMode(BlockMode::CTR)
                                               .Authorization(TAG_CALLER_NONCE)
                                               .Padding(PaddingMode::NONE),
                                           KeyFormat::RAW, key));

        auto params = AuthorizationSetBuilder()
                          .Authorization(TAG_NONCE, nonce.data(), nonce.size())
                          .BlockMode(BlockMode::CTR)
                          .Padding(PaddingMode::NONE);
        AuthorizationSet out_params;
        string ciphertext = EncryptMessage(key_blob_, message, params, &out_params);
        EXPECT_EQ(expected_ciphertext, ciphertext);
    }

    void VerifyMessage(const HidlBuf& key_blob, const string& message, const string& signature,
                       const AuthorizationSet& params) {
        SCOPED_TRACE("VerifyMessage");
        AuthorizationSet begin_out_params;
        ASSERT_EQ(ErrorCode::OK,
                  Begin(KeyPurpose::VERIFY, key_blob, params, &begin_out_params, &op_handle_));

        string unused;
        AuthorizationSet finish_params;
        AuthorizationSet finish_out_params;
        string output;
        EXPECT_EQ(ErrorCode::OK, Finish(op_handle_, finish_params, message, signature,
                                        &finish_out_params, &output));
        op_handle_ = kOpHandleSentinel;
        EXPECT_TRUE(output.empty());
    }

    void VerifyMessage(const string& message, const string& signature,
                       const AuthorizationSet& params) {
        SCOPED_TRACE("VerifyMessage");
        VerifyMessage(key_blob_, message, signature, params);
    }

    string EncryptMessage(const HidlBuf& key_blob, const string& message,
                          const AuthorizationSet& in_params, AuthorizationSet* out_params) {
        SCOPED_TRACE("EncryptMessage");
        return ProcessMessage(key_blob, KeyPurpose::ENCRYPT, message, in_params, out_params);
    }

    string EncryptMessage(const string& message, const AuthorizationSet& params,
                          AuthorizationSet* out_params) {
        SCOPED_TRACE("EncryptMessage");
        return EncryptMessage(key_blob_, message, params, out_params);
    }

    string EncryptMessage(const string& message, const AuthorizationSet& params) {
        SCOPED_TRACE("EncryptMessage");
        AuthorizationSet out_params;
        string ciphertext = EncryptMessage(message, params, &out_params);
        EXPECT_TRUE(out_params.empty())
            << "Output params should be empty. Contained: " << out_params;
        return ciphertext;
    }

    string DecryptMessage(const HidlBuf& key_blob, const string& ciphertext,
                          const AuthorizationSet& params) {
        SCOPED_TRACE("DecryptMessage");
        AuthorizationSet out_params;
        string plaintext =
            ProcessMessage(key_blob, KeyPurpose::DECRYPT, ciphertext, params, &out_params);
        EXPECT_TRUE(out_params.empty());
        return plaintext;
    }

    string DecryptMessage(const string& ciphertext, const AuthorizationSet& params) {
        SCOPED_TRACE("DecryptMessage");
        return DecryptMessage(key_blob_, ciphertext, params);
    }

    template <TagType tag_type, Tag tag, typename ValueT>
    void CheckKm0CryptoParam(TypedTag<tag_type, tag> ttag, ValueT expected) {
        SCOPED_TRACE("CheckKm0CryptoParam");
        if (is_secure_) {
            EXPECT_TRUE(contains(key_characteristics_.teeEnforced, ttag, expected));
            EXPECT_FALSE(contains(key_characteristics_.softwareEnforced, ttag));
        } else {
            EXPECT_TRUE(contains(key_characteristics_.softwareEnforced, ttag, expected));
            EXPECT_FALSE(contains(key_characteristics_.teeEnforced, ttag));
        }
    }

    template <TagType tag_type, Tag tag, typename ValueT>
    void CheckKm1CryptoParam(TypedTag<tag_type, tag> ttag, ValueT expected) {
        SCOPED_TRACE("CheckKm1CryptoParam");
        if (is_secure_ && supports_symmetric_) {
            EXPECT_TRUE(contains(key_characteristics_.teeEnforced, ttag, expected));
            EXPECT_FALSE(contains(key_characteristics_.softwareEnforced, ttag));
        } else {
            EXPECT_TRUE(contains(key_characteristics_.softwareEnforced, ttag, expected));
            EXPECT_FALSE(contains(key_characteristics_.teeEnforced, ttag));
        }
    }

    template <TagType tag_type, Tag tag, typename ValueT>
    void CheckKm2CryptoParam(TypedTag<tag_type, tag> ttag, ValueT expected) {
        SCOPED_TRACE("CheckKm2CryptoParam");
        if (supports_attestation_) {
            EXPECT_TRUE(contains(key_characteristics_.teeEnforced, ttag, expected));
            EXPECT_FALSE(contains(key_characteristics_.softwareEnforced, ttag));
        } else if (!supports_symmetric_ /* KM version < 1 or SW */) {
            EXPECT_TRUE(contains(key_characteristics_.softwareEnforced, ttag, expected));
            EXPECT_FALSE(contains(key_characteristics_.teeEnforced, ttag));
        }
    }

    void CheckOrigin(bool asymmetric = false) {
        SCOPED_TRACE("CheckOrigin");
        if (is_secure_ && supports_symmetric_) {
            EXPECT_TRUE(
                contains(key_characteristics_.teeEnforced, TAG_ORIGIN, KeyOrigin::IMPORTED));
        } else if (is_secure_) {
            // wrapped KM0
            if (asymmetric) {
                EXPECT_TRUE(
                    contains(key_characteristics_.teeEnforced, TAG_ORIGIN, KeyOrigin::UNKNOWN));
            } else {
                EXPECT_TRUE(contains(key_characteristics_.softwareEnforced, TAG_ORIGIN,
                                     KeyOrigin::IMPORTED));
            }
        } else {
            EXPECT_TRUE(
                contains(key_characteristics_.softwareEnforced, TAG_ORIGIN, KeyOrigin::IMPORTED));
        }
    }

    static bool IsSecure() { return is_secure_; }
    static bool SupportsEc() { return supports_ec_; }
    static bool SupportsSymmetric() { return supports_symmetric_; }
    static bool SupportsAllDigests() { return supports_all_digests_; }
    static bool SupportsAttestation() { return supports_attestation_; }

    static bool Km2Profile() {
        return SupportsAttestation() && SupportsAllDigests() && SupportsSymmetric() &&
               SupportsEc() && IsSecure();
    }

    static bool Km1Profile() {
        return !SupportsAttestation() && SupportsSymmetric() && SupportsEc() && IsSecure();
    }

    static bool Km0Profile() {
        return !SupportsAttestation() && !SupportsAllDigests() && !SupportsSymmetric() &&
               IsSecure();
    }

    static bool SwOnlyProfile() {
        return !SupportsAttestation() && !SupportsAllDigests() && !SupportsSymmetric() &&
               !SupportsEc() && !IsSecure();
    }

    HidlBuf key_blob_;
    KeyCharacteristics key_characteristics_;
    OperationHandle op_handle_ = kOpHandleSentinel;

  private:

    void TestBody() {}
    static sp<IKeymasterDevice> keymaster_;
    static uint32_t os_version_;
    static uint32_t os_patch_level_;

    static bool is_secure_;
    static bool supports_ec_;
    static bool supports_symmetric_;
    static bool supports_attestation_;
    static bool supports_all_digests_;
    static hidl_string name_;
    static hidl_string author_;
};

bool verify_attestation_record(const string& challenge, const string& app_id,
                               AuthorizationSet expected_sw_enforced,
                               AuthorizationSet expected_tee_enforced,
                               const hidl_vec<uint8_t>& attestation_cert) {
    X509_Ptr cert(parse_cert_blob(attestation_cert));
    if (!cert.get()) {
        ALOGE("cert not valid!");
        return false;
    }

    ASN1_OCTET_STRING* attest_rec = get_attestation_record(cert.get());
    if (!attest_rec) {
        ALOGE("cert not valid!");
        return false;
    }

    AuthorizationSet att_sw_enforced;
    AuthorizationSet att_tee_enforced;
    uint32_t att_attestation_version;
    uint32_t att_keymaster_version;
    SecurityLevel att_attestation_security_level;
    SecurityLevel att_keymaster_security_level;
    HidlBuf att_challenge;
    HidlBuf att_unique_id;
    HidlBuf att_app_id;
    if (ErrorCode::OK !=
              parse_attestation_record(attest_rec->data,                 //
                                       attest_rec->length,               //
                                       &att_attestation_version,         //
                                       &att_attestation_security_level,  //
                                       &att_keymaster_version,           //
                                       &att_keymaster_security_level,    //
                                       &att_challenge,                   //
                                       &att_sw_enforced,                 //
                                       &att_tee_enforced,                //
                                       &att_unique_id)) {
        ALOGE("parse_attestation_record not ok!");
        return false;
    }

    if (att_attestation_version != 1 && att_attestation_version != 2) {
        ALOGE("att_attestation_version %u is not 1 or 2!", att_attestation_version);
        return false;
    }

    expected_sw_enforced.push_back(TAG_ATTESTATION_APPLICATION_ID,
                                   HidlBuf(app_id));

    if (!KeymasterHidlTest::IsSecure()) {
        // SW is KM3
        if (att_keymaster_version != 3U) {
            ALOGE("att_keymaster_version %u is not 3!", att_keymaster_version);
            return false;
        }
    }

    if (KeymasterHidlTest::SupportsSymmetric()) {
        if (att_keymaster_version < 1U) {
            ALOGE("att_keymaster_version %u is less than 1!", att_keymaster_version);
            return false;
        }
    }

    if (KeymasterHidlTest::SupportsAttestation()) {
        if (att_keymaster_version < 2U) {
            ALOGE("att_keymaster_version %u is less than 2!", att_keymaster_version);
            return false;
        }
    }

    SecurityLevel sl;
    if (KeymasterHidlTest::IsSecure()) {
        sl = SecurityLevel::TRUSTED_ENVIRONMENT;
    } else {
        sl = SecurityLevel::SOFTWARE;
    }
    if (sl != att_keymaster_security_level) {
        ALOGE("att_keymaster_security_level is not right!");
        return false;
    }

    if (KeymasterHidlTest::SupportsAttestation()) {
        sl = SecurityLevel::TRUSTED_ENVIRONMENT;
    } else {
        sl = SecurityLevel::SOFTWARE;

    }
    if (sl != att_attestation_security_level) {
        ALOGE("att_attestation_security_level is not right!");
        return false;
    }

    if (challenge.length() != att_challenge.size()) {
        ALOGE("challenge's length %d not equal att_challenge's %d!", challenge.length(), att_challenge.size());
        return false;
    }

    if (0 != memcmp(challenge.data(), att_challenge.data(), challenge.length())) {
        ALOGE("memcmp challenge att_challenge not ok!");
        return false;
    }

    att_sw_enforced.Sort();
    expected_sw_enforced.Sort();
    if (!(filter_tags(expected_sw_enforced) == filter_tags(att_sw_enforced))) {
        ALOGE("expected_sw_enforced not equal att_sw_enforced Possibly b/38394619!");
        return false;
    }

    att_tee_enforced.Sort();
    expected_tee_enforced.Sort();
    if (!(filter_tags(expected_tee_enforced) == filter_tags(att_tee_enforced))) {
        ALOGE("expected_tee_enforced not equal att_tee_enforced Possibly b/38394619!");
        return false;
    }

    return true;
}

sp<IKeymasterDevice> KeymasterHidlTest::keymaster_;
uint32_t KeymasterHidlTest::os_version_;
uint32_t KeymasterHidlTest::os_patch_level_;
bool KeymasterHidlTest::is_secure_;
bool KeymasterHidlTest::supports_ec_;
bool KeymasterHidlTest::supports_symmetric_;
bool KeymasterHidlTest::supports_all_digests_;
bool KeymasterHidlTest::supports_attestation_;
hidl_string KeymasterHidlTest::name_;
hidl_string KeymasterHidlTest::author_;

typedef KeymasterHidlTest AttestationTest;

/*
 * AttestationTest.RsaAttestation
 *
 * Verifies that attesting to RSA keys works and generates the expected output.
 */
bool RsaAttestation(AttestationTest test) {
    if (ErrorCode::OK != test.GenerateKey(AuthorizationSetBuilder()
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .RsaSigningKey(1024, 3)
                                             .Digest(Digest::NONE)
                                             .Padding(PaddingMode::NONE)
                                             .Authorization(TAG_INCLUDE_UNIQUE_ID))) {
        ALOGE("RsaAttestation GenerateKey failed!!!");
        return false;
    }

    hidl_vec<hidl_vec<uint8_t>> cert_chain;
    if (ErrorCode::OK !=
              test.AttestKey(AuthorizationSetBuilder()
                            .Authorization(TAG_ATTESTATION_CHALLENGE, HidlBuf("challenge"))
                            .Authorization(TAG_ATTESTATION_APPLICATION_ID, HidlBuf("foo")),
                        &cert_chain)) {
        ALOGE("RsaAttestation AttestKey failed!!!");
        return false;
    }

    if (cert_chain.size() < 2U) {
        ALOGE("RsaAttestation cert_chain size failed! actual size %u", cert_chain.size());
        return false;
    }

    if (!verify_chain(cert_chain)) {
        ALOGE("RsaAttestation verify_chain failed!");
        return false;
    }

    if (!
        verify_attestation_record("challenge", "foo",                     //
                                  test.key_characteristics_.softwareEnforced,  //
                                  test.key_characteristics_.teeEnforced,       //
                                  cert_chain[0])) {
        ALOGE("RsaAttestation verify_attestation_record failed!");
        return false;
    }
    return true;
}

/*
 * AttestationTest.RsaAttestationRequiresAppId
 *
 * Verifies that attesting to RSA requires app ID.
 */
bool RsaAttestationRequiresAppId(AttestationTest test) {
    if (ErrorCode::OK !=
              test.GenerateKey(AuthorizationSetBuilder()
                              .Authorization(TAG_NO_AUTH_REQUIRED)
                              .RsaSigningKey(1024, 3)
                              .Digest(Digest::NONE)
                              .Padding(PaddingMode::NONE)
                              .Authorization(TAG_INCLUDE_UNIQUE_ID))) {
        ALOGE("RsaAttestationRequiresAppId GenerateKey failed!!!");
        return false;
    }

    hidl_vec<hidl_vec<uint8_t>> cert_chain;
    if (ErrorCode::ATTESTATION_APPLICATION_ID_MISSING !=
              test.AttestKey(AuthorizationSetBuilder().Authorization(
                            TAG_ATTESTATION_CHALLENGE, HidlBuf("challenge")),
                        &cert_chain)) {
        ALOGE("RsaAttestationRequiresAppId AttestKey!");
    }
    return true;
}

/*
 * AttestationTest.EcAttestation
 *
 * Verifies that attesting to EC keys works and generates the expected output.
 */
bool EcAttestation(AttestationTest test) {
    if (ErrorCode::OK != test.GenerateKey(AuthorizationSetBuilder()
                                             .Authorization(TAG_NO_AUTH_REQUIRED)
                                             .EcdsaSigningKey(EcCurve::P_256)
                                             .Digest(Digest::SHA_2_256)
                                             .Authorization(TAG_INCLUDE_UNIQUE_ID))) {
        ALOGE("EcAttestation GenerateKey failed!!!");
        return false;
    }

    hidl_vec<hidl_vec<uint8_t>> cert_chain;
    if (ErrorCode::OK !=
              test.AttestKey(AuthorizationSetBuilder()
                            .Authorization(TAG_ATTESTATION_CHALLENGE, HidlBuf("challenge"))
                            .Authorization(TAG_ATTESTATION_APPLICATION_ID, HidlBuf("foo")),
                        &cert_chain)){
        ALOGE("EcAttestation AttestKey failed!!!");
        return false;
    }

    if (cert_chain.size() < 2U) {
        ALOGE("EcAttestation cert_chain size failed! actual size %u", cert_chain.size());
        return false;
    }

    if (!verify_chain(cert_chain)) {
        ALOGE("EcAttestation verify_chain failed!");
        return false;
    }

    if (!
        verify_attestation_record("challenge", "foo",                     //
                                  test.key_characteristics_.softwareEnforced,  //
                                  test.key_characteristics_.teeEnforced,       //
                                  cert_chain[0])) {
        ALOGE("EcAttestation verify_attestation_record!");
        return false;
    }
    return true;
}

/*
 * AttestationTest.EcAttestationRequiresAttestationAppId
 *
 * Verifies that attesting to EC keys requires app ID
 */
bool EcAttestationRequiresAttestationAppId(AttestationTest test) {
    if (ErrorCode::OK !=
              test.GenerateKey(AuthorizationSetBuilder()
                              .Authorization(TAG_NO_AUTH_REQUIRED)
                              .EcdsaSigningKey(EcCurve::P_256)
                              .Digest(Digest::SHA_2_256)
                              .Authorization(TAG_INCLUDE_UNIQUE_ID))) {
        ALOGE("EcAttestationRequiresAttestationAppId GenerateKey failed!!!");
        return false;
    }

    hidl_vec<hidl_vec<uint8_t>> cert_chain;
    if (ErrorCode::ATTESTATION_APPLICATION_ID_MISSING !=
              test.AttestKey(AuthorizationSetBuilder().Authorization(
                            TAG_ATTESTATION_CHALLENGE, HidlBuf("challenge")),
                        &cert_chain)) {
        ALOGE("EcAttestationRequiresAttestationAppId AttestKey!");
    }
    return true;
}

/*
 * AttestationTest.AesAttestation
 *
 * Verifies that attesting to AES keys fails in the expected way.
 */
bool AesAttestation(AttestationTest test) {
    if (ErrorCode::OK !=
              test.GenerateKey(AuthorizationSetBuilder()
                              .Authorization(TAG_NO_AUTH_REQUIRED)
                              .AesEncryptionKey(128)
                              .EcbMode()
                              .Padding(PaddingMode::PKCS7))) {
        ALOGE("AesAttestation GenerateKey failed!!!");
        return false;
    }

    hidl_vec<hidl_vec<uint8_t>> cert_chain;
    if (ErrorCode::INCOMPATIBLE_ALGORITHM !=
              test.AttestKey(
            AuthorizationSetBuilder()
                .Authorization(TAG_ATTESTATION_CHALLENGE, HidlBuf("challenge"))
                .Authorization(TAG_ATTESTATION_APPLICATION_ID, HidlBuf("foo")),
            &cert_chain)) {
        ALOGE("AesAttestation AttestKey!");
    }
    return true;
}
/*
 * AttestationTest.HmacAttestation
 *
 * Verifies that attesting to HMAC keys fails in the expected way.
 */
bool HmacAttestation(AttestationTest test) {
    if (ErrorCode::OK !=
              test.GenerateKey(AuthorizationSetBuilder()
                              .Authorization(TAG_NO_AUTH_REQUIRED)
                              .HmacKey(128)
                              .EcbMode()
                              .Digest(Digest::SHA_2_256)
                              .Authorization(TAG_MIN_MAC_LENGTH, 128))) {
        ALOGE("HmacAttestation GenerateKey failed!!!");
        return false;
    }

    hidl_vec<hidl_vec<uint8_t>> cert_chain;
    if (ErrorCode::INCOMPATIBLE_ALGORITHM !=
              test.AttestKey(
            AuthorizationSetBuilder()
                .Authorization(TAG_ATTESTATION_CHALLENGE, HidlBuf("challenge"))
                .Authorization(TAG_ATTESTATION_APPLICATION_ID, HidlBuf("foo")),
            &cert_chain)) {
        ALOGE("HmacAttestation AttestKey!");
    }
    return true;
}

bool check_AttestationKey()
{
    KeymasterHidlTest::SetUpTestCase();
    AttestationTest test;
    bool result = RsaAttestation(test)
        && RsaAttestationRequiresAppId(test)
        && EcAttestation(test)
        && EcAttestationRequiresAttestationAppId(test)
        && AesAttestation(test)
        && HmacAttestation(test);
    ALOGD("check_AttestationKey result %d", result);
    return result;
}

//}  // namespace test
}  // namespace V3_0
}  // namespace keymaster
}  // namespace hardware
}  // namespace android

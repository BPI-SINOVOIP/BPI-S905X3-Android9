/*
 * Copyright 2017 The Android Open Source Project
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

#include <keymasterV4_0/authorization_set.h>

#include <assert.h>

namespace android {
namespace hardware {
namespace keymaster {
namespace V4_0 {

inline bool keyParamLess(const KeyParameter& a, const KeyParameter& b) {
    if (a.tag != b.tag) return a.tag < b.tag;
    int retval;
    switch (typeFromTag(a.tag)) {
        case TagType::INVALID:
        case TagType::BOOL:
            return false;
        case TagType::ENUM:
        case TagType::ENUM_REP:
        case TagType::UINT:
        case TagType::UINT_REP:
            return a.f.integer < b.f.integer;
        case TagType::ULONG:
        case TagType::ULONG_REP:
            return a.f.longInteger < b.f.longInteger;
        case TagType::DATE:
            return a.f.dateTime < b.f.dateTime;
        case TagType::BIGNUM:
        case TagType::BYTES:
            // Handle the empty cases.
            if (a.blob.size() == 0) return b.blob.size() != 0;
            if (b.blob.size() == 0) return false;

            retval = memcmp(&a.blob[0], &b.blob[0], std::min(a.blob.size(), b.blob.size()));
            // if one is the prefix of the other the longer wins
            if (retval == 0) return a.blob.size() < b.blob.size();
            // Otherwise a is less if a is less.
            else
                return retval < 0;
    }
    return false;
}

inline bool keyParamEqual(const KeyParameter& a, const KeyParameter& b) {
    if (a.tag != b.tag) return false;

    switch (typeFromTag(a.tag)) {
        case TagType::INVALID:
        case TagType::BOOL:
            return true;
        case TagType::ENUM:
        case TagType::ENUM_REP:
        case TagType::UINT:
        case TagType::UINT_REP:
            return a.f.integer == b.f.integer;
        case TagType::ULONG:
        case TagType::ULONG_REP:
            return a.f.longInteger == b.f.longInteger;
        case TagType::DATE:
            return a.f.dateTime == b.f.dateTime;
        case TagType::BIGNUM:
        case TagType::BYTES:
            if (a.blob.size() != b.blob.size()) return false;
            return a.blob.size() == 0 || memcmp(&a.blob[0], &b.blob[0], a.blob.size()) == 0;
    }
    return false;
}

void AuthorizationSet::Sort() {
    std::sort(data_.begin(), data_.end(), keyParamLess);
}

void AuthorizationSet::Deduplicate() {
    if (data_.empty()) return;

    Sort();
    std::vector<KeyParameter> result;

    auto curr = data_.begin();
    auto prev = curr++;
    for (; curr != data_.end(); ++prev, ++curr) {
        if (prev->tag == Tag::INVALID) continue;

        if (!keyParamEqual(*prev, *curr)) {
            result.emplace_back(std::move(*prev));
        }
    }
    result.emplace_back(std::move(*prev));

    std::swap(data_, result);
}

void AuthorizationSet::Union(const AuthorizationSet& other) {
    data_.insert(data_.end(), other.data_.begin(), other.data_.end());
    Deduplicate();
}

void AuthorizationSet::Subtract(const AuthorizationSet& other) {
    Deduplicate();

    auto i = other.begin();
    while (i != other.end()) {
        int pos = -1;
        do {
            pos = find(i->tag, pos);
            if (pos != -1 && keyParamEqual(*i, data_[pos])) {
                data_.erase(data_.begin() + pos);
                break;
            }
        } while (pos != -1);
        ++i;
    }
}

KeyParameter& AuthorizationSet::operator[](int at) {
    return data_[at];
}

const KeyParameter& AuthorizationSet::operator[](int at) const {
    return data_[at];
}

void AuthorizationSet::Clear() {
    data_.clear();
}

size_t AuthorizationSet::GetTagCount(Tag tag) const {
    size_t count = 0;
    for (int pos = -1; (pos = find(tag, pos)) != -1;) ++count;
    return count;
}

int AuthorizationSet::find(Tag tag, int begin) const {
    auto iter = data_.begin() + (1 + begin);

    while (iter != data_.end() && iter->tag != tag) ++iter;

    if (iter != data_.end()) return iter - data_.begin();
    return -1;
}

bool AuthorizationSet::erase(int index) {
    auto pos = data_.begin() + index;
    if (pos != data_.end()) {
        data_.erase(pos);
        return true;
    }
    return false;
}

NullOr<const KeyParameter&> AuthorizationSet::GetEntry(Tag tag) const {
    int pos = find(tag);
    if (pos == -1) return {};
    return data_[pos];
}

/**
 * Persistent format is:
 * | 32 bit indirect_size         |
 * --------------------------------
 * | indirect_size bytes of data  | this is where the blob data is stored
 * --------------------------------
 * | 32 bit element_count         | number of entries
 * | 32 bit elements_size         | total bytes used by entries (entries have variable length)
 * --------------------------------
 * | elementes_size bytes of data | where the elements are stored
 */

/**
 * Persistent format of blobs and bignums:
 * | 32 bit tag             |
 * | 32 bit blob_length     |
 * | 32 bit indirect_offset |
 */

struct OutStreams {
    std::ostream& indirect;
    std::ostream& elements;
};

OutStreams& serializeParamValue(OutStreams& out, const hidl_vec<uint8_t>& blob) {
    uint32_t buffer;

    // write blob_length
    auto blob_length = blob.size();
    if (blob_length > std::numeric_limits<uint32_t>::max()) {
        out.elements.setstate(std::ios_base::badbit);
        return out;
    }
    buffer = blob_length;
    out.elements.write(reinterpret_cast<const char*>(&buffer), sizeof(uint32_t));

    // write indirect_offset
    auto offset = out.indirect.tellp();
    if (offset < 0 || offset > std::numeric_limits<uint32_t>::max() ||
        uint32_t(offset) + uint32_t(blob_length) < uint32_t(offset)) {  // overflow check
        out.elements.setstate(std::ios_base::badbit);
        return out;
    }
    buffer = offset;
    out.elements.write(reinterpret_cast<const char*>(&buffer), sizeof(uint32_t));

    // write blob to indirect stream
    if (blob_length) out.indirect.write(reinterpret_cast<const char*>(&blob[0]), blob_length);

    return out;
}

template <typename T>
OutStreams& serializeParamValue(OutStreams& out, const T& value) {
    out.elements.write(reinterpret_cast<const char*>(&value), sizeof(T));
    return out;
}

OutStreams& serialize(TAG_INVALID_t&&, OutStreams& out, const KeyParameter&) {
    // skip invalid entries.
    return out;
}
template <typename T>
OutStreams& serialize(T ttag, OutStreams& out, const KeyParameter& param) {
    out.elements.write(reinterpret_cast<const char*>(&param.tag), sizeof(int32_t));
    return serializeParamValue(out, accessTagValue(ttag, param));
}

template <typename... T>
struct choose_serializer;
template <typename... Tags>
struct choose_serializer<MetaList<Tags...>> {
    static OutStreams& serialize(OutStreams& out, const KeyParameter& param) {
        return choose_serializer<Tags...>::serialize(out, param);
    }
};

template <>
struct choose_serializer<> {
    static OutStreams& serialize(OutStreams& out, const KeyParameter&) { return out; }
};

template <TagType tag_type, Tag tag, typename... Tail>
struct choose_serializer<TypedTag<tag_type, tag>, Tail...> {
    static OutStreams& serialize(OutStreams& out, const KeyParameter& param) {
        if (param.tag == tag) {
            return V4_0::serialize(TypedTag<tag_type, tag>(), out, param);
        } else {
            return choose_serializer<Tail...>::serialize(out, param);
        }
    }
};

OutStreams& serialize(OutStreams& out, const KeyParameter& param) {
    return choose_serializer<all_tags_t>::serialize(out, param);
}

std::ostream& serialize(std::ostream& out, const std::vector<KeyParameter>& params) {
    std::stringstream indirect;
    std::stringstream elements;
    OutStreams streams = {indirect, elements};
    for (const auto& param : params) {
        serialize(streams, param);
    }
    if (indirect.bad() || elements.bad()) {
        out.setstate(std::ios_base::badbit);
        return out;
    }
    auto pos = indirect.tellp();
    if (pos < 0 || pos > std::numeric_limits<uint32_t>::max()) {
        out.setstate(std::ios_base::badbit);
        return out;
    }
    uint32_t indirect_size = pos;
    pos = elements.tellp();
    if (pos < 0 || pos > std::numeric_limits<uint32_t>::max()) {
        out.setstate(std::ios_base::badbit);
        return out;
    }
    uint32_t elements_size = pos;
    uint32_t element_count = params.size();

    out.write(reinterpret_cast<const char*>(&indirect_size), sizeof(uint32_t));

    pos = out.tellp();
    if (indirect_size) out << indirect.rdbuf();
    assert(out.tellp() - pos == indirect_size);

    out.write(reinterpret_cast<const char*>(&element_count), sizeof(uint32_t));
    out.write(reinterpret_cast<const char*>(&elements_size), sizeof(uint32_t));

    pos = out.tellp();
    if (elements_size) out << elements.rdbuf();
    assert(out.tellp() - pos == elements_size);

    return out;
}

struct InStreams {
    std::istream& indirect;
    std::istream& elements;
};

InStreams& deserializeParamValue(InStreams& in, hidl_vec<uint8_t>* blob) {
    uint32_t blob_length = 0;
    uint32_t offset = 0;
    in.elements.read(reinterpret_cast<char*>(&blob_length), sizeof(uint32_t));
    blob->resize(blob_length);
    in.elements.read(reinterpret_cast<char*>(&offset), sizeof(uint32_t));
    in.indirect.seekg(offset);
    in.indirect.read(reinterpret_cast<char*>(&(*blob)[0]), blob->size());
    return in;
}

template <typename T>
InStreams& deserializeParamValue(InStreams& in, T* value) {
    in.elements.read(reinterpret_cast<char*>(value), sizeof(T));
    return in;
}

InStreams& deserialize(TAG_INVALID_t&&, InStreams& in, KeyParameter*) {
    // there should be no invalid KeyParamaters but if handle them as zero sized.
    return in;
}

template <typename T>
InStreams& deserialize(T&& ttag, InStreams& in, KeyParameter* param) {
    return deserializeParamValue(in, &accessTagValue(ttag, *param));
}

template <typename... T>
struct choose_deserializer;
template <typename... Tags>
struct choose_deserializer<MetaList<Tags...>> {
    static InStreams& deserialize(InStreams& in, KeyParameter* param) {
        return choose_deserializer<Tags...>::deserialize(in, param);
    }
};
template <>
struct choose_deserializer<> {
    static InStreams& deserialize(InStreams& in, KeyParameter*) {
        // encountered an unknown tag -> fail parsing
        in.elements.setstate(std::ios_base::badbit);
        return in;
    }
};
template <TagType tag_type, Tag tag, typename... Tail>
struct choose_deserializer<TypedTag<tag_type, tag>, Tail...> {
    static InStreams& deserialize(InStreams& in, KeyParameter* param) {
        if (param->tag == tag) {
            return V4_0::deserialize(TypedTag<tag_type, tag>(), in, param);
        } else {
            return choose_deserializer<Tail...>::deserialize(in, param);
        }
    }
};

InStreams& deserialize(InStreams& in, KeyParameter* param) {
    in.elements.read(reinterpret_cast<char*>(&param->tag), sizeof(Tag));
    return choose_deserializer<all_tags_t>::deserialize(in, param);
}

std::istream& deserialize(std::istream& in, std::vector<KeyParameter>* params) {
    uint32_t indirect_size = 0;
    in.read(reinterpret_cast<char*>(&indirect_size), sizeof(uint32_t));
    std::string indirect_buffer(indirect_size, '\0');
    if (indirect_buffer.size() != indirect_size) {
        in.setstate(std::ios_base::badbit);
        return in;
    }
    in.read(&indirect_buffer[0], indirect_buffer.size());

    uint32_t element_count = 0;
    in.read(reinterpret_cast<char*>(&element_count), sizeof(uint32_t));
    uint32_t elements_size = 0;
    in.read(reinterpret_cast<char*>(&elements_size), sizeof(uint32_t));

    std::string elements_buffer(elements_size, '\0');
    if (elements_buffer.size() != elements_size) {
        in.setstate(std::ios_base::badbit);
        return in;
    }
    in.read(&elements_buffer[0], elements_buffer.size());

    if (in.bad()) return in;

    // TODO write one-shot stream buffer to avoid copying here
    std::stringstream indirect(indirect_buffer);
    std::stringstream elements(elements_buffer);
    InStreams streams = {indirect, elements};

    params->resize(element_count);

    for (uint32_t i = 0; i < element_count; ++i) {
        deserialize(streams, &(*params)[i]);
    }
    return in;
}

void AuthorizationSet::Serialize(std::ostream* out) const {
    serialize(*out, data_);
}

void AuthorizationSet::Deserialize(std::istream* in) {
    deserialize(*in, &data_);
}

AuthorizationSetBuilder& AuthorizationSetBuilder::RsaKey(uint32_t key_size,
                                                         uint64_t public_exponent) {
    Authorization(TAG_ALGORITHM, Algorithm::RSA);
    Authorization(TAG_KEY_SIZE, key_size);
    Authorization(TAG_RSA_PUBLIC_EXPONENT, public_exponent);
    return *this;
}

AuthorizationSetBuilder& AuthorizationSetBuilder::EcdsaKey(uint32_t key_size) {
    Authorization(TAG_ALGORITHM, Algorithm::EC);
    Authorization(TAG_KEY_SIZE, key_size);
    return *this;
}

AuthorizationSetBuilder& AuthorizationSetBuilder::EcdsaKey(EcCurve curve) {
    Authorization(TAG_ALGORITHM, Algorithm::EC);
    Authorization(TAG_EC_CURVE, curve);
    return *this;
}

AuthorizationSetBuilder& AuthorizationSetBuilder::AesKey(uint32_t key_size) {
    Authorization(TAG_ALGORITHM, Algorithm::AES);
    return Authorization(TAG_KEY_SIZE, key_size);
}

AuthorizationSetBuilder& AuthorizationSetBuilder::TripleDesKey(uint32_t key_size) {
    Authorization(TAG_ALGORITHM, Algorithm::TRIPLE_DES);
    return Authorization(TAG_KEY_SIZE, key_size);
}

AuthorizationSetBuilder& AuthorizationSetBuilder::HmacKey(uint32_t key_size) {
    Authorization(TAG_ALGORITHM, Algorithm::HMAC);
    Authorization(TAG_KEY_SIZE, key_size);
    return SigningKey();
}

AuthorizationSetBuilder& AuthorizationSetBuilder::RsaSigningKey(uint32_t key_size,
                                                                uint64_t public_exponent) {
    RsaKey(key_size, public_exponent);
    return SigningKey();
}

AuthorizationSetBuilder& AuthorizationSetBuilder::RsaEncryptionKey(uint32_t key_size,
                                                                   uint64_t public_exponent) {
    RsaKey(key_size, public_exponent);
    return EncryptionKey();
}

AuthorizationSetBuilder& AuthorizationSetBuilder::EcdsaSigningKey(uint32_t key_size) {
    EcdsaKey(key_size);
    return SigningKey();
}

AuthorizationSetBuilder& AuthorizationSetBuilder::EcdsaSigningKey(EcCurve curve) {
    EcdsaKey(curve);
    return SigningKey();
}

AuthorizationSetBuilder& AuthorizationSetBuilder::AesEncryptionKey(uint32_t key_size) {
    AesKey(key_size);
    return EncryptionKey();
}

AuthorizationSetBuilder& AuthorizationSetBuilder::TripleDesEncryptionKey(uint32_t key_size) {
    TripleDesKey(key_size);
    return EncryptionKey();
}

AuthorizationSetBuilder& AuthorizationSetBuilder::SigningKey() {
    Authorization(TAG_PURPOSE, KeyPurpose::SIGN);
    return Authorization(TAG_PURPOSE, KeyPurpose::VERIFY);
}

AuthorizationSetBuilder& AuthorizationSetBuilder::EncryptionKey() {
    Authorization(TAG_PURPOSE, KeyPurpose::ENCRYPT);
    return Authorization(TAG_PURPOSE, KeyPurpose::DECRYPT);
}

AuthorizationSetBuilder& AuthorizationSetBuilder::NoDigestOrPadding() {
    Authorization(TAG_DIGEST, Digest::NONE);
    return Authorization(TAG_PADDING, PaddingMode::NONE);
}

AuthorizationSetBuilder& AuthorizationSetBuilder::EcbMode() {
    return Authorization(TAG_BLOCK_MODE, BlockMode::ECB);
}

AuthorizationSetBuilder& AuthorizationSetBuilder::GcmModeMinMacLen(uint32_t minMacLength) {
    return BlockMode(BlockMode::GCM)
        .Padding(PaddingMode::NONE)
        .Authorization(TAG_MIN_MAC_LENGTH, minMacLength);
}

AuthorizationSetBuilder& AuthorizationSetBuilder::GcmModeMacLen(uint32_t macLength) {
    return BlockMode(BlockMode::GCM)
        .Padding(PaddingMode::NONE)
        .Authorization(TAG_MAC_LENGTH, macLength);
}

AuthorizationSetBuilder& AuthorizationSetBuilder::BlockMode(
    std::initializer_list<V4_0::BlockMode> blockModes) {
    for (auto mode : blockModes) {
        push_back(TAG_BLOCK_MODE, mode);
    }
    return *this;
}

AuthorizationSetBuilder& AuthorizationSetBuilder::Digest(
    std::initializer_list<V4_0::Digest> digests) {
    for (auto digest : digests) {
        push_back(TAG_DIGEST, digest);
    }
    return *this;
}

AuthorizationSetBuilder& AuthorizationSetBuilder::Padding(
    std::initializer_list<V4_0::PaddingMode> paddingModes) {
    for (auto paddingMode : paddingModes) {
        push_back(TAG_PADDING, paddingMode);
    }
    return *this;
}

}  // namespace V4_0
}  // namespace keymaster
}  // namespace hardware
}  // namespace android

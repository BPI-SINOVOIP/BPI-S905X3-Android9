/*
 * Copyright (C) 2015, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "type_cpp.h"

#include <algorithm>
#include <iostream>
#include <vector>

#include <android-base/stringprintf.h>
#include <android-base/strings.h>

#include "logging.h"

using std::cerr;
using std::endl;
using std::set;
using std::string;
using std::unique_ptr;
using std::vector;

using android::base::Split;
using android::base::Join;
using android::base::StringPrintf;

namespace android {
namespace aidl {
namespace cpp {
namespace {

const char kNoPackage[] = "";
const char kNoHeader[] = "";
const char kNoValidMethod[] = "";
Type* const kNoArrayType = nullptr;
Type* const kNoNullableType = nullptr;

bool is_cpp_keyword(const std::string& str) {
  static const std::vector<std::string> kCppKeywords{
    "alignas", "alignof", "and", "and_eq", "asm", "auto", "bitand", "bitor",
    "bool", "break", "case", "catch", "char", "char16_t", "char32_t", "class",
    "compl", "concept", "const", "constexpr", "const_cast", "continue",
    "decltype", "default", "delete", "do", "double", "dynamic_cast", "else",
    "enum", "explicit", "export", "extern", "false", "float", "for", "friend",
    "goto", "if", "inline", "int", "long", "mutable", "namespace", "new",
    "noexcept", "not", "not_eq", "nullptr", "operator", "or", "or_eq",
    "private", "protected", "public", "register", "reinterpret_cast",
    "requires", "return", "short", "signed", "sizeof", "static",
    "static_assert", "static_cast", "struct", "switch", "template", "this",
    "thread_local", "throw", "true", "try", "typedef", "typeid", "typename",
    "union", "unsigned", "using", "virtual", "void", "volatile", "wchar_t",
    "while", "xor", "xor_eq",
  };
  return std::find(kCppKeywords.begin(), kCppKeywords.end(), str) !=
      kCppKeywords.end();
}

class VoidType : public Type {
 public:
  VoidType() : Type(ValidatableType::KIND_BUILT_IN, kNoPackage, "void",
                    {}, "void", kNoValidMethod, kNoValidMethod) {}
  virtual ~VoidType() = default;
  bool CanBeOutParameter() const override { return false; }
  bool CanWriteToParcel() const override { return false; }
};  // class VoidType

class CppArrayType : public Type {
 public:
  CppArrayType(int kind,  // from ValidatableType
            const std::string& package,
            const string& underlying_aidl_type,
            const string& cpp_header,
            const string& underlying_cpp_type,
            const string& underlying_cpp_type_nulllable,
            const string& read_method,
            const string& write_method,
            bool is_nullable,
            const string& src_file_name = "")
      : Type(kind, package,
             underlying_aidl_type + "[]",
             GetHeaders(is_nullable, cpp_header),
             GetCppType(is_nullable, underlying_cpp_type),
             read_method, write_method, kNoArrayType,
             (is_nullable)
                 ? kNoNullableType
                 // All arrays are nullable.
                 : new CppArrayType(kind, package, underlying_aidl_type,
                                    cpp_header, underlying_cpp_type_nulllable,
                                    underlying_cpp_type_nulllable,
                                    read_method, write_method, true),
             src_file_name) {}

  bool CanBeOutParameter() const override { return true; }

 private:
  static vector<string> GetHeaders(bool is_nullable, const string& cpp_header) {
    vector<string> result = {"vector"};
    if (is_nullable) {
      result.push_back("memory");
    }
    if (!cpp_header.empty()) {
      result.push_back(cpp_header);
    }
    return result;
  }

  static string GetCppType(bool is_nullable,
                           const string& underlying_cpp_type) {
    if (is_nullable)
      return StringPrintf("::std::unique_ptr<::std::vector<%s>>",
                          underlying_cpp_type.c_str());
    return StringPrintf("::std::vector<%s>",
                        underlying_cpp_type.c_str());
  }

  DISALLOW_COPY_AND_ASSIGN(CppArrayType);
};  // class CppArrayType

class PrimitiveType : public Type {
 public:
  PrimitiveType(const std::string& aidl_type,
                const std::string& header,
                const std::string& cpp_type,
                const std::string& read_method,
                const std::string& write_method,
                const std::string& read_array_method,
                const std::string& write_array_method)
      : Type(ValidatableType::KIND_BUILT_IN, kNoPackage, aidl_type, {header},
             cpp_type, read_method, write_method,
             new CppArrayType(ValidatableType::KIND_BUILT_IN, kNoPackage,
                              aidl_type, header, cpp_type, cpp_type,
                              read_array_method, write_array_method,
                              false)) {}

  virtual ~PrimitiveType() = default;
  bool IsCppPrimitive() const override { return true; }

 private:
  DISALLOW_COPY_AND_ASSIGN(PrimitiveType);
};  // class PrimitiveType

// Unfortunately, bytes in Java are signed.  However, most C authors would
// say that a byte is not in fact signed.  Compromise: customize this otherwise
// normal primitive to use signed single bytes, but unsigned byte arrays.
class ByteType : public Type {
 public:
  ByteType()
      : Type(ValidatableType::KIND_BUILT_IN, kNoPackage, "byte",
             {"cstdint"}, "int8_t", "readByte", "writeByte",
             new CppArrayType(ValidatableType::KIND_BUILT_IN, kNoPackage,
                              "byte", "cstdint", "uint8_t", "uint8_t",
                              "readByteVector", "writeByteVector",
                              false)) {}

  virtual ~ByteType() = default;
  bool IsCppPrimitive() const override { return true; }

 private:
  DISALLOW_COPY_AND_ASSIGN(ByteType);
};  // class PrimitiveType

class BinderType : public Type {
 public:
  BinderType(const AidlInterface& interface, const std::string& src_file_name)
      : BinderType(interface, src_file_name,
                   new BinderType(interface, src_file_name, kNoNullableType,
                                  "readNullableStrongBinder"),
                   "readStrongBinder") {}
  virtual ~BinderType() = default;

  string WriteCast(const string& val) const override {
    return write_cast_ + "(" + val + ")";
  }

 private:
  BinderType(const AidlInterface& interface,
             const std::string& src_file_name,
             Type* nullable_type, const std::string& read)
      : Type(ValidatableType::KIND_GENERATED,
             interface.GetPackage(), interface.GetName(),
             {GetCppHeader(interface)}, GetCppName(interface),
             read, "writeStrongBinder", kNoArrayType, nullable_type,
             src_file_name, interface.GetLine()),
        write_cast_(GetRawCppName(interface) + "::asBinder") {}

  static string GetCppName(const AidlInterface& interface) {
    return "::android::sp<" + GetRawCppName(interface) + ">";
  }

  static string GetRawCppName(const AidlInterface& interface) {
    vector<string> name = interface.GetSplitPackage();
    string ret;

    name.push_back(interface.GetName());

    for (const auto& term : name) {
      ret += "::" + term;
    }

    return ret;
  }

  static string GetCppHeader(const AidlInterface& interface) {
    vector<string> name = interface.GetSplitPackage();
    name.push_back(interface.GetName());
    return Join(name, '/') + ".h";
  }

  std::string write_cast_;
};

class NullableParcelableType : public Type {
 public:
  NullableParcelableType(const AidlParcelable& parcelable,
                         const std::string& src_file_name)
      : Type(ValidatableType::KIND_PARCELABLE,
             parcelable.GetPackage(), parcelable.GetName(),
             {parcelable.GetCppHeader()}, GetCppName(parcelable),
             "readParcelable", "writeNullableParcelable",
             kNoArrayType, kNoNullableType,
             src_file_name, parcelable.GetLine()) {}
  virtual ~NullableParcelableType() = default;
  bool CanBeOutParameter() const override { return true; }

 private:
  static string GetCppName(const AidlParcelable& parcelable) {
    return "::std::unique_ptr<::" + Join(parcelable.GetSplitPackage(), "::") +
        "::" + parcelable.GetCppName() + ">";
  }
};

class ParcelableType : public Type {
 public:
  ParcelableType(const AidlParcelable& parcelable,
                 const std::string& src_file_name)
      : Type(ValidatableType::KIND_PARCELABLE,
             parcelable.GetPackage(), parcelable.GetName(),
             {parcelable.GetCppHeader()}, GetCppName(parcelable),
             "readParcelable", "writeParcelable",
             new CppArrayType(
                 ValidatableType::KIND_PARCELABLE, parcelable.GetPackage(),
                 parcelable.GetName(), parcelable.GetCppHeader(),
                 GetCppName(parcelable), GetCppName(parcelable),
                 "readParcelableVector", "writeParcelableVector", false,
                 src_file_name),
             new NullableParcelableType(parcelable, src_file_name),
             src_file_name, parcelable.GetLine()) {}
  virtual ~ParcelableType() = default;
  bool CanBeOutParameter() const override { return true; }

 private:
  static string GetCppName(const AidlParcelable& parcelable) {
    return "::" + Join(parcelable.GetSplitPackage(), "::") +
        "::" + parcelable.GetCppName();
  }
};

class NullableMap : public Type {
 public:
  NullableMap()
      : Type(ValidatableType::KIND_BUILT_IN,
             "java.util", "Map",
             {"binder/Map.h", "binder/Value.h"},
             "::std::unique_ptr<::android::binder::Map>",
             "readNullableMap", "writeNullableMap") {}
  virtual ~NullableMap() = default;
  bool CanBeOutParameter() const override { return true; }
};


class MapType : public Type {
 public:
  MapType()
      : Type(ValidatableType::KIND_BUILT_IN,
             "java.util", "Map",
             {"binder/Map.h","binder/Value.h"},
             "::android::binder::Map",
             "readMap", "writeMap",
             kNoArrayType,
             new NullableMap() ) {}
  virtual ~MapType() = default;
  bool CanBeOutParameter() const override { return true; }

 private:
  DISALLOW_COPY_AND_ASSIGN(MapType);
};  // class MapType

class NullableStringListType : public Type {
 public:
  NullableStringListType()
      : Type(ValidatableType::KIND_BUILT_IN,
             "java.util", "List<" + string(kStringCanonicalName) + ">",
             {"utils/String16.h", "memory", "vector"},
             "::std::unique_ptr<::std::vector<std::unique_ptr<::android::String16>>>",
             "readString16Vector", "writeString16Vector") {}
  virtual ~NullableStringListType() = default;
  bool CanBeOutParameter() const override { return true; }

 private:
  DISALLOW_COPY_AND_ASSIGN(NullableStringListType);
};  // class NullableStringListType

class StringListType : public Type {
 public:
  StringListType()
      : Type(ValidatableType::KIND_BUILT_IN,
             "java.util", "List<" + string(kStringCanonicalName) + ">",
             {"utils/String16.h", "vector"},
             "::std::vector<::android::String16>",
             "readString16Vector", "writeString16Vector",
             kNoArrayType, new NullableStringListType()) {}
  virtual ~StringListType() = default;
  bool CanBeOutParameter() const override { return true; }

 private:
  DISALLOW_COPY_AND_ASSIGN(StringListType);
};  // class StringListType

class NullableUtf8InCppStringListType : public Type {
 public:
  NullableUtf8InCppStringListType()
      : Type(ValidatableType::KIND_BUILT_IN,
             "java.util", "List<" + string(kUtf8InCppStringCanonicalName) + ">",
             {"memory", "string", "vector"},
             "::std::unique_ptr<::std::vector<std::unique_ptr<::std::string>>>",
             "readUtf8VectorFromUtf16Vector", "writeUtf8VectorAsUtf16Vector") {}
  virtual ~NullableUtf8InCppStringListType() = default;
  bool CanBeOutParameter() const override { return true; }

 private:
  DISALLOW_COPY_AND_ASSIGN(NullableUtf8InCppStringListType);
};  // class NullableUtf8InCppStringListType

class Utf8InCppStringListType : public Type {
 public:
  Utf8InCppStringListType()
      : Type(ValidatableType::KIND_BUILT_IN,
             "java.util", "List<" + string(kUtf8InCppStringCanonicalName) + ">",
             {"string", "vector"},
             "::std::vector<::std::string>",
             "readUtf8VectorFromUtf16Vector", "writeUtf8VectorAsUtf16Vector",
             kNoArrayType, new NullableUtf8InCppStringListType()) {}
  virtual ~Utf8InCppStringListType() = default;
  bool CanBeOutParameter() const override { return true; }

 private:
  DISALLOW_COPY_AND_ASSIGN(Utf8InCppStringListType);
};  // class Utf8InCppStringListType

class NullableBinderListType : public Type {
 public:
  NullableBinderListType()
      : Type(ValidatableType::KIND_BUILT_IN, "java.util",
             "List<android.os.IBinder>", {"binder/IBinder.h", "vector"},
             "::std::unique_ptr<::std::vector<::android::sp<::android::IBinder>>>",
             "readStrongBinderVector", "writeStrongBinderVector") {}
  virtual ~NullableBinderListType() = default;
  bool CanBeOutParameter() const override { return true; }

 private:
  DISALLOW_COPY_AND_ASSIGN(NullableBinderListType);
};  // class NullableBinderListType

class BinderListType : public Type {
 public:
  BinderListType()
      : Type(ValidatableType::KIND_BUILT_IN, "java.util",
             "List<android.os.IBinder>", {"binder/IBinder.h", "vector"},
             "::std::vector<::android::sp<::android::IBinder>>",
             "readStrongBinderVector", "writeStrongBinderVector",
             kNoArrayType, new NullableBinderListType()) {}
  virtual ~BinderListType() = default;
  bool CanBeOutParameter() const override { return true; }

 private:
  DISALLOW_COPY_AND_ASSIGN(BinderListType);
};  // class BinderListType

}  // namespace

Type::Type(int kind,
           const std::string& package,
           const std::string& aidl_type,
           const vector<string>& headers,
           const string& cpp_type,
           const string& read_method,
           const string& write_method,
           Type* array_type,
           Type* nullable_type,
           const string& src_file_name,
           int line)
    : ValidatableType(kind, package, aidl_type, src_file_name, line),
      headers_(headers),
      aidl_type_(aidl_type),
      cpp_type_(cpp_type),
      parcel_read_method_(read_method),
      parcel_write_method_(write_method),
      array_type_(array_type),
      nullable_type_(nullable_type) {}

bool Type::CanWriteToParcel() const { return true; }

void TypeNamespace::Init() {
  Add(new ByteType());
  Add(new PrimitiveType(
      "int",
      "cstdint", "int32_t", "readInt32", "writeInt32",
      "readInt32Vector", "writeInt32Vector"));
  Add(new PrimitiveType(
      "long",
      "cstdint", "int64_t", "readInt64", "writeInt64",
      "readInt64Vector", "writeInt64Vector"));
  Add(new PrimitiveType(
      "float",
      kNoHeader, "float", "readFloat", "writeFloat",
      "readFloatVector", "writeFloatVector"));
  Add(new PrimitiveType(
      "double",
      kNoHeader, "double", "readDouble", "writeDouble",
      "readDoubleVector", "writeDoubleVector"));
  Add(new PrimitiveType(
      "boolean",
      kNoHeader, "bool", "readBool", "writeBool",
      "readBoolVector", "writeBoolVector"));
  // C++11 defines the char16_t type as a built in for Unicode characters.
  Add(new PrimitiveType(
      "char",
      kNoHeader, "char16_t", "readChar", "writeChar",
      "readCharVector", "writeCharVector"));

  Type* string_array_type = new CppArrayType(
      ValidatableType::KIND_BUILT_IN, "java.lang", "String",
      "utils/String16.h", "::android::String16",
      "::std::unique_ptr<::android::String16>", "readString16Vector",
      "writeString16Vector", false);

  Type* nullable_string_type =
      new Type(ValidatableType::KIND_BUILT_IN, "java.lang", "String",
               {"memory", "utils/String16.h"}, "::std::unique_ptr<::android::String16>",
               "readString16", "writeString16");

  string_type_ = new Type(ValidatableType::KIND_BUILT_IN, "java.lang", "String",
                          {"utils/String16.h"}, "::android::String16",
                          "readString16", "writeString16",
                          string_array_type, nullable_string_type);
  Add(string_type_);

  using ::android::aidl::kAidlReservedTypePackage;
  using ::android::aidl::kUtf8InCppStringClass;

  // This type is a Utf16 string in the parcel, but deserializes to
  // a std::string in Utf8 format when we use it in C++.
  Type* cpp_utf8_string_array = new CppArrayType(
      ValidatableType::KIND_BUILT_IN,
      kAidlReservedTypePackage, kUtf8InCppStringClass,
      "string", "::std::string", "::std::unique_ptr<::std::string>",
      "readUtf8VectorFromUtf16Vector", "writeUtf8VectorAsUtf16Vector",
      false);
  Type* nullable_cpp_utf8_string_type = new Type(
      ValidatableType::KIND_BUILT_IN,
      kAidlReservedTypePackage, kUtf8InCppStringClass,
      {"string", "memory"}, "::std::unique_ptr<::std::string>",
      "readUtf8FromUtf16", "writeUtf8AsUtf16");
  Add(new Type(
      ValidatableType::KIND_BUILT_IN,
      kAidlReservedTypePackage, kUtf8InCppStringClass,
      {"string"}, "::std::string", "readUtf8FromUtf16", "writeUtf8AsUtf16",
      cpp_utf8_string_array, nullable_cpp_utf8_string_type));

  Type* nullable_ibinder = new Type(
      ValidatableType::KIND_BUILT_IN, "android.os", "IBinder",
      {"binder/IBinder.h"}, "::android::sp<::android::IBinder>",
      "readNullableStrongBinder", "writeStrongBinder");
  ibinder_type_ = new Type(
      ValidatableType::KIND_BUILT_IN, "android.os", "IBinder",
      {"binder/IBinder.h"}, "::android::sp<::android::IBinder>",
      "readStrongBinder", "writeStrongBinder",
      kNoArrayType, nullable_ibinder);
  Add(ibinder_type_);

  Add(new MapType());

  Add(new BinderListType());
  Add(new StringListType());
  Add(new Utf8InCppStringListType());

  Type* fd_vector_type = new CppArrayType(
      ValidatableType::KIND_BUILT_IN, kNoPackage, "FileDescriptor",
      "android-base/unique_fd.h",
      "::android::base::unique_fd", "::android::base::unique_fd",
      "readUniqueFileDescriptorVector", "writeUniqueFileDescriptorVector",
      false);

  Add(new Type(
      ValidatableType::KIND_BUILT_IN, kNoPackage, "FileDescriptor",
      {"android-base/unique_fd.h"}, "::android::base::unique_fd",
      "readUniqueFileDescriptor", "writeUniqueFileDescriptor",
      fd_vector_type));

  void_type_ = new class VoidType();
  Add(void_type_);
}

bool TypeNamespace::AddParcelableType(const AidlParcelable& p,
                                      const string& filename) {
  if (p.GetCppHeader().empty()) {
    LOG(ERROR) << "Parcelable " << p.GetCanonicalName()
               << " has no C++ header defined.";
    return false;
  }
  Add(new ParcelableType(p, filename));
  return true;
}

bool TypeNamespace::AddBinderType(const AidlInterface& b,
                                  const string& file_name) {
  Add(new BinderType(b, file_name));
  return true;
}

bool TypeNamespace::AddListType(const std::string& type_name) {
  const Type* contained_type = FindTypeByCanonicalName(type_name);
  if (!contained_type) {
    LOG(ERROR) << "Cannot create List<" << type_name << "> because contained "
                  "type cannot be found or is invalid.";
    return false;
  }
  if (contained_type->IsCppPrimitive()) {
    LOG(ERROR) << "Cannot create List<" << type_name << "> because contained "
                  "type is a primitive in Java and Java List cannot hold "
                  "primitives.";
    return false;
  }

  if (contained_type->CanonicalName() == kStringCanonicalName ||
      contained_type->CanonicalName() == kUtf8InCppStringCanonicalName ||
      contained_type == IBinderType()) {
    return true;
  }

  // TODO Support lists of parcelables b/23600712

  LOG(ERROR) << "aidl-cpp does not yet support List<" << type_name << ">";
  return false;
}

bool TypeNamespace::AddMapType(const std::string& /* key_type_name */,
                               const std::string& /* value_type_name */) {
  // TODO Support list types b/25242025
  LOG(ERROR) << "aidl does not implement support for typed maps!";
  return false;
}

bool TypeNamespace::IsValidPackage(const string& package) const {
  if (package.empty()) {
    return false;
  }

  auto pieces = Split(package, ".");
  for (const string& piece : pieces) {
    if (is_cpp_keyword(piece)) {
      return false;
    }
  }

  return true;
}

const ValidatableType* TypeNamespace::GetArgType(const AidlArgument& a,
    int arg_index,
    const std::string& filename,
    const AidlInterface& interface) const {
  const string error_prefix = StringPrintf(
      "In file %s line %d parameter %s (%d):\n    ",
      filename.c_str(), a.GetLine(), a.GetName().c_str(), arg_index);

  // check that the name doesn't match a keyword
  if (is_cpp_keyword(a.GetName().c_str())) {
    cerr << error_prefix << "Argument name is a C++ keyword"
         << endl;
    return nullptr;
  }

  return ::android::aidl::TypeNamespace::GetArgType(a, arg_index, filename,
                                                    interface);
}

}  // namespace cpp
}  // namespace aidl
}  // namespace android

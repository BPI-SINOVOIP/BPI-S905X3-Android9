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

#include "HandleType.h"

#include "HidlTypeAssertion.h"

#include <hidl-util/Formatter.h>
#include <android-base/logging.h>

namespace android {

HandleType::HandleType(Scope* parent) : Type(parent) {}

bool HandleType::isHandle() const {
    return true;
}

std::string HandleType::typeName() const {
    return "handle";
}

std::string HandleType::getCppType(StorageMode mode,
                                   bool specifyNamespaces) const {
    const std::string base =
          std::string(specifyNamespaces ? "::android::hardware::" : "")
        + "hidl_handle";

    switch (mode) {
        case StorageMode_Stack:
            return base;

        case StorageMode_Argument:
            return "const " + base + "&";

        case StorageMode_Result:
            return base;
    }
}

std::string HandleType::getVtsType() const {
    return "TYPE_HANDLE";
}

void HandleType::emitReaderWriter(
        Formatter &out,
        const std::string &name,
        const std::string &parcelObj,
        bool parcelObjIsPointer,
        bool isReader,
        ErrorMode mode) const {
    const std::string parcelObjDeref =
        parcelObj + (parcelObjIsPointer ? "->" : ".");

    if (isReader) {
        out << "const native_handle_t *"
            << name << "_ptr;\n\n";

        out << "_hidl_err = "
            << parcelObjDeref
            << "readNullableNativeHandleNoDup("
            << "&" << name << "_ptr"
            << ");\n\n";

        handleError(out, mode);

        out << name << " = " << name << "_ptr;\n";
    } else {
        out << "_hidl_err = ";
        out << parcelObjDeref
            << "writeNativeHandleNoDup("
            << name
            << ");\n";

        handleError(out, mode);
    }
}

bool HandleType::useNameInEmitReaderWriterEmbedded(bool isReader) const {
    return !isReader;
}

void HandleType::emitReaderWriterEmbedded(
        Formatter &out,
        size_t /* depth */,
        const std::string &name,
        const std::string &sanitizedName,
        bool nameIsPointer,
        const std::string &parcelObj,
        bool parcelObjIsPointer,
        bool isReader,
        ErrorMode mode,
        const std::string &parentName,
        const std::string &offsetText) const {
    if (isReader) {
        const std::string ptrName = "_hidl_" + sanitizedName  + "_ptr";

        out << "const native_handle_t *"
            << ptrName << ";\n"
            << "_hidl_err = "
            << parcelObj
            << (parcelObjIsPointer ? "->" : ".")
            << "readNullableEmbeddedNativeHandle(\n";

        out.indent();
        out.indent();

        out << parentName
            << ",\n"
            << offsetText
            << ",\n"
            << "&" << ptrName
            << "\n"
            << ");\n\n";

        out.unindent();
        out.unindent();

        handleError(out, mode);
    } else {
        out << "_hidl_err = "
            << parcelObj
            << (parcelObjIsPointer ? "->" : ".")
            << "writeEmbeddedNativeHandle(\n";

        out.indent();
        out.indent();

        out << (nameIsPointer ? ("*" + name) : name)
            << ",\n"
            << parentName
            << ",\n"
            << offsetText
            << ");\n\n";

        out.unindent();
        out.unindent();

        handleError(out, mode);
    }
}

bool HandleType::needsEmbeddedReadWrite() const {
    return true;
}

bool HandleType::deepIsJavaCompatible(std::unordered_set<const Type*>* /* visited */) const {
    return false;
}

static HidlTypeAssertion assertion("hidl_handle", 16 /* size */);
void HandleType::getAlignmentAndSize(size_t *align, size_t *size) const {
    *align = 8;  // hidl_handle
    *size = assertion.size();
}

void HandleType::emitVtsTypeDeclarations(Formatter& out) const {
    out << "type: " << getVtsType() << "\n";
}

}  // namespace android


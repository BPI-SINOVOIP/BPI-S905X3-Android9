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

#ifndef COMPOUND_TYPE_H_

#define COMPOUND_TYPE_H_

#include "Reference.h"
#include "Scope.h"

#include <vector>

namespace android {

struct CompoundType : public Scope {
    enum Style {
        STYLE_STRUCT,
        STYLE_UNION,
    };

    CompoundType(Style style, const char* localName, const FQName& fullName,
                 const Location& location, Scope* parent);

    Style style() const;

    void setFields(std::vector<NamedReference<Type>*>* fields);

    bool isCompoundType() const override;

    bool deepCanCheckEquality(std::unordered_set<const Type*>* visited) const override;

    std::string typeName() const override;

    std::vector<const Reference<Type>*> getReferences() const override;

    status_t validate() const override;
    status_t validateUniqueNames() const;

    std::string getCppType(StorageMode mode,
                           bool specifyNamespaces) const override;

    std::string getJavaType(bool forInitializer) const override;

    std::string getVtsType() const override;

    void emitReaderWriter(
            Formatter &out,
            const std::string &name,
            const std::string &parcelObj,
            bool parcelObjIsPointer,
            bool isReader,
            ErrorMode mode) const override;

    void emitReaderWriterEmbedded(
            Formatter &out,
            size_t depth,
            const std::string &name,
            const std::string &sanitizedName,
            bool nameIsPointer,
            const std::string &parcelObj,
            bool parcelObjIsPointer,
            bool isReader,
            ErrorMode mode,
            const std::string &parentName,
            const std::string &offsetText) const override;

    void emitResolveReferences(
            Formatter &out,
            const std::string &name,
            bool nameIsPointer,
            const std::string &parcelObj,
            bool parcelObjIsPointer,
            bool isReader,
            ErrorMode mode) const override;

    void emitResolveReferencesEmbedded(
            Formatter &out,
            size_t depth,
            const std::string &name,
            const std::string &sanitizedName,
            bool nameIsPointer,
            const std::string &parcelObj,
            bool parcelObjIsPointer,
            bool isReader,
            ErrorMode mode,
            const std::string &parentName,
            const std::string &offsetText) const override;

    void emitJavaReaderWriter(
            Formatter &out,
            const std::string &parcelObj,
            const std::string &argName,
            bool isReader) const override;

    void emitJavaFieldInitializer(
            Formatter &out, const std::string &fieldName) const override;

    void emitJavaFieldReaderWriter(
            Formatter &out,
            size_t depth,
            const std::string &parcelName,
            const std::string &blobName,
            const std::string &fieldName,
            const std::string &offset,
            bool isReader) const override;

    void emitTypeDeclarations(Formatter& out) const override;
    void emitTypeForwardDeclaration(Formatter& out) const override;
    void emitPackageTypeDeclarations(Formatter& out) const override;
    void emitPackageHwDeclarations(Formatter& out) const override;

    void emitTypeDefinitions(Formatter& out, const std::string& prefix) const override;

    void emitJavaTypeDeclarations(Formatter& out, bool atTopLevel) const override;

    bool needsEmbeddedReadWrite() const override;
    bool deepNeedsResolveReferences(std::unordered_set<const Type*>* visited) const override;
    bool resultNeedsDeref() const override;

    void emitVtsTypeDeclarations(Formatter& out) const override;
    void emitVtsAttributeType(Formatter& out) const override;

    bool deepIsJavaCompatible(std::unordered_set<const Type*>* visited) const override;
    bool deepContainsPointer(std::unordered_set<const Type*>* visited) const override;

    void getAlignmentAndSize(size_t *align, size_t *size) const;

    bool containsInterface() const;
private:
    Style mStyle;
    std::vector<NamedReference<Type>*>* mFields;

    void emitStructReaderWriter(
            Formatter &out, const std::string &prefix, bool isReader) const;
    void emitResolveReferenceDef(Formatter& out, const std::string& prefix, bool isReader) const;

    DISALLOW_COPY_AND_ASSIGN(CompoundType);
};

}  // namespace android

#endif  // COMPOUND_TYPE_H_


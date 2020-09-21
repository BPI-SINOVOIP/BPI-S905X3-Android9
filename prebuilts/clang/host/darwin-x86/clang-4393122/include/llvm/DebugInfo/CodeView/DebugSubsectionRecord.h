//===- DebugSubsection.h ------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_DEBUGINFO_CODEVIEW_MODULEDEBUGFRAGMENTRECORD_H
#define LLVM_DEBUGINFO_CODEVIEW_MODULEDEBUGFRAGMENTRECORD_H

#include "llvm/DebugInfo/CodeView/CodeView.h"
#include "llvm/Support/BinaryStreamArray.h"
#include "llvm/Support/BinaryStreamRef.h"
#include "llvm/Support/BinaryStreamWriter.h"
#include "llvm/Support/Endian.h"
#include "llvm/Support/Error.h"

namespace llvm {
namespace codeview {

class DebugSubsection;

// Corresponds to the `CV_DebugSSubsectionHeader_t` structure.
struct DebugSubsectionHeader {
  support::ulittle32_t Kind;   // codeview::DebugSubsectionKind enum
  support::ulittle32_t Length; // number of bytes occupied by this record.
};

class DebugSubsectionRecord {
public:
  DebugSubsectionRecord();
  DebugSubsectionRecord(DebugSubsectionKind Kind, BinaryStreamRef Data,
                        CodeViewContainer Container);

  static Error initialize(BinaryStreamRef Stream, DebugSubsectionRecord &Info,
                          CodeViewContainer Container);

  uint32_t getRecordLength() const;
  DebugSubsectionKind kind() const;
  BinaryStreamRef getRecordData() const;

private:
  CodeViewContainer Container;
  DebugSubsectionKind Kind;
  BinaryStreamRef Data;
};

class DebugSubsectionRecordBuilder {
public:
  DebugSubsectionRecordBuilder(std::shared_ptr<DebugSubsection> Subsection,
                               CodeViewContainer Container);
  uint32_t calculateSerializedLength();
  Error commit(BinaryStreamWriter &Writer) const;

private:
  std::shared_ptr<DebugSubsection> Subsection;
  CodeViewContainer Container;
};

} // namespace codeview

template <> struct VarStreamArrayExtractor<codeview::DebugSubsectionRecord> {
  Error operator()(BinaryStreamRef Stream, uint32_t &Length,
                   codeview::DebugSubsectionRecord &Info) {
    // FIXME: We need to pass the container type through to this function.  In
    // practice this isn't super important since the subsection header describes
    // its length and we can just skip it.  It's more important when writing.
    if (auto EC = codeview::DebugSubsectionRecord::initialize(
            Stream, Info, codeview::CodeViewContainer::Pdb))
      return EC;
    Length = alignTo(Info.getRecordLength(), 4);
    return Error::success();
  }
};

namespace codeview {
typedef VarStreamArray<DebugSubsectionRecord> DebugSubsectionArray;
}
} // namespace llvm

#endif // LLVM_DEBUGINFO_CODEVIEW_MODULEDEBUGFRAGMENTRECORD_H

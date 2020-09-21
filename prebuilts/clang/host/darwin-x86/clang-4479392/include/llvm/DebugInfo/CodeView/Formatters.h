//===- Formatters.h ---------------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_DEBUGINFO_CODEVIEW_FORMATTERS_H
#define LLVM_DEBUGINFO_CODEVIEW_FORMATTERS_H

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/DebugInfo/CodeView/TypeIndex.h"
#include "llvm/Support/FormatAdapters.h"
#include "llvm/Support/FormatProviders.h"
#include "llvm/Support/FormatVariadic.h"

namespace llvm {
namespace codeview {
namespace detail {
class GuidAdapter final : public llvm::FormatAdapter<ArrayRef<uint8_t>> {
  ArrayRef<uint8_t> Guid;

public:
  explicit GuidAdapter(ArrayRef<uint8_t> Guid);
  explicit GuidAdapter(StringRef Guid);
  void format(llvm::raw_ostream &Stream, StringRef Style);
};
}

inline detail::GuidAdapter fmt_guid(StringRef Item) {
  return detail::GuidAdapter(Item);
}

inline detail::GuidAdapter fmt_guid(ArrayRef<uint8_t> Item) {
  return detail::GuidAdapter(Item);
}
}

template <> struct format_provider<codeview::TypeIndex> {
public:
  static void format(const codeview::TypeIndex &V, llvm::raw_ostream &Stream,
                     StringRef Style) {
    if (V.isNoneType())
      Stream << "<no type>";
    else {
      Stream << formatv("{0:X+4}", V.getIndex());
      if (V.isSimple())
        Stream << " (" << codeview::TypeIndex::simpleTypeName(V) << ")";
    }
  }
};
}

#endif

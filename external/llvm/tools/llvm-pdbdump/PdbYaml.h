//===- PdbYAML.h ---------------------------------------------- *- C++ --*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_TOOLS_LLVMPDBDUMP_PDBYAML_H
#define LLVM_TOOLS_LLVMPDBDUMP_PDBYAML_H

#include "OutputStyle.h"

#include "llvm/ADT/Optional.h"
#include "llvm/DebugInfo/PDB/PDBTypes.h"
#include "llvm/DebugInfo/PDB/Raw/PDBFile.h"
#include "llvm/DebugInfo/PDB/Raw/RawConstants.h"
#include "llvm/Support/Endian.h"
#include "llvm/Support/YAMLTraits.h"

#include <vector>

namespace llvm {
namespace pdb {

namespace yaml {
struct MsfHeaders {
  PDBFile::SuperBlock SuperBlock;
  uint32_t NumDirectoryBlocks;
  uint32_t BlockMapOffset;
  std::vector<support::ulittle32_t> DirectoryBlocks;
  uint32_t NumStreams;
  uint32_t FileSize;
};

struct StreamBlockList {
  std::vector<support::ulittle32_t> Blocks;
};

struct PdbInfoStream {
  PdbRaw_ImplVer Version;
  uint32_t Signature;
  uint32_t Age;
  PDB_UniqueId Guid;
};

struct PdbDbiStream {
  PdbRaw_DbiVer VerHeader;
  uint32_t Age;
  uint16_t BuildNumber;
  uint32_t PdbDllVersion;
  uint16_t PdbDllRbld;
  uint16_t Flags;
  PDB_Machine MachineType;
};

struct PdbObject {
  Optional<MsfHeaders> Headers;
  Optional<std::vector<support::ulittle32_t>> StreamSizes;
  Optional<std::vector<StreamBlockList>> StreamMap;
  Optional<PdbInfoStream> PdbStream;
  Optional<PdbDbiStream> DbiStream;
};
}
}
}

namespace llvm {
namespace yaml {

template <> struct MappingTraits<pdb::yaml::PdbObject> {
  static void mapping(IO &IO, pdb::yaml::PdbObject &Obj);
};

template <> struct MappingTraits<pdb::yaml::MsfHeaders> {
  static void mapping(IO &IO, pdb::yaml::MsfHeaders &Obj);
};

template <> struct MappingTraits<pdb::PDBFile::SuperBlock> {
  static void mapping(IO &IO, pdb::PDBFile::SuperBlock &SB);
};

template <> struct MappingTraits<pdb::yaml::StreamBlockList> {
  static void mapping(IO &IO, pdb::yaml::StreamBlockList &SB);
};

template <> struct MappingTraits<pdb::yaml::PdbInfoStream> {
  static void mapping(IO &IO, pdb::yaml::PdbInfoStream &Obj);
};

template <> struct MappingTraits<pdb::yaml::PdbDbiStream> {
  static void mapping(IO &IO, pdb::yaml::PdbDbiStream &Obj);
};
}
}

LLVM_YAML_IS_SEQUENCE_VECTOR(support::ulittle32_t)
LLVM_YAML_IS_SEQUENCE_VECTOR(llvm::pdb::yaml::StreamBlockList)

#endif // LLVM_TOOLS_LLVMPDBDUMP_PDBYAML_H

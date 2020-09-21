/*
 * Copyright (C) 2017 The Android Open Source Project
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

#include "dexter.h"
#include "experimental.h"
#include "slicer/common.h"
#include "slicer/scopeguard.h"
#include "slicer/reader.h"
#include "slicer/writer.h"
#include "slicer/chronometer.h"

#include <getopt.h>
#include <stdio.h>
#include <unistd.h>
#include <memory>
#include <sstream>
#include <map>

// Converts a class name to a type descriptor
// (ex. "java.lang.String" to "Ljava/lang/String;")
static std::string ClassNameToDescriptor(const char* class_name) {
  std::stringstream ss;
  ss << "L";
  for (auto p = class_name; *p != '\0'; ++p) {
    ss << (*p == '.' ? '/' : *p);
  }
  ss << ";";
  return ss.str();
}

void Dexter::PrintHelp() {
  printf("\nDex manipulation tool %s\n\n", VERSION);
  printf("dexter [flags...] [-e classname] [-o outfile] <dexfile>\n");
  printf(" -h : help\n");
  printf(" -s : print stats\n");
  printf(" -e : extract a single class\n");
  printf(" -l : list the classes defined in the dex file\n");
  printf(" -v : verbose output\n");
  printf(" -o : output a new .dex file\n");
  printf(" -d : dissasemble method bodies\n");
  printf(" -m : print .dex layout map\n");
  printf(" --cfg : generate control flow graph (compact|verbose)\n");
  printf("\n");
}

int Dexter::Run() {
  // names for the CFG options
  const std::map<std::string, DexDissasembler::CfgType> cfg_type_names = {
    { "none", DexDissasembler::CfgType::None },
    { "compact", DexDissasembler::CfgType::Compact },
    { "verbose", DexDissasembler::CfgType::Verbose },
  };

  // long cmdline options
  enum { kBuildCFG = 1 };
  const option long_options[] = {
    { "cfg", required_argument, 0, kBuildCFG },
    { 0, 0, 0, 0}
  };

  bool show_help = false;
  int opt = 0;
  int opt_index = 0;
  while ((opt = ::getopt_long(argc_, argv_, "hlsvdmo:e:x:", long_options, &opt_index)) != -1) {
    switch (opt) {
      case kBuildCFG: {
        auto it = cfg_type_names.find(::optarg);
        if (it == cfg_type_names.end()) {
          printf("Invalid --cfg type\n");
          show_help = true;
        }
        cfg_type_ = it->second;
      } break;
      case 's':
        stats_ = true;
        break;
      case 'v':
        verbose_ = true;
        break;
      case 'l':
        list_classes_ = true;
        break;
      case 'e':
        extract_class_ = ::optarg;
        break;
      case 'd':
        dissasemble_ = true;
        break;
      case 'm':
        print_map_ = true;
        break;
      case 'o':
        out_dex_filename_ = ::optarg;
        break;
      case 'x':
        experiments_.push_back(::optarg);
        break;
      default:
        show_help = true;
        break;
    }
  }

  if (show_help || ::optind + 1 != argc_) {
    PrintHelp();
    return 1;
  }

  dex_filename_ = argv_[::optind];
  return ProcessDex();
}

// Print the layout map of the .dex sections
static void PrintDexMap(const dex::Reader& reader) {
  printf("\nSections summary: name, offset, size [count]\n");

  const dex::MapList& dexMap = *reader.DexMapList();
  for (dex::u4 i = 0; i < dexMap.size; ++i) {
    const dex::MapItem& section = dexMap.list[i];
    const char* sectionName = "UNKNOWN";
    switch (section.type) {
      case dex::kHeaderItem:
        sectionName = "HeaderItem";
        break;
      case dex::kStringIdItem:
        sectionName = "StringIdItem";
        break;
      case dex::kTypeIdItem:
        sectionName = "TypeIdItem";
        break;
      case dex::kProtoIdItem:
        sectionName = "ProtoIdItem";
        break;
      case dex::kFieldIdItem:
        sectionName = "FieldIdItem";
        break;
      case dex::kMethodIdItem:
        sectionName = "MethodIdItem";
        break;
      case dex::kClassDefItem:
        sectionName = "ClassDefItem";
        break;
      case dex::kMapList:
        sectionName = "MapList";
        break;
      case dex::kTypeList:
        sectionName = "TypeList";
        break;
      case dex::kAnnotationSetRefList:
        sectionName = "AnnotationSetRefList";
        break;
      case dex::kAnnotationSetItem:
        sectionName = "AnnotationSetItem";
        break;
      case dex::kClassDataItem:
        sectionName = "ClassDataItem";
        break;
      case dex::kCodeItem:
        sectionName = "CodeItem";
        break;
      case dex::kStringDataItem:
        sectionName = "StringDataItem";
        break;
      case dex::kDebugInfoItem:
        sectionName = "DebugInfoItem";
        break;
      case dex::kAnnotationItem:
        sectionName = "AnnotationItem";
        break;
      case dex::kEncodedArrayItem:
        sectionName = "EncodedArrayItem";
        break;
      case dex::kAnnotationsDirectoryItem:
        sectionName = "AnnotationsDirectoryItem";
        break;
    }

    dex::u4 sectionByteSize = (i == dexMap.size - 1)
                                  ? reader.Header()->file_size - section.offset
                                  : dexMap.list[i + 1].offset - section.offset;

    printf("  %-25s : %8x, %8x  [%u]\n", sectionName, section.offset,
           sectionByteSize, section.size);
  }
}

// Print .dex IR stats
static void PrintDexIrStats(std::shared_ptr<const ir::DexFile> dex_ir) {
  printf("\n.dex IR statistics:\n");
  printf("  strings                       : %zu\n", dex_ir->strings.size());
  printf("  types                         : %zu\n", dex_ir->types.size());
  printf("  protos                        : %zu\n", dex_ir->protos.size());
  printf("  fields                        : %zu\n", dex_ir->fields.size());
  printf("  encoded_fields                : %zu\n", dex_ir->encoded_fields.size());
  printf("  methods                       : %zu\n", dex_ir->methods.size());
  printf("  encoded_methods               : %zu\n", dex_ir->encoded_methods.size());
  printf("  classes                       : %zu\n", dex_ir->classes.size());
  printf("  type_lists                    : %zu\n", dex_ir->type_lists.size());
  printf("  code                          : %zu\n", dex_ir->code.size());
  printf("  debug_info                    : %zu\n", dex_ir->debug_info.size());
  printf("  encoded_values                : %zu\n", dex_ir->encoded_values.size());
  printf("  encoded_arrays                : %zu\n", dex_ir->encoded_arrays.size());
  printf("  annotations                   : %zu\n", dex_ir->annotations.size());
  printf("  annotation_elements           : %zu\n", dex_ir->annotation_elements.size());
  printf("  annotation_sets               : %zu\n", dex_ir->annotation_sets.size());
  printf("  annotation_set_ref_lists      : %zu\n", dex_ir->annotation_set_ref_lists.size());
  printf("  annotations_directories       : %zu\n", dex_ir->annotations_directories.size());
  printf("  field_annotations             : %zu\n", dex_ir->field_annotations.size());
  printf("  method_annotations            : %zu\n", dex_ir->method_annotations.size());
  printf("  param_annotations             : %zu\n", dex_ir->param_annotations.size());
  printf("\n");
}

// Print .dex file stats
static void PrintDexFileStats(const dex::Reader& reader) {
  auto header = reader.Header();
  auto map_list = reader.DexMapList();
  printf("\n.dex file statistics:\n");
  printf("  file_size                     : %u\n", header->file_size);
  printf("  data_size                     : %u\n", header->data_size);
  printf("  link_size                     : %u\n", header->link_size);
  printf("  class_defs_size               : %u\n", header->class_defs_size);
  printf("  string_ids_size               : %u\n", header->string_ids_size);
  printf("  type_ids_size                 : %u\n", header->type_ids_size);
  printf("  proto_ids_size                : %u\n", header->proto_ids_size);
  printf("  field_ids_size                : %u\n", header->field_ids_size);
  printf("  method_ids_size               : %u\n", header->method_ids_size);
  printf("  map_list_size                 : %u\n", map_list->size);
  printf("\n");
}

// List all the classes defined in the dex file
static void ListClasses(const dex::Reader& reader) {
  printf("\nClass definitions:\n");
  auto classes = reader.ClassDefs();
  auto types = reader.TypeIds();
  for (dex::u4 i = 0; i < classes.size(); ++i) {
    auto typeId = types[classes[i].class_idx];
    const char* descriptor = reader.GetStringMUTF8(typeId.descriptor_idx);
    printf("  %s\n", dex::DescriptorToDecl(descriptor).c_str());
  }
  printf("\n");
}

// Create a new in-memory .dex image and optionally write it to disk
//
// NOTE: we always create the new in-memory image, even if we don't write it
// to an output file, for aggresive code coverage and perf timings
//
bool Dexter::CreateNewImage(std::shared_ptr<ir::DexFile> dex_ir) {
  // our custom (and trivial) allocator for dex::Writer
  struct Allocator : public dex::Writer::Allocator {
    virtual void* Allocate(size_t size) { return ::malloc(size); }
    virtual void Free(void* ptr) { ::free(ptr); }
  };

  size_t new_image_size = 0;
  dex::u1* new_image = nullptr;
  Allocator allocator;

  {
    slicer::Chronometer chrono(writer_time_);

    dex::Writer writer(dex_ir);
    new_image = writer.CreateImage(&allocator, &new_image_size);
  }

  if (new_image == nullptr) {
    printf("ERROR: Can't create a new .dex image\n");
    return false;
  }

  SLICER_SCOPE_EXIT {
    allocator.Free(new_image);
  };

  // write the new .dex image to disk?
  if (out_dex_filename_ != nullptr) {
    if (verbose_) {
      printf("\nWriting: %s\n", out_dex_filename_);
    }

    // create output file
    FILE* out_file = fopen(out_dex_filename_, "wb");
    if (out_file == nullptr) {
      printf("ERROR: Can't create output .dex file (%s)\n", out_dex_filename_);
      return false;
    }

    SLICER_SCOPE_EXIT {
      fclose(out_file);
    };

    // write the new image
    SLICER_CHECK(fwrite(new_image, 1, new_image_size, out_file) == new_image_size);
  }

  return true;
}

// Main entry point for processing an input .dex file
int Dexter::ProcessDex() {
  if (verbose_) {
    printf("\nReading: %s\n", dex_filename_);
  }

  // open input file
  FILE* in_file = fopen(dex_filename_, "rb");
  if (in_file == nullptr) {
    printf("ERROR: Can't open input .dex file (%s)\n", dex_filename_);
    return 1;
  }

  SLICER_SCOPE_EXIT {
    fclose(in_file);
  };

  // calculate file size
  fseek(in_file, 0, SEEK_END);
  size_t in_size = ftell(in_file);

  // allocate the in-memory .dex image buffer
  std::unique_ptr<dex::u1[]> in_buff(new dex::u1[in_size]);

  // read input .dex file
  fseek(in_file, 0, SEEK_SET);
  SLICER_CHECK(fread(in_buff.get(), 1, in_size, in_file) == in_size);

  // initialize the .dex reader
  dex::Reader reader(in_buff.get(), in_size);

  // print the .dex map?
  if (print_map_) {
    PrintDexMap(reader);
  }

  // list classes?
  if (list_classes_) {
    ListClasses(reader);
  }

  // parse the .dex image
  {
    slicer::Chronometer chrono(reader_time_);

    if (extract_class_ != nullptr) {
      // extract the specified class only
      std::string descriptor = ClassNameToDescriptor(extract_class_);
      dex::u4 class_idx = reader.FindClassIndex(descriptor.c_str());
      if (class_idx != dex::kNoIndex) {
        reader.CreateClassIr(class_idx);
      } else {
        printf("ERROR: Can't find class (%s)\n", extract_class_);
        return 1;
      }
    } else {
      // build the full .dex IR
      reader.CreateFullIr();
    }
  }

  auto dex_ir = reader.GetIr();

  // experiments
  for (auto experiment : experiments_) {
    slicer::Chronometer chrono(experiments_time_, true);
    if (verbose_) {
      printf("\nRunning experiment '%s' ... \n", experiment);
    }
    experimental::Run(experiment, dex_ir);
  }

  // dissasemble method bodies?
  if (dissasemble_) {
    DexDissasembler disasm(dex_ir, cfg_type_);
    disasm.DumpAllMethods();
  }

  if(!CreateNewImage(dex_ir)) {
    return 1;
  }

  // stats?
  if (stats_) {
    PrintDexFileStats(reader);
    PrintDexIrStats(dex_ir);
  }

  if (verbose_) {
    printf("\nDone (reader: %.3fms, writer: %.3fms", reader_time_, writer_time_);
    if (!experiments_.empty()) {
      printf(", experiments: %.3fms", experiments_time_);
    }
    printf(")\n");
  }

  // done
  return 0;
}

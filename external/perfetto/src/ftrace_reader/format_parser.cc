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

#include "perfetto/ftrace_reader/format_parser.h"

#include <string.h>

#include <iosfwd>
#include <iostream>
#include <memory>
#include <regex>
#include <string>
#include <vector>

#include "perfetto/base/string_splitter.h"
#include "perfetto/base/utils.h"

namespace perfetto {
namespace {

#define MAX_FIELD_LENGTH 127
#define STRINGIFY(x) STRINGIFY2(x)
#define STRINGIFY2(x) #x

const char* kCommonFieldPrefix = "common_";

bool IsCommonFieldName(const std::string& name) {
  return name.compare(0, strlen(kCommonFieldPrefix), kCommonFieldPrefix) == 0;
}

bool IsCIdentifier(const std::string& s) {
  for (const char c : s) {
    if (!(std::isalnum(c) || c == '_'))
      return false;
  }
  return !s.empty() && !std::isdigit(s[0]);
}

}  // namespace

// For example:
// "int foo" -> "foo"
// "u8 foo[(int)sizeof(struct blah)]" -> "foo"
// "char[] foo[16]" -> "foo"
// "something_went_wrong" -> ""
// "" -> ""
std::string GetNameFromTypeAndName(const std::string& type_and_name) {
  size_t right = type_and_name.size();
  if (right == 0)
    return "";

  if (type_and_name[type_and_name.size() - 1] == ']') {
    right = type_and_name.rfind('[');
    if (right == std::string::npos)
      return "";
  }

  size_t left = type_and_name.rfind(' ', right);
  if (left == std::string::npos)
    return "";
  left++;

  std::string result = type_and_name.substr(left, right - left);
  if (!IsCIdentifier(result))
    return "";

  return result;
}

bool ParseFtraceEvent(const std::string& input, FtraceEvent* output) {
  char buffer[MAX_FIELD_LENGTH + 1];

  bool has_id = false;
  bool has_name = false;

  uint32_t id = 0;
  std::string name;
  std::vector<FtraceEvent::Field> common_fields;
  std::vector<FtraceEvent::Field> fields;

  for (base::StringSplitter ss(input, '\n'); ss.Next();) {
    const char* line = ss.cur_token();
    if (!has_id && sscanf(line, "ID: %d", &id) == 1) {
      has_id = true;
      continue;
    }

    if (!has_name &&
        sscanf(line, "name: %" STRINGIFY(MAX_FIELD_LENGTH) "s", buffer) == 1) {
      name = std::string(buffer);
      has_name = true;
      continue;
    }

    if (strcmp("format:", line) == 0) {
      continue;
    }

    uint16_t offset = 0;
    uint16_t size = 0;
    int is_signed = 0;
    if (sscanf(line,
               "\tfield:%" STRINGIFY(MAX_FIELD_LENGTH) "[^;];\toffset: "
                                                       "%hu;\tsize: "
                                                       "%hu;\tsigned: %d;",
               buffer, &offset, &size, &is_signed) == 4) {
      std::string type_and_name(buffer);

      FtraceEvent::Field field{type_and_name, offset, size, is_signed == 1};

      if (IsCommonFieldName(GetNameFromTypeAndName(type_and_name))) {
        common_fields.push_back(field);
      } else {
        fields.push_back(field);
      }

      continue;
    }

    if (strncmp(line, "print fmt:", 10) == 0) {
      break;
    }

    if (output)
      fprintf(stderr, "Cannot parse line: \"%s\"\n", line);
    return false;
  }

  if (!has_id || !has_name || fields.empty()) {
    if (output)
      fprintf(stderr, "Could not parse format file: %s.\n",
              !has_id ? "no ID found"
                      : !has_name ? "no name found" : "no fields found");
    return false;
  }

  if (!output)
    return true;

  output->id = id;
  output->name = name;
  output->fields = std::move(fields);
  output->common_fields = std::move(common_fields);

  return true;
}

::std::ostream& operator<<(::std::ostream& os,
                           const FtraceEvent::Field& field) {
  PrintTo(field, &os);
  return os;
}

// Allow gtest to pretty print FtraceEvent::Field.
void PrintTo(const FtraceEvent::Field& field, ::std::ostream* os) {
  *os << "FtraceEvent::Field(" << field.type_and_name << ", " << field.offset
      << ", " << field.size << ", " << field.is_signed << ")";
}

}  // namespace perfetto

/*
 * Copyright (C) 2018 The Android Open Source Project
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

#ifndef TOOLS_FTRACE_PROTO_GEN_FTRACE_PROTO_GEN_H_
#define TOOLS_FTRACE_PROTO_GEN_FTRACE_PROTO_GEN_H_

#include <set>
#include <string>
#include <vector>

#include "perfetto/ftrace_reader/format_parser.h"

namespace perfetto {

struct Proto {
  struct Field {
    std::string type;
    std::string name;
    uint32_t number;
  };
  std::string name;
  std::vector<Field> fields;

  std::string ToString();
};

void PrintFtraceEventProtoAdditions(const std::set<std::string>& events);
void PrintEventFormatterMain(const std::set<std::string>& events);
void PrintEventFormatterUsingStatements(const std::set<std::string>& events);
void PrintEventFormatterFunctions(const std::set<std::string>& events);
void PrintInodeHandlerMain(const std::string& event_name,
                           const perfetto::Proto& proto);

bool GenerateProto(const FtraceEvent& format, Proto* proto_out);
std::string InferProtoType(const FtraceEvent::Field& field);

std::set<std::string> GetWhitelistedEvents(const std::string& whitelist_path);
std::string SingleEventInfo(perfetto::FtraceEvent format,
                            perfetto::Proto proto,
                            const std::string& group,
                            const std::string& proto_field_id);
void GenerateEventInfo(const std::vector<std::string>& events_info);

}  // namespace perfetto

#endif  // TOOLS_FTRACE_PROTO_GEN_FTRACE_PROTO_GEN_H_

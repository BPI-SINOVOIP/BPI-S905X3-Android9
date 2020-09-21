/*
 *
 * Copyright 2015, The Android Open Source Project
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

#ifndef SYSTEM_EXTRAS_PERFPROFD_CONFIGREADER_H_
#define SYSTEM_EXTRAS_PERFPROFD_CONFIGREADER_H_

#include <string>
#include <map>

#include "config.h"

//
// This table describes the perfprofd config file syntax in terms of
// key/value pairs.  Values come in two flavors: strings, or unsigned
// integers. In the latter case the reader sets allowable
// minimum/maximum for the setting.
//
class ConfigReader {

 public:
  ConfigReader();
  ~ConfigReader();

  // Ask for the current setting of a config item
  unsigned getUnsignedValue(const char *key) const;
  bool getBoolValue(const char *key) const;
  std::string getStringValue(const char *key) const;

  // read the specified config file, applying any settings it contains
  // returns true for successful read, false if conf file cannot be opened.
  bool readFile();

  bool Read(const std::string& data, bool fail_on_error);

  // set/get path to config file
  static void setConfigFilePath(const char *path);
  static const char *getConfigFilePath();

  // override a config item (for unit testing purposes)
  void overrideUnsignedEntry(const char *key, unsigned new_value);

  void FillConfig(Config* config);

 private:
  void addUnsignedEntry(const char *key,
                        unsigned default_value,
                        unsigned min_value,
                        unsigned max_value);
  void addStringEntry(const char *key, const char *default_value);
  void addDefaultEntries();
  bool parseLine(const char *key, const char *value, unsigned linecount);

  typedef struct { unsigned minv, maxv; } values;
  std::map<std::string, values> u_info;
  std::map<std::string, unsigned> u_entries;
  std::map<std::string, std::string> s_entries;
  bool trace_config_read;
};

#endif

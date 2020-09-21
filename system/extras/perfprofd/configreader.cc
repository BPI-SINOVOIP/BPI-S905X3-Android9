/*
**
** Copyright 2015, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <algorithm>
#include <cstring>
#include <sstream>

#include <android-base/file.h>
#include <android-base/logging.h>

#include "configreader.h"

//
// Config file path
//
static const char *config_file_path =
    "/data/data/com.google.android.gms/files/perfprofd.conf";

ConfigReader::ConfigReader()
    : trace_config_read(false)
{
  addDefaultEntries();
}

ConfigReader::~ConfigReader()
{
}

const char *ConfigReader::getConfigFilePath()
{
  return config_file_path;
}

void ConfigReader::setConfigFilePath(const char *path)
{
  config_file_path = strdup(path);
  LOG(INFO) << "config file path set to " << config_file_path;
}

//
// Populate the reader with the set of allowable entries
//
void ConfigReader::addDefaultEntries()
{
  struct DummyConfig : public Config {
    void Sleep(size_t seconds) override {}
    bool IsProfilingEnabled() const override { return false; }
  };
  DummyConfig config;

  // Average number of seconds between perf profile collections (if
  // set to 100, then over time we want to see a perf profile
  // collected every 100 seconds). The actual time within the interval
  // for the collection is chosen randomly.
  addUnsignedEntry("collection_interval", config.collection_interval_in_s, 0, UINT32_MAX);

  // Use the specified fixed seed for random number generation (unit
  // testing)
  addUnsignedEntry("use_fixed_seed", config.use_fixed_seed, 0, UINT32_MAX);

  // For testing purposes, number of times to iterate through main
  // loop.  Value of zero indicates that we should loop forever.
  addUnsignedEntry("main_loop_iterations", config.main_loop_iterations, 0, UINT32_MAX);

  // Destination directory (where to write profiles). This location
  // chosen since it is accessible to the uploader service.
  addStringEntry("destination_directory", config.destination_directory.c_str());

  // Config directory (where to read configs).
  addStringEntry("config_directory", config.config_directory.c_str());

  // Full path to 'perf' executable.
  addStringEntry("perf_path", config.perf_path.c_str());

  // Desired sampling period (passed to perf -c option).
  addUnsignedEntry("sampling_period", config.sampling_period, 0, UINT32_MAX);
  // Desired sampling frequency (passed to perf -f option).
  addUnsignedEntry("sampling_frequency", config.sampling_frequency, 0, UINT32_MAX);

  // Length of time to collect samples (number of seconds for 'perf
  // record -a' run).
  addUnsignedEntry("sample_duration", config.sample_duration_in_s, 1, 600);

  // If this parameter is non-zero it will cause perfprofd to
  // exit immediately if the build type is not userdebug or eng.
  // Currently defaults to 1 (true).
  addUnsignedEntry("only_debug_build", config.only_debug_build ? 1 : 0, 0, 1);

  // If the "mpdecision" service is running at the point we are ready
  // to kick off a profiling run, then temporarily disable the service
  // and hard-wire all cores on prior to the collection run, provided
  // that the duration of the recording is less than or equal to the value of
  // 'hardwire_cpus_max_duration'.
  addUnsignedEntry("hardwire_cpus", config.hardwire_cpus, 0, 1);
  addUnsignedEntry("hardwire_cpus_max_duration",
                   config.hardwire_cpus_max_duration_in_s,
                   1,
                   UINT32_MAX);

  // Maximum number of unprocessed profiles we can accumulate in the
  // destination directory. Once we reach this limit, we continue
  // to collect, but we just overwrite the most recent profile.
  addUnsignedEntry("max_unprocessed_profiles", config.max_unprocessed_profiles, 1, UINT32_MAX);

  // If set to 1, pass the -g option when invoking 'perf' (requests
  // stack traces as opposed to flat profile).
  addUnsignedEntry("stack_profile", config.stack_profile ? 1 : 0, 0, 1);

  // For unit testing only: if set to 1, emit info messages on config
  // file parsing.
  addUnsignedEntry("trace_config_read", config.trace_config_read ? 1 : 0, 0, 1);

  // Control collection of various additional profile tags
  addUnsignedEntry("collect_cpu_utilization", config.collect_cpu_utilization ? 1 : 0, 0, 1);
  addUnsignedEntry("collect_charging_state", config.collect_charging_state ? 1 : 0, 0, 1);
  addUnsignedEntry("collect_booting", config.collect_booting ? 1 : 0, 0, 1);
  addUnsignedEntry("collect_camera_active", config.collect_camera_active ? 1 : 0, 0, 1);

  // If true, use an ELF symbolizer to on-device symbolize.
  addUnsignedEntry("use_elf_symbolizer", config.use_elf_symbolizer ? 1 : 0, 0, 1);

  // If true, use libz to compress the output proto.
  addUnsignedEntry("compress", config.compress ? 1 : 0, 0, 1);

  // If true, send the proto to dropbox instead of to a file.
  addUnsignedEntry("dropbox", config.send_to_dropbox ? 1 : 0, 0, 1);

  // The pid of the process to profile. May be negative, in which case
  // the whole system will be profiled.
  addUnsignedEntry("process", static_cast<uint32_t>(-1), 0, UINT32_MAX);
}

void ConfigReader::addUnsignedEntry(const char *key,
                                    unsigned default_value,
                                    unsigned min_value,
                                    unsigned max_value)
{
  std::string ks(key);
  CHECK(u_entries.find(ks) == u_entries.end() &&
        s_entries.find(ks) == s_entries.end())
      << "internal error -- duplicate entry for key " << key;
  values vals;
  vals.minv = min_value;
  vals.maxv = max_value;
  u_info[ks] = vals;
  u_entries[ks] = default_value;
}

void ConfigReader::addStringEntry(const char *key, const char *default_value)
{
  std::string ks(key);
  CHECK(u_entries.find(ks) == u_entries.end() &&
        s_entries.find(ks) == s_entries.end())
      << "internal error -- duplicate entry for key " << key;
  CHECK(default_value != nullptr) << "internal error -- bad default value for key " << key;
  s_entries[ks] = std::string(default_value);
}

unsigned ConfigReader::getUnsignedValue(const char *key) const
{
  std::string ks(key);
  auto it = u_entries.find(ks);
  CHECK(it != u_entries.end());
  return it->second;
}

bool ConfigReader::getBoolValue(const char *key) const
{
  std::string ks(key);
  auto it = u_entries.find(ks);
  CHECK(it != u_entries.end());
  return it->second != 0;
}

std::string ConfigReader::getStringValue(const char *key) const
{
  std::string ks(key);
  auto it = s_entries.find(ks);
  CHECK(it != s_entries.end());
  return it->second;
}

void ConfigReader::overrideUnsignedEntry(const char *key, unsigned new_value)
{
  std::string ks(key);
  auto it = u_entries.find(ks);
  CHECK(it != u_entries.end());
  values vals;
  auto iit = u_info.find(key);
  CHECK(iit != u_info.end());
  vals = iit->second;
  CHECK(new_value >= vals.minv && new_value <= vals.maxv);
  it->second = new_value;
  LOG(INFO) << "option " << key << " overridden to " << new_value;
}


//
// Parse a key=value pair read from the config file. This will issue
// warnings or errors to the system logs if the line can't be
// interpreted properly.
//
bool ConfigReader::parseLine(const char *key,
                             const char *value,
                             unsigned linecount)
{
  assert(key);
  assert(value);

  auto uit = u_entries.find(key);
  if (uit != u_entries.end()) {
    unsigned uvalue = 0;
    if (isdigit(value[0]) == 0 || sscanf(value, "%u", &uvalue) != 1) {
      LOG(WARNING) << "line " << linecount << ": malformed unsigned value (ignored)";
      return false;
    } else {
      values vals;
      auto iit = u_info.find(key);
      assert(iit != u_info.end());
      vals = iit->second;
      if (uvalue < vals.minv || uvalue > vals.maxv) {
        LOG(WARNING) << "line " << linecount << ": "
                     << "specified value " << uvalue << " for '" << key << "' "
                     << "outside permitted range [" << vals.minv << " " << vals.maxv
                     << "] (ignored)";
        return false;
      } else {
        if (trace_config_read) {
          LOG(INFO) << "option " << key << " set to " << uvalue;
        }
        uit->second = uvalue;
      }
    }
    trace_config_read = (getUnsignedValue("trace_config_read") != 0);
    return true;
  }

  auto sit = s_entries.find(key);
  if (sit != s_entries.end()) {
    if (trace_config_read) {
      LOG(INFO) << "option " << key << " set to " << value;
    }
    sit->second = std::string(value);
    return true;
  }

  LOG(WARNING) << "line " << linecount << ": unknown option '" << key << "' ignored";
  return false;
}

static bool isblank(const std::string &line)
{
  auto non_space = [](char c) { return isspace(c) == 0; };
  return std::find_if(line.begin(), line.end(), non_space) == line.end();
}



bool ConfigReader::readFile()
{
  std::string contents;
  if (! android::base::ReadFileToString(config_file_path, &contents)) {
    return false;
  }
  return Read(contents, /* fail_on_error */ false);
}

bool ConfigReader::Read(const std::string& content, bool fail_on_error) {
  std::stringstream ss(content);
  std::string line;
  for (unsigned linecount = 1;
       std::getline(ss,line,'\n');
       linecount += 1)
  {

    // comment line?
    if (line[0] == '#') {
      continue;
    }

    // blank line?
    if (isblank(line.c_str())) {
      continue;
    }

    // look for X=Y assignment
    auto efound = line.find('=');
    if (efound == std::string::npos) {
      LOG(WARNING) << "line " << linecount << ": line malformed (no '=' found)";
      if (fail_on_error) {
        return false;
      }
      continue;
    }

    std::string key(line.substr(0, efound));
    std::string value(line.substr(efound+1, std::string::npos));

    bool parse_success = parseLine(key.c_str(), value.c_str(), linecount);
    if (fail_on_error && !parse_success) {
      return false;
    }
  }

  return true;
}

void ConfigReader::FillConfig(Config* config) {
  config->collection_interval_in_s = getUnsignedValue("collection_interval");

  config->use_fixed_seed = getUnsignedValue("use_fixed_seed");

  config->main_loop_iterations = getUnsignedValue("main_loop_iterations");

  config->destination_directory = getStringValue("destination_directory");

  config->config_directory = getStringValue("config_directory");

  config->perf_path = getStringValue("perf_path");

  config->sampling_period = getUnsignedValue("sampling_period");
  config->sampling_frequency = getUnsignedValue("sampling_frequency");

  config->sample_duration_in_s = getUnsignedValue("sample_duration");

  config->only_debug_build = getBoolValue("only_debug_build");

  config->hardwire_cpus = getBoolValue("hardwire_cpus");
  config->hardwire_cpus_max_duration_in_s = getUnsignedValue("hardwire_cpus_max_duration");

  config->max_unprocessed_profiles = getUnsignedValue("max_unprocessed_profiles");

  config->stack_profile = getBoolValue("stack_profile");

  config->trace_config_read = getBoolValue("trace_config_read");

  config->collect_cpu_utilization = getBoolValue("collect_cpu_utilization");
  config->collect_charging_state = getBoolValue("collect_charging_state");
  config->collect_booting = getBoolValue("collect_booting");
  config->collect_camera_active = getBoolValue("collect_camera_active");

  config->process = static_cast<int32_t>(getUnsignedValue("process"));
  config->use_elf_symbolizer = getBoolValue("use_elf_symbolizer");
  config->compress = getBoolValue("compress");
  config->send_to_dropbox = getBoolValue("dropbox");
}

/******************************************************************************
 *
 *  Copyright 2014 Google, Inc.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

#define LOG_TAG "bt_btif_config"

#include "btif_config.h"

#include <base/logging.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <string>

#include <mutex>

#include "bt_types.h"
#include "btcore/include/module.h"
#include "btif_api.h"
#include "btif_common.h"
#include "btif_config_transcode.h"
#include "btif_util.h"
#include "osi/include/alarm.h"
#include "osi/include/allocator.h"
#include "osi/include/compat.h"
#include "osi/include/config.h"
#include "osi/include/log.h"
#include "osi/include/osi.h"
#include "osi/include/properties.h"

#define BT_CONFIG_SOURCE_TAG_NUM 1010001

#define INFO_SECTION "Info"
#define FILE_TIMESTAMP "TimeCreated"
#define FILE_SOURCE "FileSource"
#define TIME_STRING_LENGTH sizeof("YYYY-MM-DD HH:MM:SS")
static const char* TIME_STRING_FORMAT = "%Y-%m-%d %H:%M:%S";

// TODO(armansito): Find a better way than searching by a hardcoded path.
#if defined(OS_GENERIC)
static const char* CONFIG_FILE_PATH = "bt_config.conf";
static const char* CONFIG_BACKUP_PATH = "bt_config.bak";
static const char* CONFIG_LEGACY_FILE_PATH = "bt_config.xml";
#else   // !defined(OS_GENERIC)
static const char* CONFIG_FILE_PATH = "/data/misc/bluedroid/bt_config.conf";
static const char* CONFIG_BACKUP_PATH = "/data/misc/bluedroid/bt_config.bak";
static const char* CONFIG_LEGACY_FILE_PATH =
    "/data/misc/bluedroid/bt_config.xml";
#endif  // defined(OS_GENERIC)
static const period_ms_t CONFIG_SETTLE_PERIOD_MS = 3000;

static void timer_config_save_cb(void* data);
static void btif_config_write(uint16_t event, char* p_param);
static bool is_factory_reset(void);
static void delete_config_files(void);
static void btif_config_remove_unpaired(config_t* config);
static void btif_config_remove_restricted(config_t* config);
static std::unique_ptr<config_t> btif_config_open(const char* filename);

static enum ConfigSource {
  NOT_LOADED,
  ORIGINAL,
  BACKUP,
  LEGACY,
  NEW_FILE,
  RESET
} btif_config_source = NOT_LOADED;

static int btif_config_devices_loaded = -1;
static char btif_config_time_created[TIME_STRING_LENGTH];

// TODO(zachoverflow): Move these two functions out, because they are too
// specific for this file
// {grumpy-cat/no, monty-python/you-make-me-sad}
bool btif_get_device_type(const RawAddress& bda, int* p_device_type) {
  if (p_device_type == NULL) return false;

  std::string addrstr = bda.ToString();
  const char* bd_addr_str = addrstr.c_str();

  if (!btif_config_get_int(bd_addr_str, "DevType", p_device_type)) return false;

  LOG_DEBUG(LOG_TAG, "%s: Device [%s] type %d", __func__, bd_addr_str,
            *p_device_type);
  return true;
}

bool btif_get_address_type(const RawAddress& bda, int* p_addr_type) {
  if (p_addr_type == NULL) return false;

  std::string addrstr = bda.ToString();
  const char* bd_addr_str = addrstr.c_str();

  if (!btif_config_get_int(bd_addr_str, "AddrType", p_addr_type)) return false;

  LOG_DEBUG(LOG_TAG, "%s: Device [%s] address type %d", __func__, bd_addr_str,
            *p_addr_type);
  return true;
}

static std::mutex config_lock;  // protects operations on |config|.
static std::unique_ptr<config_t> config;
static alarm_t* config_timer;

// Module lifecycle functions

static future_t* init(void) {
  std::unique_lock<std::mutex> lock(config_lock);

  if (is_factory_reset()) delete_config_files();

  std::string file_source;

  config = btif_config_open(CONFIG_FILE_PATH);
  btif_config_source = ORIGINAL;
  if (!config) {
    LOG_WARN(LOG_TAG, "%s unable to load config file: %s; using backup.",
             __func__, CONFIG_FILE_PATH);
    config = btif_config_open(CONFIG_BACKUP_PATH);
    btif_config_source = BACKUP;
    file_source = "Backup";
  }
  if (!config) {
    LOG_WARN(LOG_TAG,
             "%s unable to load backup; attempting to transcode legacy file.",
             __func__);
    config = btif_config_transcode(CONFIG_LEGACY_FILE_PATH);
    btif_config_source = LEGACY;
    file_source = "Legacy";
  }
  if (!config) {
    LOG_ERROR(LOG_TAG,
              "%s unable to transcode legacy file; creating empty config.",
              __func__);
    config = config_new_empty();
    btif_config_source = NEW_FILE;
    file_source = "Empty";
  }

  if (!file_source.empty())
    config_set_string(config.get(), INFO_SECTION, FILE_SOURCE, file_source);

  btif_config_remove_unpaired(config.get());

  // Cleanup temporary pairings if we have left guest mode
  if (!is_restricted_mode()) btif_config_remove_restricted(config.get());

  // Read or set config file creation timestamp
  const std::string* time_str;
  time_str = config_get_string(*config, INFO_SECTION, FILE_TIMESTAMP, NULL);
  if (time_str != NULL) {
    strlcpy(btif_config_time_created, time_str->c_str(), TIME_STRING_LENGTH);
  } else {
    time_t current_time = time(NULL);
    struct tm* time_created = localtime(&current_time);
    strftime(btif_config_time_created, TIME_STRING_LENGTH, TIME_STRING_FORMAT,
             time_created);
    config_set_string(config.get(), INFO_SECTION, FILE_TIMESTAMP,
                      btif_config_time_created);
  }

  // TODO(sharvil): use a non-wake alarm for this once we have
  // API support for it. There's no need to wake the system to
  // write back to disk.
  config_timer = alarm_new("btif.config");
  if (!config_timer) {
    LOG_ERROR(LOG_TAG, "%s unable to create alarm.", __func__);
    goto error;
  }

  LOG_EVENT_INT(BT_CONFIG_SOURCE_TAG_NUM, btif_config_source);

  return future_new_immediate(FUTURE_SUCCESS);

error:
  alarm_free(config_timer);
  config.reset();
  config_timer = NULL;
  btif_config_source = NOT_LOADED;
  return future_new_immediate(FUTURE_FAIL);
}

static std::unique_ptr<config_t> btif_config_open(const char* filename) {
  std::unique_ptr<config_t> config = config_new(filename);
  if (!config) return nullptr;

  if (!config_has_section(*config, "Adapter")) {
    LOG_ERROR(LOG_TAG, "Config is missing adapter section");
    return nullptr;
  }

  return config;
}

static future_t* shut_down(void) {
  btif_config_flush();
  return future_new_immediate(FUTURE_SUCCESS);
}

static future_t* clean_up(void) {
  btif_config_flush();

  alarm_free(config_timer);
  config_timer = NULL;

  std::unique_lock<std::mutex> lock(config_lock);
  config.reset();
  return future_new_immediate(FUTURE_SUCCESS);
}

EXPORT_SYMBOL module_t btif_config_module = {.name = BTIF_CONFIG_MODULE,
                                             .init = init,
                                             .start_up = NULL,
                                             .shut_down = shut_down,
                                             .clean_up = clean_up};

bool btif_config_has_section(const char* section) {
  CHECK(config != NULL);
  CHECK(section != NULL);

  std::unique_lock<std::mutex> lock(config_lock);
  return config_has_section(*config, section);
}

bool btif_config_exist(const std::string& section, const std::string& key) {
  CHECK(config != NULL);

  std::unique_lock<std::mutex> lock(config_lock);
  return config_has_key(*config, section, key);
}

bool btif_config_get_int(const std::string& section, const std::string& key,
                         int* value) {
  CHECK(config != NULL);
  CHECK(value != NULL);

  std::unique_lock<std::mutex> lock(config_lock);
  bool ret = config_has_key(*config, section, key);
  if (ret) *value = config_get_int(*config, section, key, *value);

  return ret;
}

bool btif_config_set_int(const std::string& section, const std::string& key,
                         int value) {
  CHECK(config != NULL);

  std::unique_lock<std::mutex> lock(config_lock);
  config_set_int(config.get(), section, key, value);

  return true;
}

bool btif_config_get_uint64(const std::string& section, const std::string& key,
                            uint64_t* value) {
  CHECK(config != NULL);
  CHECK(value != NULL);

  std::unique_lock<std::mutex> lock(config_lock);
  bool ret = config_has_key(*config, section, key);
  if (ret) *value = config_get_uint64(*config, section, key, *value);

  return ret;
}

bool btif_config_set_uint64(const std::string& section, const std::string& key,
                            uint64_t value) {
  CHECK(config != NULL);

  std::unique_lock<std::mutex> lock(config_lock);
  config_set_uint64(config.get(), section, key, value);

  return true;
}

bool btif_config_get_str(const std::string& section, const std::string& key,
                         char* value, int* size_bytes) {
  CHECK(config != NULL);
  CHECK(value != NULL);
  CHECK(size_bytes != NULL);

  {
    std::unique_lock<std::mutex> lock(config_lock);
    const std::string* stored_value =
        config_get_string(*config, section, key, NULL);
    if (!stored_value) return false;
    strlcpy(value, stored_value->c_str(), *size_bytes);
  }

  *size_bytes = strlen(value) + 1;
  return true;
}

bool btif_config_set_str(const std::string& section, const std::string& key,
                         const std::string& value) {
  CHECK(config != NULL);

  std::unique_lock<std::mutex> lock(config_lock);
  config_set_string(config.get(), section, key, value);
  return true;
}

bool btif_config_get_bin(const std::string& section, const std::string& key,
                         uint8_t* value, size_t* length) {
  CHECK(config != NULL);
  CHECK(value != NULL);
  CHECK(length != NULL);

  std::unique_lock<std::mutex> lock(config_lock);
  const std::string* value_str = config_get_string(*config, section, key, NULL);

  if (!value_str) return false;

  size_t value_len = value_str->length();
  if ((value_len % 2) != 0 || *length < (value_len / 2)) return false;

  for (size_t i = 0; i < value_len; ++i)
    if (!isxdigit(value_str->c_str()[i])) return false;

  const char* ptr = value_str->c_str();
  for (*length = 0; *ptr; ptr += 2, *length += 1)
    sscanf(ptr, "%02hhx", &value[*length]);

  return true;
}

size_t btif_config_get_bin_length(const std::string& section,
                                  const std::string& key) {
  CHECK(config != NULL);

  std::unique_lock<std::mutex> lock(config_lock);
  const std::string* value_str = config_get_string(*config, section, key, NULL);
  if (!value_str) return 0;

  size_t value_len = value_str->length();
  return ((value_len % 2) != 0) ? 0 : (value_len / 2);
}

bool btif_config_set_bin(const std::string& section, const std::string& key,
                         const uint8_t* value, size_t length) {
  const char* lookup = "0123456789abcdef";

  CHECK(config != NULL);

  if (length > 0) CHECK(value != NULL);

  char* str = (char*)osi_calloc(length * 2 + 1);

  for (size_t i = 0; i < length; ++i) {
    str[(i * 2) + 0] = lookup[(value[i] >> 4) & 0x0F];
    str[(i * 2) + 1] = lookup[value[i] & 0x0F];
  }

  {
    std::unique_lock<std::mutex> lock(config_lock);
    config_set_string(config.get(), section, key, str);
  }

  osi_free(str);
  return true;
}

std::list<section_t>& btif_config_sections() { return config->sections; }

bool btif_config_remove(const std::string& section, const std::string& key) {
  CHECK(config != NULL);

  std::unique_lock<std::mutex> lock(config_lock);
  return config_remove_key(config.get(), section, key);
}

void btif_config_save(void) {
  CHECK(config != NULL);
  CHECK(config_timer != NULL);

  alarm_set(config_timer, CONFIG_SETTLE_PERIOD_MS, timer_config_save_cb, NULL);
}

void btif_config_flush(void) {
  CHECK(config != NULL);
  CHECK(config_timer != NULL);

  alarm_cancel(config_timer);
  btif_config_write(0, NULL);
}

bool btif_config_clear(void) {
  CHECK(config != NULL);
  CHECK(config_timer != NULL);

  alarm_cancel(config_timer);

  std::unique_lock<std::mutex> lock(config_lock);

  config = config_new_empty();

  bool ret = config_save(*config, CONFIG_FILE_PATH);
  btif_config_source = RESET;
  return ret;
}

static void timer_config_save_cb(UNUSED_ATTR void* data) {
  // Moving file I/O to btif context instead of timer callback because
  // it usually takes a lot of time to be completed, introducing
  // delays during A2DP playback causing blips or choppiness.
  btif_transfer_context(btif_config_write, 0, NULL, 0, NULL);
}

static void btif_config_write(UNUSED_ATTR uint16_t event,
                              UNUSED_ATTR char* p_param) {
  CHECK(config != NULL);
  CHECK(config_timer != NULL);

  std::unique_lock<std::mutex> lock(config_lock);
  rename(CONFIG_FILE_PATH, CONFIG_BACKUP_PATH);
  std::unique_ptr<config_t> config_paired = config_new_clone(*config);
  btif_config_remove_unpaired(config_paired.get());
  config_save(*config_paired, CONFIG_FILE_PATH);
}

static void btif_config_remove_unpaired(config_t* conf) {
  CHECK(conf != NULL);
  int paired_devices = 0;

  // The paired config used to carry information about
  // discovered devices during regular inquiry scans.
  // We remove these now and cache them in memory instead.
  for (auto it = conf->sections.begin(); it != conf->sections.end();) {
    std::string& section = it->name;
    if (RawAddress::IsValidAddress(section)) {
      // TODO: config_has_key loop thorugh all data, maybe just make it so we
      // loop just once ?
      if (!config_has_key(*conf, section, "LinkKey") &&
          !config_has_key(*conf, section, "LE_KEY_PENC") &&
          !config_has_key(*conf, section, "LE_KEY_PID") &&
          !config_has_key(*conf, section, "LE_KEY_PCSRK") &&
          !config_has_key(*conf, section, "LE_KEY_LENC") &&
          !config_has_key(*conf, section, "LE_KEY_LCSRK")) {
        it = conf->sections.erase(it);
        continue;
      }
      paired_devices++;
    }
    it++;
  }

  // should only happen once, at initial load time
  if (btif_config_devices_loaded == -1)
    btif_config_devices_loaded = paired_devices;
}

void btif_debug_config_dump(int fd) {
  dprintf(fd, "\nBluetooth Config:\n");

  dprintf(fd, "  Config Source: ");
  switch (btif_config_source) {
    case NOT_LOADED:
      dprintf(fd, "Not loaded\n");
      break;
    case ORIGINAL:
      dprintf(fd, "Original file\n");
      break;
    case BACKUP:
      dprintf(fd, "Backup file\n");
      break;
    case LEGACY:
      dprintf(fd, "Legacy file\n");
      break;
    case NEW_FILE:
      dprintf(fd, "New file\n");
      break;
    case RESET:
      dprintf(fd, "Reset file\n");
      break;
  }

  std::string original = "Original";
  dprintf(fd, "  Devices loaded: %d\n", btif_config_devices_loaded);
  dprintf(fd, "  File created/tagged: %s\n", btif_config_time_created);
  dprintf(fd, "  File source: %s\n",
          config_get_string(*config, INFO_SECTION, FILE_SOURCE, &original)
              ->c_str());
}

static void btif_config_remove_restricted(config_t* config) {
  CHECK(config != NULL);

  for (auto it = config->sections.begin(); it != config->sections.end();) {
    const std::string& section = it->name;
    if (RawAddress::IsValidAddress(section) &&
        config_has_key(*config, section, "Restricted")) {
      BTIF_TRACE_DEBUG("%s: Removing restricted device %s", __func__,
                       section.c_str());
      it = config->sections.erase(it);
      continue;
    }
    it++;
  }
}

static bool is_factory_reset(void) {
  char factory_reset[PROPERTY_VALUE_MAX] = {0};
  osi_property_get("persist.bluetooth.factoryreset", factory_reset, "false");
  return strncmp(factory_reset, "true", 4) == 0;
}

static void delete_config_files(void) {
  remove(CONFIG_FILE_PATH);
  remove(CONFIG_BACKUP_PATH);
  osi_property_set("persist.bluetooth.factoryreset", "false");
}

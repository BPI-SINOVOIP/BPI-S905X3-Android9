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

#include <sstream>
#include <stdlib.h>
#include <string.h>
#include <sys/statvfs.h>
#include <sys/xattr.h>

#include <android-base/logging.h>
#include <android-base/stringprintf.h>
#include <cutils/properties.h>
#include <gtest/gtest.h>

#include "InstalldNativeService.h"
#include "dexopt.h"
#include "globals.h"
#include "utils.h"

using android::base::StringPrintf;

namespace android {
namespace installd {

constexpr const char* kTestUuid = "TEST";

static constexpr int FLAG_FORCE = 1 << 16;

int get_property(const char *key, char *value, const char *default_value) {
    return property_get(key, value, default_value);
}

bool calculate_oat_file_path(char path[PKG_PATH_MAX], const char *oat_dir, const char *apk_path,
        const char *instruction_set) {
    return calculate_oat_file_path_default(path, oat_dir, apk_path, instruction_set);
}

bool calculate_odex_file_path(char path[PKG_PATH_MAX], const char *apk_path,
        const char *instruction_set) {
    return calculate_odex_file_path_default(path, apk_path, instruction_set);
}

bool create_cache_path(char path[PKG_PATH_MAX], const char *src, const char *instruction_set) {
    return create_cache_path_default(path, src, instruction_set);
}

static std::string get_full_path(const char* path) {
    return StringPrintf("/data/local/tmp/user/0/%s", path);
}

static void mkdir(const char* path, uid_t owner, gid_t group, mode_t mode) {
    const std::string fullPath = get_full_path(path);
    ::mkdir(fullPath.c_str(), mode);
    ::chown(fullPath.c_str(), owner, group);
    ::chmod(fullPath.c_str(), mode);
}

static void touch(const char* path, uid_t owner, gid_t group, mode_t mode) {
    int fd = ::open(get_full_path(path).c_str(), O_RDWR | O_CREAT, mode);
    ::fchown(fd, owner, group);
    ::fchmod(fd, mode);
    ::close(fd);
}

static int stat_gid(const char* path) {
    struct stat buf;
    ::stat(get_full_path(path).c_str(), &buf);
    return buf.st_gid;
}

static int stat_mode(const char* path) {
    struct stat buf;
    ::stat(get_full_path(path).c_str(), &buf);
    return buf.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO | S_ISGID);
}

class ServiceTest : public testing::Test {
protected:
    InstalldNativeService* service;
    std::unique_ptr<std::string> testUuid;

    virtual void SetUp() {
        setenv("ANDROID_LOG_TAGS", "*:v", 1);
        android::base::InitLogging(nullptr);

        service = new InstalldNativeService();
        testUuid = std::make_unique<std::string>();
        *testUuid = std::string(kTestUuid);
        system("mkdir -p /data/local/tmp/user/0");

        init_globals_from_data_and_root();
    }

    virtual void TearDown() {
        delete service;
        system("rm -rf /data/local/tmp/user");
    }
};

TEST_F(ServiceTest, FixupAppData_Upgrade) {
    LOG(INFO) << "FixupAppData_Upgrade";

    mkdir("com.example", 10000, 10000, 0700);
    mkdir("com.example/normal", 10000, 10000, 0700);
    mkdir("com.example/cache", 10000, 10000, 0700);
    touch("com.example/cache/file", 10000, 10000, 0700);

    service->fixupAppData(testUuid, 0);

    EXPECT_EQ(10000, stat_gid("com.example/normal"));
    EXPECT_EQ(20000, stat_gid("com.example/cache"));
    EXPECT_EQ(20000, stat_gid("com.example/cache/file"));

    EXPECT_EQ(0700, stat_mode("com.example/normal"));
    EXPECT_EQ(02771, stat_mode("com.example/cache"));
    EXPECT_EQ(0700, stat_mode("com.example/cache/file"));
}

TEST_F(ServiceTest, FixupAppData_Moved) {
    LOG(INFO) << "FixupAppData_Moved";

    mkdir("com.example", 10000, 10000, 0700);
    mkdir("com.example/foo", 10000, 10000, 0700);
    touch("com.example/foo/file", 10000, 20000, 0700);
    mkdir("com.example/bar", 10000, 20000, 0700);
    touch("com.example/bar/file", 10000, 20000, 0700);

    service->fixupAppData(testUuid, 0);

    EXPECT_EQ(10000, stat_gid("com.example/foo"));
    EXPECT_EQ(20000, stat_gid("com.example/foo/file"));
    EXPECT_EQ(10000, stat_gid("com.example/bar"));
    EXPECT_EQ(10000, stat_gid("com.example/bar/file"));

    service->fixupAppData(testUuid, FLAG_FORCE);

    EXPECT_EQ(10000, stat_gid("com.example/foo"));
    EXPECT_EQ(10000, stat_gid("com.example/foo/file"));
    EXPECT_EQ(10000, stat_gid("com.example/bar"));
    EXPECT_EQ(10000, stat_gid("com.example/bar/file"));
}

TEST_F(ServiceTest, HashSecondaryDex) {
    LOG(INFO) << "HashSecondaryDex";

    mkdir("com.example", 10000, 10000, 0700);
    mkdir("com.example/foo", 10000, 10000, 0700);
    touch("com.example/foo/file", 10000, 20000, 0700);

    std::vector<uint8_t> result;
    std::string dexPath = get_full_path("com.example/foo/file");
    EXPECT_TRUE(service->hashSecondaryDexFile(
        dexPath, "com.example", 10000, testUuid, FLAG_STORAGE_CE, &result).isOk());

    EXPECT_EQ(result.size(), 32U);

    std::ostringstream output;
    output << std::hex << std::setfill('0');
    for (auto b : result) {
        output << std::setw(2) << +b;
    }

    // This is the SHA256 of an empty string (sha256sum /dev/null)
    EXPECT_EQ(output.str(), "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
}

TEST_F(ServiceTest, HashSecondaryDex_NoSuch) {
    LOG(INFO) << "HashSecondaryDex_NoSuch";

    std::vector<uint8_t> result;
    std::string dexPath = get_full_path("com.example/foo/file");
    EXPECT_TRUE(service->hashSecondaryDexFile(
        dexPath, "com.example", 10000, testUuid, FLAG_STORAGE_CE, &result).isOk());

    EXPECT_EQ(result.size(), 0U);
}

TEST_F(ServiceTest, HashSecondaryDex_Unreadable) {
    LOG(INFO) << "HashSecondaryDex_Unreadable";

    mkdir("com.example", 10000, 10000, 0700);
    mkdir("com.example/foo", 10000, 10000, 0700);
    touch("com.example/foo/file", 10000, 20000, 0300);

    std::vector<uint8_t> result;
    std::string dexPath = get_full_path("com.example/foo/file");
    EXPECT_TRUE(service->hashSecondaryDexFile(
        dexPath, "com.example", 10000, testUuid, FLAG_STORAGE_CE, &result).isOk());

    EXPECT_EQ(result.size(), 0U);
}

TEST_F(ServiceTest, HashSecondaryDex_WrongApp) {
    LOG(INFO) << "HashSecondaryDex_WrongApp";

    mkdir("com.example", 10000, 10000, 0700);
    mkdir("com.example/foo", 10000, 10000, 0700);
    touch("com.example/foo/file", 10000, 20000, 0700);

    std::vector<uint8_t> result;
    std::string dexPath = get_full_path("com.example/foo/file");
    EXPECT_FALSE(service->hashSecondaryDexFile(
        dexPath, "com.wrong", 10000, testUuid, FLAG_STORAGE_CE, &result).isOk());
}

TEST_F(ServiceTest, CalculateOat) {
    char buf[PKG_PATH_MAX];

    EXPECT_TRUE(calculate_oat_file_path(buf, "/path/to/oat", "/path/to/file.apk", "isa"));
    EXPECT_EQ("/path/to/oat/isa/file.odex", std::string(buf));

    EXPECT_FALSE(calculate_oat_file_path(buf, "/path/to/oat", "/path/to/file", "isa"));
    EXPECT_FALSE(calculate_oat_file_path(buf, "/path/to/oat", "file", "isa"));
}

TEST_F(ServiceTest, CalculateOdex) {
    char buf[PKG_PATH_MAX];

    EXPECT_TRUE(calculate_odex_file_path(buf, "/path/to/file.apk", "isa"));
    EXPECT_EQ("/path/to/oat/isa/file.odex", std::string(buf));
}

TEST_F(ServiceTest, CalculateCache) {
    char buf[PKG_PATH_MAX];

    EXPECT_TRUE(create_cache_path(buf, "/path/to/file.apk", "isa"));
    EXPECT_EQ("/data/dalvik-cache/isa/path@to@file.apk@classes.dex", std::string(buf));
}

}  // namespace installd
}  // namespace android

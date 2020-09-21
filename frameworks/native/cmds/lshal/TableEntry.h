/*
 * Copyright (C) 2016 The Android Open Source Project
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

#ifndef FRAMEWORK_NATIVE_CMDS_LSHAL_TABLE_ENTRY_H_
#define FRAMEWORK_NATIVE_CMDS_LSHAL_TABLE_ENTRY_H_

#include <stdint.h>

#include <string>
#include <vector>
#include <iostream>

#include <procpartition/procpartition.h>

#include "TextTable.h"

namespace android {
namespace lshal {

using android::procpartition::Partition;
using Pids = std::vector<int32_t>;

enum : unsigned int {
    HWSERVICEMANAGER_LIST, // through defaultServiceManager()->list()
    PTSERVICEMANAGER_REG_CLIENT, // through registerPassthroughClient
    LIST_DLLIB, // through listing dynamic libraries
};
using TableEntrySource = unsigned int;

enum : unsigned int {
    ARCH_UNKNOWN = 0,
    ARCH32       = 1 << 0,
    ARCH64       = 1 << 1,
    ARCH_BOTH    = ARCH32 | ARCH64
};
using Architecture = unsigned int;

enum class TableColumnType : unsigned int {
    INTERFACE_NAME,
    TRANSPORT,
    SERVER_PID,
    SERVER_CMD,
    SERVER_ADDR,
    CLIENT_PIDS,
    CLIENT_CMDS,
    ARCH,
    THREADS,
    RELEASED,
    HASH,
};

enum {
    NO_PID = -1,
    NO_PTR = 0
};

struct TableEntry {
    std::string interfaceName{};
    std::string transport{};
    int32_t serverPid{NO_PID};
    uint32_t threadUsage{0};
    uint32_t threadCount{0};
    std::string serverCmdline{};
    uint64_t serverObjectAddress{NO_PTR};
    Pids clientPids{};
    std::vector<std::string> clientCmdlines{};
    Architecture arch{ARCH_UNKNOWN};
    // empty: unknown, all zeros: unreleased, otherwise: released
    std::string hash{};
    Partition partition{Partition::UNKNOWN};

    static bool sortByInterfaceName(const TableEntry &a, const TableEntry &b) {
        return a.interfaceName < b.interfaceName;
    };
    static bool sortByServerPid(const TableEntry &a, const TableEntry &b) {
        return a.serverPid < b.serverPid;
    };

    std::string getThreadUsage() const {
        if (threadCount == 0) {
            return "N/A";
        }

        return std::to_string(threadUsage) + "/" + std::to_string(threadCount);
    }

    std::string isReleased() const;

    std::string getField(TableColumnType type) const;

    bool operator==(const TableEntry& other) const;
    std::string to_string() const;
};

using SelectedColumns = std::vector<TableColumnType>;

class Table {
public:
    using Entries = std::vector<TableEntry>;

    Entries::iterator begin() { return mEntries.begin(); }
    Entries::const_iterator begin() const { return mEntries.begin(); }
    Entries::iterator end() { return mEntries.end(); }
    Entries::const_iterator end() const { return mEntries.end(); }
    size_t size() const { return mEntries.size(); }

    void add(TableEntry&& entry) { mEntries.push_back(std::move(entry)); }

    void setSelectedColumns(const SelectedColumns& s) { mSelectedColumns = s; }
    const SelectedColumns& getSelectedColumns() const { return mSelectedColumns; }

    void setDescription(std::string&& d) { mDescription = std::move(d); }

    // Write table content.
    TextTable createTextTable(bool neat = true,
        const std::function<std::string(const std::string&)>& emitDebugInfo = nullptr) const;

private:
    std::string mDescription;
    Entries mEntries;
    SelectedColumns mSelectedColumns;
};

using TableEntryCompare = std::function<bool(const TableEntry &, const TableEntry &)>;

class MergedTable {
public:
    MergedTable(std::vector<const Table*>&& tables) : mTables(std::move(tables)) {}
    TextTable createTextTable();
private:
    std::vector<const Table*> mTables;
};

}  // namespace lshal
}  // namespace android

#endif  // FRAMEWORK_NATIVE_CMDS_LSHAL_TABLE_ENTRY_H_

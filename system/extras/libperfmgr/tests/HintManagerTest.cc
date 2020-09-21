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
 * See the License for the specic language governing permissions and
 * limitations under the License.
 */

#include <algorithm>
#include <thread>

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/test_utils.h>

#include <gtest/gtest.h>

#include "perfmgr/HintManager.h"

namespace android {
namespace perfmgr {

using namespace std::chrono_literals;

constexpr auto kSLEEP_TOLERANCE_MS = 50ms;

// JSON_CONFIG
// {
//     "Nodes": [
//         {
//             "Name": "CPUCluster0MinFreq",
//             "Path": "/sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq",
//             "Values": [
//                 "1512000",
//                 "1134000",
//                 "384000"
//             ],
//             "DefaultIndex": 2,
//             "ResetOnInit": true
//         },
//         {
//             "Name": "CPUCluster1MinFreq",
//             "Path": "/sys/devices/system/cpu/cpu4/cpufreq/scaling_min_freq",
//             "Values": [
//                 "1512000",
//                 "1134000",
//                 "384000"
//             ],
//             "HoldFd": true
//         }
//     ],
//     "Actions": [
//         {
//             "PowerHint": "INTERACTION",
//             "Node": "CPUCluster1MinFreq",
//             "Value": "1134000",
//             "Duration": 800
//         },
//         {
//             "PowerHint": "LAUNCH",
//             "Node": "CPUCluster0MinFreq",
//             "Value": "1134000",
//             "Duration": 500
//         },
//         {
//             "PowerHint": "LAUNCH",
//             "Node": "CPUCluster1MinFreq",
//             "Value": "1512000",
//             "Duration": 2000
//         }
//     ]
// }
constexpr char kJSON_RAW[] =
    "{\"Nodes\":[{\"Name\":\"CPUCluster0MinFreq\",\"Path\":\"/sys/devices/"
    "system/cpu/cpu0/cpufreq/"
    "scaling_min_freq\",\"Values\":[\"1512000\",\"1134000\",\"384000\"],"
    "\"DefaultIndex\":2,\"ResetOnInit\":true},{\"Name\":\"CPUCluster1MinFreq\","
    "\"Path\":\"/sys/devices/system/cpu/cpu4/cpufreq/"
    "scaling_min_freq\",\"Values\":[\"1512000\",\"1134000\",\"384000\"],"
    "\"HoldFd\":true}],\"Actions\":[{\"PowerHint\":\"INTERACTION\",\"Node\":"
    "\"CPUCluster1MinFreq\",\"Value\":\"1134000\",\"Duration\":800},{"
    "\"PowerHint\":"
    "\"LAUNCH\",\"Node\":\"CPUCluster0MinFreq\",\"Value\":\"1134000\","
    "\"Duration\":"
    "500},{\"PowerHint\":\"LAUNCH\",\"Node\":\"CPUCluster1MinFreq\","
    "\"Value\":\"1512000\",\"Duration\":2000}]}";

class HintManagerTest : public ::testing::Test, public HintManager {
  protected:
    HintManagerTest()
        : HintManager(nullptr,
                      std::map<std::string, std::vector<NodeAction>>{}) {
        android::base::SetMinimumLogSeverity(android::base::VERBOSE);
    }

    virtual void SetUp() {
        // Set up dummy nodes
        std::unique_ptr<TemporaryFile> tf = std::make_unique<TemporaryFile>();
        nodes_.emplace_back(
            new Node("n0", tf->path,
                     {{"n0_value0"}, {"n0_value1"}, {"n0_value2"}}, 2, false));
        files_.emplace_back(std::move(tf));
        tf = std::make_unique<TemporaryFile>();
        nodes_.emplace_back(
            new Node("n1", tf->path,
                     {{"n1_value0"}, {"n1_value1"}, {"n1_value2"}}, 2, true));
        files_.emplace_back(std::move(tf));
        nm_ = new NodeLooperThread(std::move(nodes_));
        // Set up dummy actions
        // "INTERACTION"
        // Node0, value1, 800ms
        // Node1, value1, forever
        // "LAUNCH"
        // Node0, value0, forever
        // Node1, value0, 400ms
        actions_ = std::map<std::string, std::vector<NodeAction>>{
            {"INTERACTION", {{0, 1, 800ms}, {1, 1, 0ms}}},
            {"LAUNCH", {{0, 0, 0ms}, {1, 0, 400ms}}}};

        // Prepare dummy files to replace the nodes' path in example json_doc
        files_.emplace_back(std::make_unique<TemporaryFile>());
        files_.emplace_back(std::make_unique<TemporaryFile>());
        // replace filepath
        json_doc_ = kJSON_RAW;
        std::string from =
            "/sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq";
        size_t start_pos = json_doc_.find(from);
        json_doc_.replace(start_pos, from.length(), files_[0 + 2]->path);
        from = "/sys/devices/system/cpu/cpu4/cpufreq/scaling_min_freq";
        start_pos = json_doc_.find(from);
        json_doc_.replace(start_pos, from.length(), files_[1 + 2]->path);
    }

    virtual void TearDown() {
        actions_.clear();
        nodes_.clear();
        files_.clear();
        nm_ = nullptr;
    }
    sp<NodeLooperThread> nm_;
    std::map<std::string, std::vector<NodeAction>> actions_;
    std::vector<std::unique_ptr<Node>> nodes_;
    std::vector<std::unique_ptr<TemporaryFile>> files_;
    std::string json_doc_;
};

static inline void _VerifyPathValue(const std::string& path,
                                    const std::string& value) {
    std::string s;
    EXPECT_TRUE(android::base::ReadFileToString(path, &s)) << strerror(errno);
    EXPECT_EQ(value, s);
}

// Test GetHints
TEST_F(HintManagerTest, GetHintsTest) {
    HintManager hm(nm_, actions_);
    std::vector<std::string> hints = hm.GetHints();
    EXPECT_TRUE(hm.IsRunning());
    EXPECT_EQ(2u, hints.size());
    EXPECT_NE(std::find(hints.begin(), hints.end(), "INTERACTION"), hints.end());
    EXPECT_NE(std::find(hints.begin(), hints.end(), "LAUNCH"), hints.end());
}

// Test initialization of default values
TEST_F(HintManagerTest, HintInitDefaultTest) {
    HintManager hm(nm_, actions_);
    std::this_thread::sleep_for(kSLEEP_TOLERANCE_MS);
    EXPECT_TRUE(hm.IsRunning());
    _VerifyPathValue(files_[0]->path, "");
    _VerifyPathValue(files_[1]->path, "n1_value2");
}

// Test hint/cancel/expire
TEST_F(HintManagerTest, HintTest) {
    HintManager hm(nm_, actions_);
    EXPECT_TRUE(hm.IsRunning());
    EXPECT_TRUE(hm.DoHint("INTERACTION"));
    std::this_thread::sleep_for(kSLEEP_TOLERANCE_MS);
    _VerifyPathValue(files_[0]->path, "n0_value1");
    _VerifyPathValue(files_[1]->path, "n1_value1");
    // this won't change the expire time of INTERACTION hint
    EXPECT_TRUE(hm.DoHint("INTERACTION", 200ms));
    // now place new hint
    EXPECT_TRUE(hm.DoHint("LAUNCH"));
    std::this_thread::sleep_for(kSLEEP_TOLERANCE_MS);
    _VerifyPathValue(files_[0]->path, "n0_value0");
    _VerifyPathValue(files_[1]->path, "n1_value0");
    EXPECT_TRUE(hm.DoHint("LAUNCH", 500ms));
    // no"LAUNCH" node1 not expired
    std::this_thread::sleep_for(400ms);
    _VerifyPathValue(files_[0]->path, "n0_value0");
    _VerifyPathValue(files_[1]->path, "n1_value0");
    // "LAUNCH" node1 expired
    std::this_thread::sleep_for(100ms + kSLEEP_TOLERANCE_MS);
    _VerifyPathValue(files_[0]->path, "n0_value0");
    _VerifyPathValue(files_[1]->path, "n1_value1");
    EXPECT_TRUE(hm.EndHint("LAUNCH"));
    std::this_thread::sleep_for(kSLEEP_TOLERANCE_MS);
    // "LAUNCH" canceled
    _VerifyPathValue(files_[0]->path, "n0_value1");
    _VerifyPathValue(files_[1]->path, "n1_value1");
    std::this_thread::sleep_for(200ms);
    // "INTERACTION" node0 expired
    _VerifyPathValue(files_[0]->path, "n0_value2");
    _VerifyPathValue(files_[1]->path, "n1_value1");
    EXPECT_TRUE(hm.EndHint("INTERACTION"));
    std::this_thread::sleep_for(kSLEEP_TOLERANCE_MS);
    // "INTERACTION" canceled
    _VerifyPathValue(files_[0]->path, "n0_value2");
    _VerifyPathValue(files_[1]->path, "n1_value2");
}

// Test parsing nodes with duplicate name
TEST_F(HintManagerTest, ParseNodesTest) {
    std::vector<std::unique_ptr<Node>> nodes =
        HintManager::ParseNodes(json_doc_);
    EXPECT_EQ(2u, nodes.size());
    EXPECT_EQ("CPUCluster0MinFreq", nodes[0]->GetName());
    EXPECT_EQ("CPUCluster1MinFreq", nodes[1]->GetName());
    EXPECT_EQ(files_[0 + 2]->path, nodes[0]->GetPath());
    EXPECT_EQ(files_[1 + 2]->path, nodes[1]->GetPath());
    EXPECT_EQ("1512000", nodes[0]->GetValues()[0]);
    EXPECT_EQ("1134000", nodes[0]->GetValues()[1]);
    EXPECT_EQ("384000", nodes[0]->GetValues()[2]);
    EXPECT_EQ("1512000", nodes[1]->GetValues()[0]);
    EXPECT_EQ("1134000", nodes[1]->GetValues()[1]);
    EXPECT_EQ("384000", nodes[1]->GetValues()[2]);
    EXPECT_EQ(2u, nodes[0]->GetDefaultIndex());
    EXPECT_EQ(2u, nodes[1]->GetDefaultIndex());
    EXPECT_TRUE(nodes[0]->GetResetOnInit());
    EXPECT_FALSE(nodes[1]->GetResetOnInit());
    EXPECT_FALSE(nodes[0]->GetHoldFd());
    EXPECT_TRUE(nodes[1]->GetHoldFd());
}

// Test parsing actions
TEST_F(HintManagerTest, ParseNodesDuplicateNameTest) {
    std::string from = "CPUCluster0MinFreq";
    size_t start_pos = json_doc_.find(from);
    json_doc_.replace(start_pos, from.length(), "CPUCluster1MinFreq");
    std::vector<std::unique_ptr<Node>> nodes =
        HintManager::ParseNodes(json_doc_);
    EXPECT_EQ(0u, nodes.size());
}

// Test parsing nodes with duplicate path
TEST_F(HintManagerTest, ParseNodesDuplicatePathTest) {
    std::string from = files_[0 + 2]->path;
    size_t start_pos = json_doc_.find(from);
    json_doc_.replace(start_pos, from.length(), files_[1 + 2]->path);
    std::vector<std::unique_ptr<Node>> nodes =
        HintManager::ParseNodes(json_doc_);
    EXPECT_EQ(0u, nodes.size());
}

// Test parsing invalid json for nodes
TEST_F(HintManagerTest, ParseBadNodesTest) {
    std::vector<std::unique_ptr<Node>> nodes =
        HintManager::ParseNodes("invalid json");
    EXPECT_EQ(0u, nodes.size());
    nodes = HintManager::ParseNodes(
        "{\"devices\":{\"15\":[\"armeabi-v7a\"],\"16\":[\"armeabi-v7a\"],"
        "\"26\":[\"armeabi-v7a\",\"arm64-v8a\",\"x86\",\"x86_64\"]}}");
    EXPECT_EQ(0u, nodes.size());
}

// Test parsing actions
TEST_F(HintManagerTest, ParseActionsTest) {
    std::vector<std::unique_ptr<Node>> nodes =
        HintManager::ParseNodes(json_doc_);
    std::map<std::string, std::vector<NodeAction>> actions =
        HintManager::ParseActions(json_doc_, nodes);
    EXPECT_EQ(2u, actions.size());
    EXPECT_EQ(1u, actions["INTERACTION"].size());

    EXPECT_EQ(1u, actions["INTERACTION"][0].node_index);
    EXPECT_EQ(1u, actions["INTERACTION"][0].value_index);
    EXPECT_EQ(std::chrono::milliseconds(800).count(),
              actions["INTERACTION"][0].timeout_ms.count());

    EXPECT_EQ(2u, actions["LAUNCH"].size());

    EXPECT_EQ(0u, actions["LAUNCH"][0].node_index);
    EXPECT_EQ(1u, actions["LAUNCH"][0].value_index);
    EXPECT_EQ(std::chrono::milliseconds(500).count(),
              actions["LAUNCH"][0].timeout_ms.count());

    EXPECT_EQ(1u, actions["LAUNCH"][1].node_index);
    EXPECT_EQ(0u, actions["LAUNCH"][1].value_index);
    EXPECT_EQ(std::chrono::milliseconds(2000).count(),
              actions["LAUNCH"][1].timeout_ms.count());
}

// Test parsing actions with duplicate node
TEST_F(HintManagerTest, ParseActionDuplicateNodeTest) {
    std::string from = "\"Node\":\"CPUCluster0MinFreq\"";
    size_t start_pos = json_doc_.find(from);
    json_doc_.replace(start_pos, from.length(),
                      "\"Node\": \"CPUCluster1MinFreq\"");
    std::vector<std::unique_ptr<Node>> nodes =
        HintManager::ParseNodes(json_doc_);
    EXPECT_EQ(2u, nodes.size());
    std::map<std::string, std::vector<NodeAction>> actions =
        HintManager::ParseActions(json_doc_, nodes);
    EXPECT_EQ(0u, actions.size());
}

// Test parsing invalid json for actions
TEST_F(HintManagerTest, ParseBadActionsTest) {
    std::vector<std::unique_ptr<Node>> nodes =
        HintManager::ParseNodes(json_doc_);
    std::map<std::string, std::vector<NodeAction>> actions =
        HintManager::ParseActions("invalid json", nodes);
    EXPECT_EQ(0u, actions.size());
    actions = HintManager::ParseActions(
        "{\"devices\":{\"15\":[\"armeabi-v7a\"],\"16\":[\"armeabi-v7a\"],"
        "\"26\":[\"armeabi-v7a\",\"arm64-v8a\",\"x86\",\"x86_64\"]}}",
        nodes);
    EXPECT_EQ(0u, actions.size());
}

// Test hint/cancel/expire with json config
TEST_F(HintManagerTest, GetFromJSONTest) {
    TemporaryFile json_file;
    ASSERT_TRUE(android::base::WriteStringToFile(json_doc_, json_file.path))
        << strerror(errno);
    std::unique_ptr<HintManager> hm = HintManager::GetFromJSON(json_file.path);
    EXPECT_NE(nullptr, hm.get());
    std::this_thread::sleep_for(kSLEEP_TOLERANCE_MS);
    EXPECT_TRUE(hm->IsRunning());
    // Initial default value on Node0
    _VerifyPathValue(files_[0 + 2]->path, "384000");
    _VerifyPathValue(files_[1 + 2]->path, "");
    // Do INTERACTION
    EXPECT_TRUE(hm->DoHint("INTERACTION"));
    std::this_thread::sleep_for(kSLEEP_TOLERANCE_MS);
    _VerifyPathValue(files_[0 + 2]->path, "384000");
    _VerifyPathValue(files_[1 + 2]->path, "1134000");
    // Do LAUNCH
    EXPECT_TRUE(hm->DoHint("LAUNCH"));
    std::this_thread::sleep_for(kSLEEP_TOLERANCE_MS);
    _VerifyPathValue(files_[0 + 2]->path, "1134000");
    _VerifyPathValue(files_[1 + 2]->path, "1512000");
    std::this_thread::sleep_for(500ms);
    // "LAUNCH" node0 expired
    _VerifyPathValue(files_[0 + 2]->path, "384000");
    _VerifyPathValue(files_[1 + 2]->path, "1512000");
    EXPECT_TRUE(hm->EndHint("LAUNCH"));
    std::this_thread::sleep_for(kSLEEP_TOLERANCE_MS);
    // "LAUNCH" canceled
    _VerifyPathValue(files_[0 + 2]->path, "384000");
    _VerifyPathValue(files_[1 + 2]->path, "1134000");
    std::this_thread::sleep_for(300ms);
    // "INTERACTION" node1 expired
    _VerifyPathValue(files_[0 + 2]->path, "384000");
    _VerifyPathValue(files_[1 + 2]->path, "384000");
}

}  // namespace perfmgr
}  // namespace android

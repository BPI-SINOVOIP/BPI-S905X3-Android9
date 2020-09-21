/******************************************************************************
 *
 *  Copyright 2016 Google, Inc.
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

#include <gtest/gtest.h>
#include <string>
#include <vector>
using std::vector;

#include "bt_address.h"

namespace {
const std::string kTestAddr1 = "12:34:56:78:9a:bc";
const std::string kTestAddr2 = "cb:a9:87:65:43:21";
const std::string kTestAddr3 = "cb:a9:56:78:9a:bc";
const std::string kUpperMask = "ff:ff:00:00:00:00";
const std::string kLowerMask = "00:00:ff:ff:ff:ff";
const std::string kZeros = "00:00:00:00:00:00";
const vector<uint8_t> kZeros_octets = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
const vector<uint8_t> kTestAddr1_octets = {0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12};
}  // namespace

namespace test_vendor_lib {

class BtAddressTest : public ::testing::Test {
 public:
  BtAddressTest() {}
  ~BtAddressTest() {}
};

TEST_F(BtAddressTest, IsValid) {
  EXPECT_FALSE(BtAddress::IsValid(""));
  EXPECT_FALSE(BtAddress::IsValid("000000000000"));
  EXPECT_FALSE(BtAddress::IsValid("00:00:00:00:0000"));
  EXPECT_FALSE(BtAddress::IsValid("00:00:00:00:00:0"));
  EXPECT_FALSE(BtAddress::IsValid("00:00:00:00:00:0;"));
  EXPECT_FALSE(BtAddress::IsValid("00:00:000:00:00:0;"));
  EXPECT_TRUE(BtAddress::IsValid("00:00:00:00:00:00"));
  EXPECT_FALSE(BtAddress::IsValid("aB:cD:eF:Gh:iJ:Kl"));
  EXPECT_TRUE(BtAddress::IsValid(kTestAddr1));
  EXPECT_TRUE(BtAddress::IsValid(kTestAddr2));
  EXPECT_TRUE(BtAddress::IsValid(kTestAddr3));
}

TEST_F(BtAddressTest, test_comparisons) {
  BtAddress btaddr1;
  BtAddress btaddr1_copy;
  BtAddress btaddr2;

  EXPECT_TRUE(btaddr1.FromString(kTestAddr1));
  EXPECT_TRUE(btaddr1_copy.FromString(kTestAddr1));
  EXPECT_TRUE(btaddr2.FromString(kTestAddr2));

  EXPECT_TRUE(btaddr1 == btaddr1_copy);
  EXPECT_FALSE(btaddr1 == btaddr2);

  EXPECT_FALSE(btaddr1 != btaddr1_copy);
  EXPECT_TRUE(btaddr1 != btaddr2);

  EXPECT_FALSE(btaddr1 < btaddr1_copy);
  EXPECT_TRUE(btaddr1 < btaddr2);
  EXPECT_FALSE(btaddr2 < btaddr1);

  EXPECT_FALSE(btaddr1 > btaddr1_copy);
  EXPECT_FALSE(btaddr1 > btaddr2);
  EXPECT_TRUE(btaddr2 > btaddr1);

  EXPECT_TRUE(btaddr1 <= btaddr1_copy);
  EXPECT_TRUE(btaddr1 <= btaddr2);
  EXPECT_FALSE(btaddr2 <= btaddr1);

  EXPECT_TRUE(btaddr1 >= btaddr1_copy);
  EXPECT_FALSE(btaddr1 >= btaddr2);
  EXPECT_TRUE(btaddr2 >= btaddr1);
}

TEST_F(BtAddressTest, test_assignment) {
  BtAddress btaddr1;
  BtAddress btaddr2;
  BtAddress btaddr3;

  EXPECT_TRUE(btaddr1.FromString(kTestAddr1));
  EXPECT_TRUE(btaddr2.FromString(kTestAddr2));
  EXPECT_TRUE(btaddr3.FromString(kTestAddr3));

  EXPECT_TRUE(btaddr1 != btaddr2);
  EXPECT_TRUE(btaddr2 != btaddr3);
  EXPECT_TRUE(btaddr1 != btaddr3);

  btaddr1 = btaddr2;
  EXPECT_TRUE(btaddr1 == btaddr2);
  EXPECT_TRUE(btaddr2 != btaddr3);
  EXPECT_TRUE(btaddr1 != btaddr3);

  btaddr3 = btaddr2;
  EXPECT_TRUE(btaddr1 == btaddr2);
  EXPECT_TRUE(btaddr2 == btaddr3);
  EXPECT_TRUE(btaddr1 == btaddr3);
}

TEST_F(BtAddressTest, test_bitoperations) {
  BtAddress btaddr1;
  BtAddress btaddr2;
  BtAddress btaddr3;
  BtAddress btaddr_lowmask;
  BtAddress btaddr_upmask;

  EXPECT_TRUE(btaddr1.FromString(kTestAddr1));
  EXPECT_TRUE(btaddr2.FromString(kTestAddr2));
  EXPECT_TRUE(btaddr3.FromString(kTestAddr3));
  EXPECT_TRUE(btaddr_lowmask.FromString(kLowerMask));
  EXPECT_TRUE(btaddr_upmask.FromString(kUpperMask));

  EXPECT_TRUE(btaddr1 != btaddr2);
  EXPECT_TRUE(btaddr2 != btaddr3);
  btaddr1 &= btaddr_lowmask;
  btaddr2 &= btaddr_upmask;
  btaddr1 |= btaddr2;
  EXPECT_TRUE(btaddr1 == btaddr3);
}

TEST_F(BtAddressTest, FromString) {
  BtAddress btaddrA;
  BtAddress btaddrB;
  BtAddress btaddrC;
  EXPECT_TRUE(btaddrA.FromString(kTestAddr1));
  EXPECT_TRUE(btaddrA != btaddrB);
  EXPECT_TRUE(btaddrC == btaddrB);
  EXPECT_TRUE(btaddrB.FromString(kTestAddr1));
  EXPECT_TRUE(btaddrC.FromString(kTestAddr1));
  EXPECT_TRUE(btaddrA == btaddrB);
  EXPECT_TRUE(btaddrC == btaddrB);
}

TEST_F(BtAddressTest, FromVector) {
  BtAddress btaddr1;
  EXPECT_TRUE(btaddr1.FromString(kTestAddr1));
  BtAddress btaddrA;
  BtAddress btaddrB;
  EXPECT_TRUE(btaddrA.FromVector(kTestAddr1_octets));
  EXPECT_TRUE(btaddrA != btaddrB);
  EXPECT_TRUE(btaddrB.FromVector(kTestAddr1_octets));
  EXPECT_TRUE(btaddrA == btaddrB);
  EXPECT_TRUE(btaddr1 == btaddrB);
}

TEST_F(BtAddressTest, ToVector) {
  BtAddress btaddr1;
  BtAddress btaddr1_copy;
  BtAddress btaddr2;
  vector<uint8_t> octets1;
  vector<uint8_t> octets1_copy;
  vector<uint8_t> octets2;
  EXPECT_TRUE(btaddr1.FromString(kTestAddr1));
  EXPECT_TRUE(btaddr1_copy.FromString(kTestAddr1));
  EXPECT_TRUE(btaddr2.FromString(kTestAddr2));
  EXPECT_TRUE(btaddr1 == btaddr1_copy);
  EXPECT_TRUE(btaddr1 != btaddr2);
  btaddr1.ToVector(octets1);
  btaddr2.ToVector(octets2);
  btaddr1_copy.ToVector(octets1_copy);
  EXPECT_TRUE(octets1 != octets2);
  EXPECT_TRUE(octets1 == octets1_copy);
  EXPECT_TRUE(octets1.size() == BtAddress::kOctets);
  btaddr1.ToVector(octets1);
  EXPECT_TRUE(octets1.size() == (2 * BtAddress::kOctets));
}

TEST_F(BtAddressTest, ToString) {
  BtAddress btaddr_zeros;
  BtAddress btaddr1;
  EXPECT_TRUE(btaddr_zeros.FromString(kZeros));
  EXPECT_TRUE(btaddr1.FromString(kTestAddr1));
  EXPECT_TRUE(btaddr_zeros.ToString() == kZeros);
  EXPECT_TRUE(btaddr1.ToString() == kTestAddr1);
  EXPECT_TRUE(btaddr_zeros.ToString() != kTestAddr1);
  EXPECT_TRUE(btaddr1.ToString() != kZeros);
}

}  // namespace test_vendor_lib

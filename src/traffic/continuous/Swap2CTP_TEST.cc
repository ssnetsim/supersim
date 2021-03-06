/*
 * Licensed under the Apache License, Version 2.0 (the 'License');
 * you may not use this file except in compliance with the License.
 * See the NOTICE file distributed with this work for additional information
 * regarding copyright ownership. You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "traffic/continuous/Swap2CTP.h"

#include <cassert>

#include "bits/bits.h"
#include "gtest/gtest.h"
#include "nlohmann/json.hpp"
#include "prim/prim.h"
#include "test/TestSetup_TESTLIB.h"

TEST(Swap2CTP, no_enabled_dims) {
  TestSetup test(1, 1, 1, 1, 0xBAADF00D);
  u32 src, dst, numTerminals;
  nlohmann::json settings;
  Swap2CTP* tp;
  std::map<u32, u32> swap1, swap2;

  settings["dimensions"][0] = nlohmann::json(4);
  settings["dimensions"][1] = nlohmann::json(3);
  settings["concentration"] = nlohmann::json(2);
  settings["interface_ports"] = nlohmann::json(1);

  numTerminals = 2 * 3 * 4;
  swap1 = {{0, 4},  {2, 6},   {4, 0},   {6, 2},   {8, 12},  {10, 14},
           {12, 8}, {14, 10}, {16, 20}, {18, 22}, {20, 16}, {22, 18}};
  swap2 = {{1, 17},  {3, 19},  {5, 21}, {7, 23}, {9, 9},  {11, 11},
           {13, 13}, {15, 15}, {17, 1}, {19, 3}, {21, 5}, {23, 7}};

  for (const auto& p : swap1) {
    src = p.first;
    dst = p.second;
    tp = new Swap2CTP("TP", nullptr, numTerminals, src, settings);
    for (u32 idx = 0; idx < 100; ++idx) {
      u32 next = tp->nextDestination();
      ASSERT_LT(next, numTerminals);
      ASSERT_EQ(next, dst);
    }
    delete tp;
  }
  for (const auto& p : swap2) {
    src = p.first;
    dst = p.second;
    tp = new Swap2CTP("TP", nullptr, numTerminals, src, settings);
    for (u32 idx = 0; idx < 100; ++idx) {
      u32 next = tp->nextDestination();
      ASSERT_LT(next, numTerminals);
      ASSERT_EQ(next, dst);
    }
    delete tp;
  }
}

TEST(Swap2CTP, enabled_dims_0_1) {
  TestSetup test(1, 1, 1, 1, 0xBAADF00D);
  u32 src, dst, numTerminals;
  nlohmann::json settings;
  Swap2CTP* tp;
  std::map<u32, u32> swap1, swap2;

  settings["dimensions"][0] = nlohmann::json(4);
  settings["dimensions"][1] = nlohmann::json(3);
  settings["concentration"] = nlohmann::json(2);
  settings["interface_ports"] = nlohmann::json(1);
  settings["enabled_dimensions"][0] = true;
  settings["enabled_dimensions"][1] = true;

  numTerminals = 2 * 3 * 4;
  swap1 = {{0, 4},  {2, 6},   {4, 0},   {6, 2},   {8, 12},  {10, 14},
           {12, 8}, {14, 10}, {16, 20}, {18, 22}, {20, 16}, {22, 18}};
  swap2 = {{1, 17},  {3, 19},  {5, 21}, {7, 23}, {9, 9},  {11, 11},
           {13, 13}, {15, 15}, {17, 1}, {19, 3}, {21, 5}, {23, 7}};

  for (const auto& p : swap1) {
    src = p.first;
    dst = p.second;
    tp = new Swap2CTP("TP", nullptr, numTerminals, src, settings);
    for (u32 idx = 0; idx < 100; ++idx) {
      u32 next = tp->nextDestination();
      ASSERT_LT(next, numTerminals);
      ASSERT_EQ(next, dst);
    }
    delete tp;
  }
  for (const auto& p : swap2) {
    src = p.first;
    dst = p.second;
    tp = new Swap2CTP("TP", nullptr, numTerminals, src, settings);
    for (u32 idx = 0; idx < 100; ++idx) {
      u32 next = tp->nextDestination();
      ASSERT_LT(next, numTerminals);
      ASSERT_EQ(next, dst);
    }
    delete tp;
  }
}

TEST(Swap2CTP, enabled_dims_1_0_3d) {
  TestSetup test(1, 1, 1, 1, 0xBAADF00D);
  u32 src, dst, numTerminals;
  nlohmann::json settings;
  Swap2CTP* tp;
  std::map<u32, u32> swap1, swap2;

  settings["dimensions"][0] = nlohmann::json(4);
  settings["dimensions"][1] = nlohmann::json(3);
  settings["dimensions"][2] = nlohmann::json(3);
  settings["concentration"] = nlohmann::json(2);
  settings["interface_ports"] = nlohmann::json(1);
  settings["enabled_dimensions"][0] = true;
  settings["enabled_dimensions"][1] = true;
  settings["enabled_dimensions"][2] = false;

  numTerminals = 2 * 3 * 4;
  swap1 = {{0, 4},  {2, 6},   {4, 0},   {6, 2},   {8, 12},  {10, 14},
           {12, 8}, {14, 10}, {16, 20}, {18, 22}, {20, 16}, {22, 18}};
  swap2 = {{1, 17},  {3, 19},  {5, 21}, {7, 23}, {9, 9},  {11, 11},
           {13, 13}, {15, 15}, {17, 1}, {19, 3}, {21, 5}, {23, 7}};

  for (const auto& p : swap1) {
    src = p.first;
    dst = p.second;
    tp = new Swap2CTP("TP", nullptr, numTerminals, src, settings);
    for (u32 idx = 0; idx < 100; ++idx) {
      u32 next = tp->nextDestination();
      ASSERT_LT(next, numTerminals);
      ASSERT_EQ(next, dst);
    }
    delete tp;
  }
  for (const auto& p : swap2) {
    src = p.first;
    dst = p.second;
    tp = new Swap2CTP("TP", nullptr, numTerminals, src, settings);
    for (u32 idx = 0; idx < 100; ++idx) {
      u32 next = tp->nextDestination();
      ASSERT_LT(next, numTerminals);
      ASSERT_EQ(next, dst);
    }
    delete tp;
  }
}

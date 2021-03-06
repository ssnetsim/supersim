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
#include "traffic/continuous/DimRotateCTP.h"

#include <cassert>

#include "bits/bits.h"
#include "gtest/gtest.h"
#include "nlohmann/json.hpp"
#include "prim/prim.h"
#include "test/TestSetup_TESTLIB.h"

TEST(DimRotateCTP, right) {
  TestSetup test(1, 1, 1, 1, 0xBAADF00D);
  u32 src, dst, numTerminals;
  nlohmann::json settings;
  DimRotateCTP* tp;
  std::map<u32, u32> pairs;

  settings["dimensions"][0] = nlohmann::json(3);
  settings["dimensions"][1] = nlohmann::json(3);
  settings["dimensions"][2] = nlohmann::json(3);
  settings["concentration"] = nlohmann::json(4);
  settings["interface_ports"] = nlohmann::json(1);
  settings["direction"] = "right";

  numTerminals = 4 * 3 * 3 * 3 * 1;
  pairs = {{0, 0},   {1, 3},   {2, 6},   {3, 9},   {4, 12},  {5, 15},  {6, 18},
           {7, 21},  {8, 24},  {9, 1},   {10, 4},  {11, 7},  {12, 10}, {13, 13},
           {14, 16}, {15, 19}, {16, 22}, {17, 25}, {18, 2},  {19, 5},  {20, 8},
           {21, 11}, {22, 14}, {23, 17}, {24, 20}, {25, 23}, {26, 26}};

  for (u32 iface = 0; iface < 4; ++iface) {
    for (const auto& p : pairs) {
      src = p.first * 4 + iface;
      dst = p.second * 4 + iface;
      tp = new DimRotateCTP("TP", nullptr, numTerminals, src, settings);
      for (u32 idx = 0; idx < 100; ++idx) {
        u32 next = tp->nextDestination();
        ASSERT_LT(next, numTerminals);
        ASSERT_EQ(next, dst);
      }
      delete tp;
    }
  }
}

TEST(DimRotateCTP, left) {
  TestSetup test(1, 1, 1, 1, 0xBAADF00D);
  u32 src, dst, numTerminals;
  nlohmann::json settings;
  DimRotateCTP* tp;
  std::map<u32, u32> pairs;

  settings["dimensions"][0] = nlohmann::json(3);
  settings["dimensions"][1] = nlohmann::json(3);
  settings["dimensions"][2] = nlohmann::json(3);
  settings["concentration"] = nlohmann::json(4);
  settings["interface_ports"] = nlohmann::json(1);
  settings["direction"] = "left";

  numTerminals = 4 * 3 * 3 * 3 * 1;
  pairs = {{0, 0},   {1, 9},   {2, 18},  {3, 1},   {4, 10},  {5, 19},  {6, 2},
           {7, 11},  {8, 20},  {9, 3},   {10, 12}, {11, 21}, {12, 4},  {13, 13},
           {14, 22}, {15, 5},  {16, 14}, {17, 23}, {18, 6},  {19, 15}, {20, 24},
           {21, 7},  {22, 16}, {23, 25}, {24, 8},  {25, 17}, {26, 26}};

  for (u32 iface = 0; iface < 4; ++iface) {
    for (const auto& p : pairs) {
      src = p.first * 4 + iface;
      dst = p.second * 4 + iface;
      tp = new DimRotateCTP("TP", nullptr, numTerminals, src, settings);
      for (u32 idx = 0; idx < 100; ++idx) {
        u32 next = tp->nextDestination();
        ASSERT_LT(next, numTerminals);
        ASSERT_EQ(next, dst);
      }
      delete tp;
    }
  }
}

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
#include "traffic/continuous/TornadoCTP.h"

#include <cassert>

#include "bits/bits.h"
#include "gtest/gtest.h"
#include "nlohmann/json.hpp"
#include "prim/prim.h"
#include "test/TestSetup_TESTLIB.h"

TEST(TornadoCTP, no_dimMask) {
  TestSetup test(1, 1, 1, 1, 0xBAADF00D);
  u32 src, dst, numTerminals;
  nlohmann::json settings;
  TornadoCTP* tp;
  std::map<u32, u32> pairs;

  settings["dimensions"][0] = nlohmann::json(5);
  settings["dimensions"][1] = nlohmann::json(4);
  settings["concentration"] = nlohmann::json(4);
  settings["interface_ports"] = nlohmann::json(1);

  numTerminals = 4 * 4 * 5 * 1;
  pairs = {
    {0, 8},
    {4, 12},
    {8, 16},
    {12, 0},
    {16, 4}
  };

  for (u32 off = 0; off < 4; ++off) {
    for (u32 iface = 0; iface < 4; ++iface) {
      for (const auto& p : pairs) {
        src = p.first + 4 * 5 * off + iface;
        dst = p.second + 4 * 5 * off + iface;
        tp = new TornadoCTP(
            "TP", nullptr, numTerminals, src, settings);
        for (u32 idx = 0; idx < 100; ++idx) {
          u32 next = tp->nextDestination();
          ASSERT_LT(next, numTerminals);
          ASSERT_EQ(next, dst);
        }
        delete tp;
      }
    }
  }
}

TEST(TornadoCTP, dimension_0) {
  TestSetup test(1, 1, 1, 1, 0xBAADF00D);
  u32 src, dst, numTerminals;
  nlohmann::json settings;
  TornadoCTP* tp;
  std::map<u32, u32> pairs;

  settings["dimensions"][0] = nlohmann::json(5);
  settings["dimensions"][1] = nlohmann::json(4);
  settings["concentration"] = nlohmann::json(4);
  settings["interface_ports"] = nlohmann::json(1);
  settings["enabled_dimensions"][0] = true;
  settings["enabled_dimensions"][1] = false;

  numTerminals = 4 * 4 * 5 * 1;
  pairs = {
    {0, 8},
    {4, 12},
    {8, 16},
    {12, 0},
    {16, 4}
  };

  for (u32 off = 0; off < 4; ++off) {
    for (u32 iface = 0; iface < 4; ++iface) {
      for (const auto& p : pairs) {
        src = p.first + 4 * 5 * off + iface;
        dst = p.second + 4 * 5 * off + iface;
        tp = new TornadoCTP(
            "TP", nullptr, numTerminals, src, settings);
        for (u32 idx = 0; idx < 100; ++idx) {
          u32 next = tp->nextDestination();
          ASSERT_LT(next, numTerminals);
          ASSERT_EQ(next, dst);
        }
        delete tp;
      }
    }
  }
}

TEST(TornadoCTP, dimension_1) {
  TestSetup test(1, 1, 1, 1, 0xBAADF00D);
  u32 src, dst, numTerminals;
  nlohmann::json settings;
  TornadoCTP* tp;
  std::map<u32, u32> pairs;

  settings["dimensions"][0] = nlohmann::json(5);
  settings["dimensions"][1] = nlohmann::json(4);
  settings["concentration"] = nlohmann::json(4);
  settings["interface_ports"] = nlohmann::json(1);
  settings["enabled_dimensions"][0] = false;
  settings["enabled_dimensions"][1] = true;

  numTerminals = 4 * 4 * 5 * 1;
  pairs = {
    {0, 20},
    {20, 40},
    {40, 60},
    {60, 0}
  };

  for (u32 off = 0; off < 5; ++off) {
    for (u32 iface = 0; iface < 4; ++iface) {
      for (const auto& p : pairs) {
        src = p.first + 4 * off + iface;
        dst = p.second + 4 * off + iface;
        tp = new TornadoCTP(
            "TP", nullptr, numTerminals, src, settings);
        for (u32 idx = 0; idx < 100; ++idx) {
          u32 next = tp->nextDestination();
          ASSERT_LT(next, numTerminals);
          ASSERT_EQ(next, dst);
        }
        delete tp;
      }
    }
  }
}

TEST(TornadoCTP, dimension_1_3d) {
  TestSetup test(1, 1, 1, 1, 0xBAADF00D);
  u32 src, dst, numTerminals;
  nlohmann::json settings;
  TornadoCTP* tp;
  std::map<u32, u32> pairs;

  settings["dimensions"][0] = nlohmann::json(5);
  settings["dimensions"][1] = nlohmann::json(4);
  settings["dimensions"][2] = nlohmann::json(3);
  settings["concentration"] = nlohmann::json(4);
  settings["interface_ports"] = nlohmann::json(1);
  settings["enabled_dimensions"][0] = false;
  settings["enabled_dimensions"][1] = true;
  settings["enabled_dimensions"][2] = false;

  numTerminals = 3 * 4 * 4 * 5 * 1;
  pairs = {
    {0, 20},
    {20, 40},
    {40, 60},
    {60, 0}
  };

  for (u32 off = 0; off < 5; ++off) {
    for (u32 iface = 0; iface < 4; ++iface) {
      for (const auto& p : pairs) {
        src = p.first + 4 * off + iface;
        dst = p.second + 4 * off + iface;
        tp = new TornadoCTP(
            "TP", nullptr, numTerminals, src, settings);
        for (u32 idx = 0; idx < 100; ++idx) {
          u32 next = tp->nextDestination();
          ASSERT_LT(next, numTerminals);
          ASSERT_EQ(next, dst);
        }
        delete tp;
      }
    }
  }
}

TEST(TornadoCTP, dimension_1_3d_1) {
  TestSetup test(1, 1, 1, 1, 0xBAADF00D);
  u32 src, dst, numTerminals;
  nlohmann::json settings;
  TornadoCTP* tp;
  std::map<u32, u32> pairs;

  settings["dimensions"][0] = nlohmann::json(5);
  settings["dimensions"][1] = nlohmann::json(4);
  settings["dimensions"][2] = nlohmann::json(3);
  settings["concentration"] = nlohmann::json(4);
  settings["interface_ports"] = nlohmann::json(1);
  settings["enabled_dimensions"] = {false, true, false};

  numTerminals = 3 * 4 * 4 * 5 * 1;
  pairs = {
    {0, 20},
    {20, 40},
    {40, 60},
    {60, 0}
  };

  for (u32 off = 0; off < 5; ++off) {
    for (u32 iface = 0; iface < 4; ++iface) {
      for (const auto& p : pairs) {
        src = p.first + 4 * off + iface + 4 * 4 * 5;
        dst = p.second + 4 * off + iface + 4 * 4 * 5;
        tp = new TornadoCTP(
            "TP", nullptr, numTerminals, src, settings);
        for (u32 idx = 0; idx < 100; ++idx) {
          u32 next = tp->nextDestination();
          ASSERT_LT(next, numTerminals);
          ASSERT_EQ(next, dst);
        }
        delete tp;
      }
    }
  }
}

TEST(TornadoCTP, 2d) {
  TestSetup test(1, 1, 1, 1, 0xBAADF00D);
  u32 src, dst, numTerminals;
  nlohmann::json settings;
  TornadoCTP* tp;
  std::map<u32, u32> pairs;

  settings["dimensions"][0] = nlohmann::json(5);
  settings["dimensions"][1] = nlohmann::json(4);
  settings["concentration"] = nlohmann::json(4);
  settings["interface_ports"] = nlohmann::json(1);
  settings["enabled_dimensions"][0] = true;
  settings["enabled_dimensions"][1] = true;

  numTerminals = 4 * 4 * 5 * 1;
  pairs = {
    {0, 28},
    {4, 32},
    {8, 36},
    {12, 20},
    {16, 24},
    {20, 48},
    {24, 52},
    {28, 56},
    {32, 40},
    {36, 44},
    {40, 68},
    {44, 72},
    {48, 76},
    {52, 60},
    {56, 64},
    {60, 8},
    {64, 12},
    {68, 16},
    {72, 0},
    {76, 4}
  };

  for (u32 iface = 0; iface < 4; ++iface) {
    for (const auto& p : pairs) {
      src = p.first + iface;
      dst = p.second + iface;
      tp = new TornadoCTP(
          "TP", nullptr, numTerminals, src, settings);
      for (u32 idx = 0; idx < 100; ++idx) {
        u32 next = tp->nextDestination();
        ASSERT_LT(next, numTerminals);
        ASSERT_EQ(next, dst);
      }
      delete tp;
    }
  }
}

TEST(TornadoCTP, 3d) {
  TestSetup test(1, 1, 1, 1, 0xBAADF00D);
  u32 src, dst, numTerminals;
  nlohmann::json settings;
  TornadoCTP* tp;
  std::map<u32, u32> pairs;

  settings["dimensions"][0] = nlohmann::json(5);
  settings["dimensions"][1] = nlohmann::json(4);
  settings["dimensions"][2] = nlohmann::json(3);
  settings["concentration"] = nlohmann::json(4);
  settings["interface_ports"] = nlohmann::json(1);
  settings["enabled_dimensions"][0] = true;
  settings["enabled_dimensions"][1] = true;
  settings["enabled_dimensions"][2] = true;

  numTerminals = 3 * 4 * 4 * 5 * 1;
  pairs = {
    {0, 28},
    {4, 32},
    {8, 36},
    {12, 20},
    {16, 24},
    {20, 48},
    {24, 52},
    {28, 56},
    {32, 40},
    {36, 44},
    {40, 68},
    {44, 72},
    {48, 76},
    {52, 60},
    {56, 64},
    {60, 8},
    {64, 12},
    {68, 16},
    {72, 0},
    {76, 4}
  };

  for (u32 iface = 0; iface < 4; ++iface) {
    for (const auto& p : pairs) {
      src = p.first + iface + 4 * 4 * 5;
      dst = p.second + iface + 2 * 4 * 4 * 5;
      tp = new TornadoCTP(
          "TP", nullptr, numTerminals, src, settings);
      for (u32 idx = 0; idx < 100; ++idx) {
        u32 next = tp->nextDestination();
        ASSERT_LT(next, numTerminals);
        ASSERT_EQ(next, dst);
      }
      delete tp;
    }
  }
}

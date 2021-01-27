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
#include "traffic/continuous/BitReverseCTP.h"

#include "bits/bits.h"
#include "gtest/gtest.h"
#include "nlohmann/json.hpp"
#include "prim/prim.h"
#include "test/TestSetup_TESTLIB.h"

TEST(BitReverseCTP, simple) {
  TestSetup test(123, 123, 123, 123, 456789);

  const u32 TERMS = 1 << 8;
  for (u32 idx = 0; idx < TERMS; idx++) {
    nlohmann::json settings;
    BitReverseCTP tp("TP", nullptr, TERMS, idx, settings);
    for (u32 cnt = 0; cnt < 1000; cnt++) {
      u32 next = tp.nextDestination();
      ASSERT_LE(next, TERMS);
      ASSERT_EQ(next, bits::reverse<u32>(idx, bits::floorLog2(TERMS)));
    }
  }
}

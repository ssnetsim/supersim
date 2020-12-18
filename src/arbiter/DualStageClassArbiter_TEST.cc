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
#include <nlohmann/json.hpp>
#include <gtest/gtest.h>
#include <prim/prim.h>

#include <string>
#include <vector>

#include "arbiter/Arbiter.h"
#include "arbiter/DualStageClassArbiter.h"

#include "arbiter/Arbiter_TESTLIB.h"
#include "test/TestSetup_TESTLIB.h"


TEST(DualStageClassArbiter, full) {
  TestSetup testSetup(1, 1, 1, 1, 123);

  for (u32 size = 3; size < 100; size += 3) {
    bool* request = new bool[size];
    u64* metadata = new u64[size];
    bool* grant = new bool[size];
    do {
      for (u32 idx = 0; idx < size; idx++) {
        request[idx] = gSim->rnd.nextBool();
        metadata[idx] = idx + 10000;
      }
    } while (hotCount(request, size) <= size/2);

    nlohmann::json settings;
    settings["classes"] = 2;
    settings["class_map"] = nlohmann::json::array();
    settings["class_map"].push_back(0);
    settings["class_map"].push_back(1);
    settings["class_map"].push_back(1);
    settings["metadata_func"] = "min";
    settings["stage1_arbiter"] = nlohmann::json();
    settings["stage1_arbiter"]["type"] = "comparing";
    settings["stage1_arbiter"]["greater"] = false;
    settings["stage2_arbiter"] = nlohmann::json();
    settings["stage2_arbiter"]["type"] = "lru";

    Arbiter* arb = new DualStageClassArbiter(
        "Arb", nullptr, size, settings);
    assert(arb->size() == size);
    for (u32 idx = 0; idx < size; idx++) {
      arb->setRequest(idx, &request[idx]);
      arb->setMetadata(idx, &metadata[idx]);
      arb->setGrant(idx, &grant[idx]);
    }

    // do more arbitrations
    const u32 ARBS = 100;
    for (u32 a = 0; a < ARBS; a++) {
      memset(grant, false, size);
      arb->arbitrate();

      ASSERT_EQ(hotCount(grant, size), 1u);
      u32 awinner = winnerId(grant, size);
      ASSERT_TRUE(request[awinner]);
      // only latch the priority sometimes
      if (gSim->rnd.nextBool()) {
        arb->latch();
      }
    }

    // zero requests input test
    for (u32 idx = 0; idx < size; idx++) {
      request[idx] = false;
    }
    memset(grant, false, size);
    arb->arbitrate();
    ASSERT_EQ(hotCount(grant, size), 0u);

    // cleanup
    delete[] request;
    delete[] metadata;
    delete[] grant;
    delete arb;
  }
}

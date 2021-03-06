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
#include "routing/util.h"

#include <vector>

#include "gtest/gtest.h"

TEST(routing_util, vcToRc) {
  u32 rcs = 5;
  std::vector<u32> exp({0, 1, 2, 3, 4, 0, 1, 2});
  u32 num = exp.size();
  for (u32 base = 0; base < 16; base++) {
    for (u32 vc = base; vc < base + num; vc++) {
      u32 relVc = vc - base;
      ASSERT_EQ(vcToRc(base, num, vc, rcs), exp.at(relVc));
    }
  }
}

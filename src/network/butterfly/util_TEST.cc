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
#include "network/butterfly/util.h"

#include <gtest/gtest.h>
#include <prim/prim.h>

#include <vector>

TEST(Butterfly, translateInterfaceIdToAddress1) {
  const u32 routerRadix = 2;
  const u32 numStages = 3;
  const u32 stageWidth = 4;
  const u32 interfacePorts = 1;

  std::vector<u32> act;
  std::vector<u32> exp;
  u32 id;

  id = 0;
  exp = {0, 0, 0};
  Butterfly::translateInterfaceIdToAddress(
      routerRadix, numStages, stageWidth, interfacePorts, id, &act);
  ASSERT_EQ(act, exp);

  id = 3;
  exp = {0, 1, 1};
  Butterfly::translateInterfaceIdToAddress(
      routerRadix, numStages, stageWidth, interfacePorts, id, &act);
  ASSERT_EQ(act, exp);

  id = 4;
  exp = {1, 0, 0};
  Butterfly::translateInterfaceIdToAddress(
      routerRadix, numStages, stageWidth, interfacePorts, id, &act);
  ASSERT_EQ(act, exp);

  id = 6;
  exp = {1, 1, 0};
  Butterfly::translateInterfaceIdToAddress(
      routerRadix, numStages, stageWidth, interfacePorts, id, &act);
  ASSERT_EQ(act, exp);

  id = 7;
  exp = {1, 1, 1};
  Butterfly::translateInterfaceIdToAddress(
      routerRadix, numStages, stageWidth, interfacePorts, id, &act);
  ASSERT_EQ(act, exp);
}

TEST(Butterfly, translateInterfaceIdToAddress2) {
  const u32 routerRadix = 2;
  const u32 numStages = 3;
  const u32 stageWidth = 4;
  const u32 interfacePorts = 2;

  std::vector<u32> act;
  std::vector<u32> exp;
  u32 id;

  id = 0;
  exp = {0, 0, 0};
  Butterfly::translateInterfaceIdToAddress(
      routerRadix, numStages, stageWidth, interfacePorts, id, &act);
  ASSERT_EQ(act, exp);

  id = 1;
  exp = {0, 1, 0};
  Butterfly::translateInterfaceIdToAddress(
      routerRadix, numStages, stageWidth, interfacePorts, id, &act);
  ASSERT_EQ(act, exp);

  id = 2;
  exp = {1, 0, 0};
  Butterfly::translateInterfaceIdToAddress(
      routerRadix, numStages, stageWidth, interfacePorts, id, &act);
  ASSERT_EQ(act, exp);

  id = 3;
  exp = {1, 1, 0};
  Butterfly::translateInterfaceIdToAddress(
      routerRadix, numStages, stageWidth, interfacePorts, id, &act);
  ASSERT_EQ(act, exp);
}

TEST(Butterfly, translateInterfaceIdToAddress3) {
  const u32 routerRadix = 4;
  const u32 numStages = 3;
  const u32 stageWidth = 16;
  const u32 interfacePorts = 2;

  std::vector<u32> act;
  std::vector<u32> exp;
  u32 id;

  id = 0;
  exp = {0, 0, 0};
  Butterfly::translateInterfaceIdToAddress(
      routerRadix, numStages, stageWidth, interfacePorts, id, &act);
  ASSERT_EQ(act, exp);

  id = 1;
  exp = {0, 0, 1};
  Butterfly::translateInterfaceIdToAddress(
      routerRadix, numStages, stageWidth, interfacePorts, id, &act);
  ASSERT_EQ(act, exp);

  id = 7;
  exp = {0, 3, 1};
  Butterfly::translateInterfaceIdToAddress(
      routerRadix, numStages, stageWidth, interfacePorts, id, &act);
  ASSERT_EQ(act, exp);

  id = 8;
  exp = {1, 0, 0};
  Butterfly::translateInterfaceIdToAddress(
      routerRadix, numStages, stageWidth, interfacePorts, id, &act);
  ASSERT_EQ(act, exp);

  id = 31;
  exp = {3, 3, 1};
  Butterfly::translateInterfaceIdToAddress(
      routerRadix, numStages, stageWidth, interfacePorts, id, &act);
  ASSERT_EQ(act, exp);
}

TEST(Butterfly, translateInterfaceAddressToId1) {
  const u32 routerRadix = 2;
  const u32 numStages = 3;
  const u32 stageWidth = 4;
  const u32 interfacePorts = 1;

  std::vector<u32> addr;

  addr = {0, 0, 0};
  ASSERT_EQ(0u, Butterfly::translateInterfaceAddressToId(
      routerRadix, numStages, stageWidth, interfacePorts, &addr));

  addr = {0, 1, 1};
  ASSERT_EQ(3u, Butterfly::translateInterfaceAddressToId(
      routerRadix, numStages, stageWidth, interfacePorts, &addr));

  addr = {1, 0, 0};
  ASSERT_EQ(4u, Butterfly::translateInterfaceAddressToId(
      routerRadix, numStages, stageWidth, interfacePorts, &addr));

  addr = {1, 1, 0};
  ASSERT_EQ(6u, Butterfly::translateInterfaceAddressToId(
      routerRadix, numStages, stageWidth, interfacePorts, &addr));

  addr = {1, 1, 1};
  ASSERT_EQ(7u, Butterfly::translateInterfaceAddressToId(
      routerRadix, numStages, stageWidth, interfacePorts, &addr));
}

TEST(Butterfly, translateInterfaceAddressToId2) {
  const u32 routerRadix = 2;
  const u32 numStages = 3;
  const u32 stageWidth = 4;
  const u32 interfacePorts = 2;

  std::vector<u32> addr;

  addr = {0, 0, 0};
  ASSERT_EQ(0u, Butterfly::translateInterfaceAddressToId(
      routerRadix, numStages, stageWidth, interfacePorts, &addr));

  addr = {0, 1, 0};
  ASSERT_EQ(1u, Butterfly::translateInterfaceAddressToId(
      routerRadix, numStages, stageWidth, interfacePorts, &addr));

  addr = {1, 0, 0};
  ASSERT_EQ(2u, Butterfly::translateInterfaceAddressToId(
      routerRadix, numStages, stageWidth, interfacePorts, &addr));

  addr = {1, 1, 0};
  ASSERT_EQ(3u, Butterfly::translateInterfaceAddressToId(
      routerRadix, numStages, stageWidth, interfacePorts, &addr));
}

TEST(Butterfly, translateInterfaceAddressToId3) {
  const u32 routerRadix = 4;
  const u32 numStages = 3;
  const u32 stageWidth = 16;
  const u32 interfacePorts = 2;

  std::vector<u32> addr;

  addr = {0, 0, 0};
  ASSERT_EQ(0u, Butterfly::translateInterfaceAddressToId(
      routerRadix, numStages, stageWidth, interfacePorts, &addr));

  addr = {0, 0, 1};
  ASSERT_EQ(1u, Butterfly::translateInterfaceAddressToId(
      routerRadix, numStages, stageWidth, interfacePorts, &addr));

  addr = {0, 1, 0};
  ASSERT_EQ(2u, Butterfly::translateInterfaceAddressToId(
      routerRadix, numStages, stageWidth, interfacePorts, &addr));

  addr = {0, 3, 1};
  ASSERT_EQ(7u, Butterfly::translateInterfaceAddressToId(
      routerRadix, numStages, stageWidth, interfacePorts, &addr));

  addr = {1, 0, 0};
  ASSERT_EQ(8u, Butterfly::translateInterfaceAddressToId(
      routerRadix, numStages, stageWidth, interfacePorts, &addr));

  addr = {1, 3, 1};
  ASSERT_EQ(15u, Butterfly::translateInterfaceAddressToId(
      routerRadix, numStages, stageWidth, interfacePorts, &addr));

  addr = {3, 0, 0};
  ASSERT_EQ(24u, Butterfly::translateInterfaceAddressToId(
      routerRadix, numStages, stageWidth, interfacePorts, &addr));

  addr = {3, 2, 1};
  ASSERT_EQ(29u, Butterfly::translateInterfaceAddressToId(
      routerRadix, numStages, stageWidth, interfacePorts, &addr));

  addr = {3, 3, 0};
  ASSERT_EQ(30u, Butterfly::translateInterfaceAddressToId(
      routerRadix, numStages, stageWidth, interfacePorts, &addr));

  addr = {3, 3, 1};
  ASSERT_EQ(31u, Butterfly::translateInterfaceAddressToId(
      routerRadix, numStages, stageWidth, interfacePorts, &addr));
}

TEST(Butterfly, translateRouterIdToAddress) {
  const u32 routerRadix = 2;
  const u32 numStages = 3;
  const u32 stageWidth = 4;

  std::vector<u32> act;
  std::vector<u32> exp;
  u32 id;

  id = 0;
  exp = {0, 0};
  Butterfly::translateRouterIdToAddress(routerRadix, numStages, stageWidth,
                                        id, &act);
  ASSERT_EQ(act, exp);

  id = 1;
  exp = {0, 1};
  Butterfly::translateRouterIdToAddress(routerRadix, numStages, stageWidth,
                                        id, &act);
  ASSERT_EQ(act, exp);

  id = 3;
  exp = {0, 3};
  Butterfly::translateRouterIdToAddress(routerRadix, numStages, stageWidth,
                                        id, &act);
  ASSERT_EQ(act, exp);

  id = 4;
  exp = {1, 0};
  Butterfly::translateRouterIdToAddress(routerRadix, numStages, stageWidth,
                                        id, &act);
  ASSERT_EQ(act, exp);

  id = 10;
  exp = {2, 2};
  Butterfly::translateRouterIdToAddress(routerRadix, numStages, stageWidth,
                                        id, &act);
  ASSERT_EQ(act, exp);
}

TEST(Butterfly, translateRouterAddressToId) {
  const u32 routerRadix = 2;
  const u32 numStages = 3;
  const u32 stageWidth = 4;

  std::vector<u32> addr;

  addr = {0, 0};
  ASSERT_EQ(0u, Butterfly::translateRouterAddressToId(
      routerRadix, numStages, stageWidth, &addr));

  addr = {0, 1};
  ASSERT_EQ(1u, Butterfly::translateRouterAddressToId(
      routerRadix, numStages, stageWidth, &addr));

  addr = {0, 3};
  ASSERT_EQ(3u, Butterfly::translateRouterAddressToId(
      routerRadix, numStages, stageWidth, &addr));

  addr = {1, 0};
  ASSERT_EQ(4u, Butterfly::translateRouterAddressToId(
      routerRadix, numStages, stageWidth, &addr));

  addr = {2, 2};
  ASSERT_EQ(10u, Butterfly::translateRouterAddressToId(
      routerRadix, numStages, stageWidth, &addr));
}

TEST(Butterfly, computeMinimalHops) {
  u32 numStages;
  u32 exp;

  numStages = 3;
  exp = 3;
  ASSERT_EQ(exp, Butterfly::computeMinimalHops(numStages));

  numStages = 5;
  exp = 5;
  ASSERT_EQ(exp, Butterfly::computeMinimalHops(numStages));

  numStages = 10;
  exp = 10;
  ASSERT_EQ(exp, Butterfly::computeMinimalHops(numStages));
}

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
#include <string>

#include "event/Component.h"
#include "event/Simulator.h"
#include "event/VectorQueue.h"
#include "nlohmann/json.hpp"
#include "settings/settings.h"
#include "test/TestSetup_TESTLIB.h"

TestSetup::TestSetup(u64 _channelCycleTime, u64 _routerCycleTime,
                     u64 _interfaceCycleTime, u64 _terminalCycleTime,
                     u64 _randomSeed) {
  std::string str =
      std::string("{\n") +
      "  \"simulator\": {\n" +
      "     \"channel_cycle_time\": " + std::to_string(_channelCycleTime) +
      ",\n" +
      "     \"router_cycle_time\": " +
      std::to_string(_routerCycleTime) + ",\n" +
      "     \"interface_cycle_time\": " +
      std::to_string(_interfaceCycleTime) + ",\n" +
      "     \"terminal_cycle_time\": " +
      std::to_string(_terminalCycleTime) + ",\n" +
      "     \"print_progress\": false,\n" +
      "     \"print_interval\": 1.0,\n" +
      "     \"random_seed\": " + std::to_string(_randomSeed) + "\n" +
      "  }\n" +
      "}\n" +
      std::string();

  nlohmann::json settings;
  settings::initString(str.c_str(), &settings);

  gSim = new VectorQueue(settings["simulator"]);
}

TestSetup::~TestSetup() {
  delete gSim;
  gSim = nullptr;
  Component::clearNames();
}

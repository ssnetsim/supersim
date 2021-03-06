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
#include "network/hyperx/MinRoutingAlgorithm.h"

#include <vector>

#include "gtest/gtest.h"
#include "network/hyperx/RoutingAlgorithm.h"
#include "nlohmann/json.hpp"
#include "prim/prim.h"
#include "routing/RoutingAlgorithm_TESTLIB.h"

TEST(HyperX_MinRoutingAlgorithm, construct) {
  std::vector<u32> latencies({1, 2, 3, 4});
  std::vector<std::string> outputTypes({"vc", "port"});
  std::vector<std::string> outputAlgs({"minimal", "random"});
  std::vector<u32> maxOutputs({0, 1, 2, 3, 4});
  std::vector<std::string> minTypes({"random", "adaptive"});

  for (auto& latency : latencies) {
    for (auto& outputType : outputTypes) {
      for (auto& outputAlg : outputAlgs) {
        for (auto& maxOutput : maxOutputs) {
          for (auto& minType : minTypes) {
            nlohmann::json settings;
            settings["algorithm"] = "minimal";
            settings["latency"] = latency;
            settings["output_type"] = outputType;
            settings["output_algorithm"] = outputAlg;
            settings["max_outputs"] = maxOutput;
            settings["minimal"] = minType;
            RoutingAlgorithmTestRouter tr("Router", 16, 24);
            HyperX::RoutingAlgorithm* ra = HyperX::RoutingAlgorithm::create(
                "RoutingAlgorithm", &tr, &tr, 1, 12, 1, 1, {2, 3, 4}, {4, 3, 1},
                6, 1, settings);
            delete ra;
          }
        }
      }
    }
  }
}

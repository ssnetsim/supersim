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
#ifndef TRAFFIC_DISTRIBUTION_RANDOMDTP_H_
#define TRAFFIC_DISTRIBUTION_RANDOMDTP_H_

#include <string>
#include <vector>

#include "nlohmann/json.hpp"
#include "prim/prim.h"
#include "traffic/distribution/DistributionTrafficPattern.h"

class RandomDTP : public DistributionTrafficPattern {
 public:
  RandomDTP(const std::string& _name, const Component* _parent,
            u32 _numTerminals, u32 _self, nlohmann::json _settings);
  ~RandomDTP();

  u32 size() const override;
  u32 nextDestination() override;
  bool complete() const override;
  void reset() override;

 private:
  bool sendToSelf_;
  std::vector<u32> destinations_;
};

#endif  // TRAFFIC_DISTRIBUTION_RANDOMDTP_H_

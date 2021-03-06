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
#ifndef TRAFFIC_CONTINUOUS_UNIFORMRANDOMQUADRANTCTP_H_
#define TRAFFIC_CONTINUOUS_UNIFORMRANDOMQUADRANTCTP_H_

#include <string>
#include <vector>

#include "nlohmann/json.hpp"
#include "prim/prim.h"
#include "traffic/continuous/ContinuousTrafficPattern.h"

class UniformRandomQuadrantCTP : public ContinuousTrafficPattern {
 public:
  UniformRandomQuadrantCTP(const std::string& _name, const Component* _parent,
                           u32 _numTerminals, u32 _self,
                           nlohmann::json _settings);

  ~UniformRandomQuadrantCTP();

  u32 nextDestination() override;

 private:
  std::vector<u32> dstVect_;
};

#endif  // TRAFFIC_CONTINUOUS_UNIFORMRANDOMQUADRANTCTP_H_

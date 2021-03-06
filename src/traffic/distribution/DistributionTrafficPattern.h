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
#ifndef TRAFFIC_DISTRIBUTION_DISTRIBUTIONTRAFFICPATTERN_H_
#define TRAFFIC_DISTRIBUTION_DISTRIBUTIONTRAFFICPATTERN_H_

#include <string>

#include "event/Component.h"
#include "nlohmann/json.hpp"
#include "prim/prim.h"

#define DISTRIBUTIONTRAFFICPATTERN_ARGS \
  const std::string&, const Component*, u32, u32, nlohmann::json

class DistributionTrafficPattern : public Component {
 public:
  DistributionTrafficPattern(const std::string& _name, const Component* _parent,
                             u32 _numTerminals, u32 _self,
                             nlohmann::json _settings);
  virtual ~DistributionTrafficPattern();

  // this is the factory for distribution traffic patterns
  static DistributionTrafficPattern* create(DISTRIBUTIONTRAFFICPATTERN_ARGS);

  virtual u32 size() const = 0;
  virtual u32 nextDestination() = 0;
  virtual bool complete() const = 0;
  virtual void reset() = 0;

 protected:
  const u32 numTerminals_;
  const u32 self_;
};

#endif  // TRAFFIC_DISTRIBUTION_DISTRIBUTIONTRAFFICPATTERN_H_

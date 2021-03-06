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
#include "congestion/CongestionSensor.h"

#include <algorithm>
#include <cassert>
#include <cmath>

#include "factory/ObjectFactory.h"
#include "router/Router.h"

CongestionSensor::CongestionSensor(const std::string& _name,
                                   const Component* _parent,
                                   PortedDevice* _device,
                                   nlohmann::json _settings)
    : Component(_name, _parent),
      device_(_device),
      numPorts_(device_->numPorts()),
      numVcs_(device_->numVcs()),
      granularity_(_settings["granularity"].get<u32>()),
      minimum_(_settings["minimum"].get<f64>()),
      offset_(_settings["offset"].get<f64>()) {
  assert(!_settings["granularity"].is_null());
  assert(!_settings["minimum"].is_null());
  assert(!_settings["offset"].is_null());
  assert(minimum_ >= 0.0);
  assert(offset_ >= 0.0);
}

CongestionSensor::~CongestionSensor() {}

CongestionSensor* CongestionSensor::create(const std::string& _name,
                                           const Component* _parent,
                                           PortedDevice* _device,
                                           nlohmann::json _settings) {
  // retrieve the algorithm
  const std::string& algorithm = _settings["algorithm"].get<std::string>();

  // attempt to build the congestion status
  CongestionSensor* cs =
      factory::ObjectFactory<CongestionSensor, CONGESTIONSENSOR_ARGS>::create(
          algorithm, _name, _parent, _device, _settings);

  // check that the algorithm exists within the factory
  if (cs == nullptr) {
    fprintf(stderr, "unknown congestion status algorithm: %s\n",
            algorithm.c_str());
    assert(false);
  }
  return cs;
}

f64 CongestionSensor::status(u32 _inputPort, u32 _inputVc, u32 _outputPort,
                             u32 _outputVc) const {
  assert(gSim->epsilon() == 0);

  // gather value from subclass
  f64 value = computeStatus(_inputPort, _inputVc, _outputPort, _outputVc);

  // check bounds
  assert(value >= 0.0);

  // apply granularization
  if (granularity_ > 0) {
    value = std::round(value * granularity_) / granularity_;
  }

  // apply offset and minimum constraints
  value = offset_ + std::max(minimum_, value);

  return value;
}

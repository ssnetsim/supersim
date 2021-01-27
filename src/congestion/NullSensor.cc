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
#include "congestion/NullSensor.h"

#include <algorithm>

#include "factory/ObjectFactory.h"

NullSensor::NullSensor(
    const std::string& _name, const Component* _parent, PortedDevice* _device,
    nlohmann::json _settings)
    : CongestionSensor(_name, _parent, _device, _settings) {}

NullSensor::~NullSensor() {}

void NullSensor::initCredits(u32 _vcIdx, u32 _credits) {}

void NullSensor::incrementCredit(u32 _vcIdx) {}

void NullSensor::decrementCredit(u32 _vcIdx) {}

CongestionSensor::Style NullSensor::style() const {
  return CongestionSensor::Style::kNull;
}

CongestionSensor::Resolution NullSensor::resolution() const {
  return CongestionSensor::Resolution::kNull;
}

f64 NullSensor::computeStatus(
    u32 _inputPort, u32 _inputVc, u32 _outputPort, u32 _outputVc) const {
  // asserting false caused problems with least-congested minimal and weighted
  //  reductions, returning 0.0 instead
  return 0.0;
}

registerWithObjectFactory("null_sensor", CongestionSensor,
                          NullSensor, CONGESTIONSENSOR_ARGS);

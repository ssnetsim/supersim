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
#include "traffic/continuous/DimTransposeCTP.h"

#include <cassert>
#include <vector>

#include "factory/ObjectFactory.h"
#include "network/cube/util.h"

DimTransposeCTP::DimTransposeCTP(const std::string& _name,
                                 const Component* _parent, u32 _numTerminals,
                                 u32 _self, nlohmann::json _settings)
    : ContinuousTrafficPattern(_name, _parent, _numTerminals, _self,
                               _settings) {
  // parse the settings
  assert(_settings.contains("dimensions") &&
         _settings["dimensions"].is_array());
  assert(_settings.contains("concentration") &&
         _settings["concentration"].is_number_integer());
  assert(_settings.contains("interface_ports") &&
         _settings["interface_ports"].is_number_integer());
  const u32 dimensions = _settings["dimensions"].size();
  assert(dimensions > 1);
  std::vector<u32> widths(dimensions);
  for (u32 i = 0; i < dimensions; i++) {
    widths.at(i) = _settings["dimensions"][i].get<u32>();
  }
  const u32 concentration = _settings["concentration"].get<u32>();
  const u32 interfacePorts = _settings["interface_ports"].get<u32>();

  std::vector<u32> workingDims(2);
  std::vector<bool> dimMask(dimensions, false);
  if (_settings.contains("enabled_dimensions")) {
    assert(_settings["enabled_dimensions"].is_array());
    assert(_settings["enabled_dimensions"].size() == dimensions);
    u32 trueCntr = 0;
    for (u32 dim = 0; dim < dimensions; ++dim) {
      dimMask.at(dim) = _settings["enabled_dimensions"][dim].get<bool>();
      if (dimMask.at(dim) == true) {
        trueCntr++;
      }
    }
    assert(trueCntr == 2);
  } else {
    dimMask.at(0) = true;
    dimMask.at(1) = true;
  }

  u32 idx = 0;
  for (u32 dim = 0; dim < dimensions; ++dim) {
    if (dimMask.at(dim) == true) {
      workingDims.at(idx) = dim;
      idx++;
    }
  }
  assert(widths.at(workingDims.at(0)) == widths.at(workingDims.at(1)));

  // get self as a vector address
  std::vector<u32> addr;
  Cube::translateInterfaceIdToAddress(self_, widths, concentration,
                                      interfacePorts, &addr);

  u32 idx0 = workingDims.at(0) + 1;
  u32 idx1 = workingDims.at(1) + 1;
  u32 tmp = addr.at(idx0);
  addr.at(idx0) = addr.at(idx1);
  addr.at(idx1) = tmp;

  // compute the tornado destination id
  dest_ = Cube::translateInterfaceAddressToId(&addr, widths, concentration,
                                              interfacePorts);
}

DimTransposeCTP::~DimTransposeCTP() {}

u32 DimTransposeCTP::nextDestination() {
  return dest_;
}

registerWithObjectFactory("dim_transpose", ContinuousTrafficPattern,
                          DimTransposeCTP, CONTINUOUSTRAFFICPATTERN_ARGS);

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
#include "traffic/continuous/DimComplementReverseCTP.h"

#include <cassert>
#include <vector>

#include "factory/ObjectFactory.h"
#include "network/cube/util.h"

DimComplementReverseCTP::DimComplementReverseCTP(const std::string& _name,
                                                 const Component* _parent,
                                                 u32 _numTerminals, u32 _self,
                                                 nlohmann::json _settings)
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
  std::vector<u32> widths(dimensions);
  for (u32 i = 0; i < dimensions; i++) {
    widths.at(i) = _settings["dimensions"][i].get<u32>();
  }
  const u32 concentration = _settings["concentration"].get<u32>();
  const u32 interfacePorts = _settings["interface_ports"].get<u32>();

  for (u32 i = 1; i < dimensions / 2; i++) {
    assert(widths.at(i) == widths.at(dimensions - i - 1));
  }

  // get self as a vector address
  std::vector<u32> addr;
  Cube::translateInterfaceIdToAddress(self_, widths, concentration,
                                      interfacePorts, &addr);

  for (u32 dim = 0; dim < dimensions / 2; dim++) {
    u32 tmp = widths.at(dim) - 1 - addr.at(dim + 1);
    addr.at(dim + 1) = widths.at(dim) - 1 - addr.at(dimensions - dim);
    addr.at(dimensions - dim) = tmp;
  }

  if (dimensions % 2 == 1) {
    u32 dim = dimensions / 2;
    addr.at(dim + 1) = widths.at(dim) - 1 - addr.at(dim + 1);
  }

  // compute the tornado destination id
  dest_ = Cube::translateInterfaceAddressToId(&addr, widths, concentration,
                                              interfacePorts);
}

DimComplementReverseCTP::~DimComplementReverseCTP() {}

u32 DimComplementReverseCTP::nextDestination() {
  return dest_;
}

registerWithObjectFactory("dim_complement_reverse", ContinuousTrafficPattern,
                          DimComplementReverseCTP,
                          CONTINUOUSTRAFFICPATTERN_ARGS);

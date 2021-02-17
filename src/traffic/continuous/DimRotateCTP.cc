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
#include "traffic/continuous/DimRotateCTP.h"

#include <cassert>
#include <vector>

#include "factory/ObjectFactory.h"
#include "network/cube/util.h"

DimRotateCTP::DimRotateCTP(const std::string& _name, const Component* _parent,
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

  assert(_settings.contains("direction"));
  assert(_settings["direction"].is_string());
  std::string dir = _settings["direction"].get<std::string>();

  // get self as a vector address
  std::vector<u32> addr;
  Cube::translateInterfaceIdToAddress(self_, widths, concentration,
                                      interfacePorts, &addr);

  if (dir == "left") {
    u32 tmp = addr.at(1);
    for (u32 dim = 1; dim < dimensions; dim++) {
      addr.at(dim) = addr.at(dim + 1);
    }
    addr.at(dimensions) = tmp;
  } else if (dir == "right") {
    u32 tmp = addr.at(dimensions);
    for (u32 dim = dimensions; dim > 1; dim--) {
      addr.at(dim) = addr.at(dim - 1);
    }
    addr.at(1) = tmp;
  } else {
    fprintf(stderr, "invalid direction spec: %s\n", dir.c_str());
    assert(false);
  }

  // compute the tornado destination id
  dest_ = Cube::translateInterfaceAddressToId(&addr, widths, concentration,
                                              interfacePorts);
}

DimRotateCTP::~DimRotateCTP() {}

u32 DimRotateCTP::nextDestination() {
  return dest_;
}

registerWithObjectFactory("dim_rotate", ContinuousTrafficPattern, DimRotateCTP,
                          CONTINUOUSTRAFFICPATTERN_ARGS);

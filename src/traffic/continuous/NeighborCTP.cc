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
#include "traffic/continuous/NeighborCTP.h"

#include <cassert>
#include <vector>

#include "factory/ObjectFactory.h"
#include "network/cube/util.h"

NeighborCTP::NeighborCTP(const std::string& _name, const Component* _parent,
                         u32 _numTerminals, u32 _self, nlohmann::json _settings)
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

  std::vector<bool> dimMask(dimensions, false);
  if (_settings.contains("enabled_dimensions")) {
    assert(_settings["enabled_dimensions"].is_array());
    assert(_settings["enabled_dimensions"].size() == dimensions);
    for (u32 dim = 0; dim < dimensions; ++dim) {
      dimMask.at(dim) = _settings["enabled_dimensions"][dim].get<bool>();
    }
  } else {
    dimMask.at(0) = true;
  }

  assert(_settings.contains("direction"));
  assert(_settings["direction"].is_string());
  std::string dir = _settings["direction"].get<std::string>();

  // get self as a vector address
  std::vector<u32> addr;
  Cube::translateInterfaceIdToAddress(self_, widths, concentration,
                                      interfacePorts, &addr);

  // compute the destination vector address
  for (u32 dim = 0; dim < dimensions; dim++) {
    if (dimMask.at(dim)) {
      u32 dimOffset = 0;
      if (dir == "right") {
        dimOffset = 1;
      } else if (dir == "left") {
        dimOffset = widths.at(dim) - 1;
      } else {
        fprintf(stderr, "invalid direction spec: %s\n", dir.c_str());
        assert(false);
      }

      u32 idx = dim + 1;
      addr.at(idx) = (addr.at(idx) + dimOffset) % widths.at(dim);
    }
  }

  // compute the  destination id
  dest_ = Cube::translateInterfaceAddressToId(&addr, widths, concentration,
                                              interfacePorts);
}

NeighborCTP::~NeighborCTP() {}

u32 NeighborCTP::nextDestination() {
  return dest_;
}

registerWithObjectFactory("neighbor", ContinuousTrafficPattern, NeighborCTP,
                          CONTINUOUSTRAFFICPATTERN_ARGS);

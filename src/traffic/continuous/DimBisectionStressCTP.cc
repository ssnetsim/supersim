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
#include "traffic/continuous/DimBisectionStressCTP.h"

#include <cassert>
#include <vector>

#include "factory/ObjectFactory.h"
#include "network/cube/util.h"

DimBisectionStressCTP::DimBisectionStressCTP(const std::string& _name,
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

  assert(_settings.contains("mode") && _settings["mode"].is_string());
  for (u32 i = 0; i < dimensions; i++) {
    assert(widths.at(i) % 2 == 0);
  }

  // get self as a vector address
  std::vector<u32> addr;
  Cube::translateInterfaceIdToAddress(self_, widths, concentration,
                                      interfacePorts, &addr);

  const u32 interfacesPerRouter = concentration / interfacePorts;
  u32 nodeGroup = 0;
  if (_settings["mode"] == "parity") {
    assert(interfacesPerRouter % 2 == 0);
    nodeGroup = addr.at(0) % 2;
  } else if (_settings["mode"] == "half") {
    assert(widths.at(dimensions - 1) % 2 == 0);
    nodeGroup = self_ < numTerminals_ / 2 ? 0 : 1;
  } else if (_settings["mode"] == "quadrant") {
    u32 paritySum = 0;
    for (u32 i = 0; i < dimensions; i++) {
      paritySum =
          addr.at(i + 1) < (widths.at(i) / 2) ? paritySum : paritySum + 1;
    }
    nodeGroup = paritySum % 2;
  } else {
    fprintf(stderr, "Unknown dim bisection stress mode\n");
    assert(false);
  }

  for (u32 dim = 0; dim < dimensions; dim++) {
    if (nodeGroup > 0) {
      // Send in Exchange manner half width across the bisection
      if (addr.at(dim + 1) < (widths.at(dim) / 2)) {
        addr.at(dim + 1) += (widths.at(dim) + 1) / 2;
      } else if (addr.at(dim + 1) > ((widths.at(dim) - 1) / 2)) {
        addr.at(dim + 1) -= (widths.at(dim) + 1) / 2;
      }
    } else {
      addr.at(dim + 1) = widths.at(dim) - 1 - addr.at(dim + 1);
    }
  }

  // compute the destination id
  dest_ = Cube::translateInterfaceAddressToId(&addr, widths, concentration,
                                              interfacePorts);
}

DimBisectionStressCTP::~DimBisectionStressCTP() {}

u32 DimBisectionStressCTP::nextDestination() {
  return dest_;
}

registerWithObjectFactory("dim_bisection_stress", ContinuousTrafficPattern,
                          DimBisectionStressCTP, CONTINUOUSTRAFFICPATTERN_ARGS);

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
#include "traffic/continuous/RandomExchangeNeighborCTP.h"

#include <cassert>
#include <vector>

#include "factory/ObjectFactory.h"
#include "network/cube/util.h"

RandomExchangeNeighborCTP::RandomExchangeNeighborCTP(const std::string& _name,
                                                     const Component* _parent,
                                                     u32 _numTerminals,
                                                     u32 _self,
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
  std::vector<u32> widths;
  widths.resize(dimensions);
  for (u32 i = 0; i < dimensions; i++) {
    widths.at(i) = _settings["dimensions"][i].get<u32>();
  }
  const u32 concentration = _settings["concentration"].get<u32>();
  const u32 interfacePorts = _settings["interface_ports"].get<u32>();
  bool allInterfaces = _settings["all_interfaces"].get<bool>();

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

  assert(dimensions > 1);
  for (u32 i = 0; i < dimensions; i++) {
    assert(widths.at(i) % 2 == 0);
  }

  const u32 interfacesPerRouter = concentration / interfacePorts;
  for (u32 dim = 0; dim < dimensions; ++dim) {
    if (dimMask.at(dim)) {
      std::vector<u32> addr;
      // get self as a vector address
      Cube::translateInterfaceIdToAddress(self_, widths, concentration,
                                          interfacePorts, &addr);
      addr.at(dim + 1) =
          (addr.at(dim + 1) + widths.at(dim) - 1) % widths.at(dim);
      if (allInterfaces) {
        for (u32 iface = 0; iface < interfacesPerRouter; ++iface) {
          addr.at(0) = iface;
          u32 dstId = Cube::translateInterfaceAddressToId(
              &addr, widths, concentration, interfacePorts);
          dstVect_.emplace_back(dstId);
        }
      } else {
        u32 dstId = Cube::translateInterfaceAddressToId(
            &addr, widths, concentration, interfacePorts);
        dstVect_.emplace_back(dstId);
      }
      addr.at(dim + 1) = (addr.at(dim + 1) + 2) % widths.at(dim);
      if (allInterfaces) {
        for (u32 iface = 0; iface < interfacesPerRouter; ++iface) {
          addr.at(0) = iface;
          u32 dstId = Cube::translateInterfaceAddressToId(
              &addr, widths, concentration, interfacePorts);
          dstVect_.emplace_back(dstId);
        }
      } else {
        u32 dstId = Cube::translateInterfaceAddressToId(
            &addr, widths, concentration, interfacePorts);
        dstVect_.emplace_back(dstId);
      }
    }
  }
}

RandomExchangeNeighborCTP::~RandomExchangeNeighborCTP() {}

u32 RandomExchangeNeighborCTP::nextDestination() {
  return dstVect_.at(gSim->rnd.nextU64(0, dstVect_.size() - 1));
}

registerWithObjectFactory("random_exchange_neighbor", ContinuousTrafficPattern,
                          RandomExchangeNeighborCTP,
                          CONTINUOUSTRAFFICPATTERN_ARGS);

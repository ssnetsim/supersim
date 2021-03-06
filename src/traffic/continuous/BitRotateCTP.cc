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
#include "traffic/continuous/BitRotateCTP.h"

#include <cassert>

#include "bits/bits.h"
#include "factory/ObjectFactory.h"

BitRotateCTP::BitRotateCTP(const std::string& _name, const Component* _parent,
                           u32 _numTerminals, u32 _self,
                           nlohmann::json _settings)
    : ContinuousTrafficPattern(_name, _parent, _numTerminals, _self,
                               _settings) {
  assert(bits::isPow2(numTerminals_));
  assert(_settings.contains("direction"));
  assert(_settings["direction"].is_string());
  std::string dir = _settings["direction"].get<std::string>();
  if (dir == "right") {
    dest_ = bits::rotateRight<u32>(self_, bits::floorLog2(numTerminals_));
  } else if (dir == "left") {
    dest_ = bits::rotateLeft<u32>(self_, bits::floorLog2(numTerminals_));
  } else {
    fprintf(stderr, "invalid direction spec: %s\n", dir.c_str());
    assert(false);
  }
}

BitRotateCTP::~BitRotateCTP() {}

u32 BitRotateCTP::nextDestination() {
  return dest_;
}

registerWithObjectFactory("bit_rotate", ContinuousTrafficPattern, BitRotateCTP,
                          CONTINUOUSTRAFFICPATTERN_ARGS);

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
#include "traffic/continuous/LoopbackCTP.h"

#include <cassert>

#include "factory/ObjectFactory.h"

LoopbackCTP::LoopbackCTP(
    const std::string& _name, const Component* _parent, u32 _numTerminals,
    u32 _self, nlohmann::json _settings)
    : ContinuousTrafficPattern(_name, _parent, _numTerminals, _self,
                               _settings) {}

LoopbackCTP::~LoopbackCTP() {}

u32 LoopbackCTP::nextDestination() {
  return self_;
}

registerWithObjectFactory("loopback", ContinuousTrafficPattern,
                          LoopbackCTP, CONTINUOUSTRAFFICPATTERN_ARGS);

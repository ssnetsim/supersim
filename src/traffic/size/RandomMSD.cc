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
#include "traffic/size/RandomMSD.h"

#include <cassert>

#include "event/Simulator.h"
#include "factory/ObjectFactory.h"

RandomMSD::RandomMSD(
    const std::string& _name, const Component* _parent,
    nlohmann::json _settings)
    : MessageSizeDistribution(_name, _parent, _settings),
      minMessageSize_(_settings["min_message_size"].get<u32>()),
      maxMessageSize_(_settings["max_message_size"].get<u32>()),
      doDependent_(_settings.contains("dependent_min_message_size") &&
                   _settings.contains("dependent_max_message_size")),
      depMinMessageSize_(_settings.value("dependent_min_message_size", 0)),
      depMaxMessageSize_(_settings.value("dependent_max_message_size", 0)) {
  assert(minMessageSize_ > 0);
  assert(maxMessageSize_ > 0);
  assert(maxMessageSize_ >= minMessageSize_);
  if (doDependent_) {
    assert(depMinMessageSize_ > 0);
    assert(depMaxMessageSize_ > 0);
    assert(depMaxMessageSize_ >= depMinMessageSize_);
  } else {
    assert(_settings["dependent_min_message_size"].is_null());
    assert(_settings["dependent_max_message_size"].is_null());
  }
}

RandomMSD::~RandomMSD() {}

u32 RandomMSD::minMessageSize() const {
  return minMessageSize_;
}

u32 RandomMSD::maxMessageSize() const {
  return maxMessageSize_;
}

u32 RandomMSD::nextMessageSize() {
  return gSim->rnd.nextU64(minMessageSize_, maxMessageSize_);
}

u32 RandomMSD::nextMessageSize(const Message* _msg) {
  if (doDependent_) {
    return gSim->rnd.nextU64(depMinMessageSize_, depMaxMessageSize_);
  } else {
    return nextMessageSize();
  }
}

registerWithObjectFactory("random", MessageSizeDistribution,
                          RandomMSD,
                          MESSAGESIZEDISTRIBUTION_ARGS);

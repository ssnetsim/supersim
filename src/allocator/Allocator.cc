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
#include "allocator/Allocator.h"

#include <cassert>

#include "factory/ObjectFactory.h"

Allocator::Allocator(const std::string& _name, const Component* _parent,
                     u32 _numClients, u32 _numResources,
                     nlohmann::json _settings)
    : Component(_name, _parent),
      numClients_(_numClients),
      numResources_(_numResources) {
  assert(numClients_ > 0);
  assert(numResources_ > 0);
}

Allocator::~Allocator() {}

Allocator* Allocator::create(const std::string& _name, const Component* _parent,
                             u32 _numClients, u32 _numResources,
                             nlohmann::json _settings) {
  // retrieve the allocator type
  const std::string& type = _settings["type"].get<std::string>();

  // try to construct an allocator
  Allocator* alloc = factory::ObjectFactory<Allocator, ALLOCATOR_ARGS>::create(
      type, _name, _parent, _numClients, _numResources, _settings);

  // check that the factory had an entry for that type
  if (alloc == nullptr) {
    fprintf(stderr, "unknown Allocator type: %s\n", type.c_str());
    assert(false);
  }
  return alloc;
}

u32 Allocator::numClients() const {
  return numClients_;
}

u32 Allocator::numResources() const {
  return numResources_;
}

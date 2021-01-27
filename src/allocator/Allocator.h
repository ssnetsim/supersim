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
#ifndef ALLOCATOR_ALLOCATOR_H_
#define ALLOCATOR_ALLOCATOR_H_

#include <string>
#include <vector>

#include "arbiter/Arbiter.h"
#include "event/Component.h"
#include "nlohmann/json.hpp"
#include "prim/prim.h"

#define ALLOCATOR_ARGS const std::string&, const Component*, u32, u32,  \
    nlohmann::json

class Allocator : public Component {
 public:
  Allocator(const std::string& _name, const Component* _parent,
            u32 _numClients, u32 _numResources, nlohmann::json _settings);
  virtual ~Allocator();

  // this is the factory for allocators
  static Allocator* create(ALLOCATOR_ARGS);

  // returns number of clients
  u32 numClients() const;
  // returns number of resources
  u32 numResources() const;
  // maps the request to the specified client and resource
  virtual void setRequest(u32 _client, u32 _resource, bool* _request) = 0;
  // maps the metadata to the specified client and resource
  virtual void setMetadata(u32 _client, u32 _resource, u64* _metadata) = 0;
  // maps the grant to the specified client and resource
  virtual void setGrant(u32 _client, u32 _resource, bool* _grant) = 0;

  // computes the allocation logic, must be overridden
  //  should only set grants true (logically OR)
  virtual void allocate() = 0;

 protected:
  const u32 numClients_;
  const u32 numResources_;
};

#endif  // ALLOCATOR_ALLOCATOR_H_

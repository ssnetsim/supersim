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
#ifndef ARBITER_LRUARBITER_H_
#define ARBITER_LRUARBITER_H_

#include <list>
#include <string>

#include "arbiter/Arbiter.h"
#include "event/Component.h"
#include "nlohmann/json.hpp"
#include "prim/prim.h"

// least receeived used (LRU)
class LruArbiter : public Arbiter {
 public:
  LruArbiter(const std::string& _name, const Component* _parent, u32 _size,
             nlohmann::json _settings);
  ~LruArbiter();

  void latch() override;
  u32 arbitrate() override;

 private:
  std::list<u32> priority_;
  std::list<u32>::iterator lastWinner_;
};

#endif  // ARBITER_LRUARBITER_H_

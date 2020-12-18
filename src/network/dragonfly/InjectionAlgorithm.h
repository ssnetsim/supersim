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
#ifndef NETWORK_DRAGONFLY_INJECTIONALGORITHM_H_
#define NETWORK_DRAGONFLY_INJECTIONALGORITHM_H_

#include <nlohmann/json.hpp>
#include <prim/prim.h>

#include <string>

#include "event/Component.h"
#include "routing/InjectionAlgorithm.h"
#include "interface/Interface.h"

#define DRAGONFLY_INJECTIONALGORITHM_ARGS const std::string&, \
    const Component*, Interface*, u32, u32, u32, nlohmann::json

namespace Dragonfly {

class InjectionAlgorithm : public ::InjectionAlgorithm {
 public:
  InjectionAlgorithm(const std::string& _name, const Component* _parent,
                     Interface* _interface, u32 _baseVc, u32 _numVcs,
                     u32 _inputPc, nlohmann::json _settings);
  virtual ~InjectionAlgorithm();

  // this is a injection algorithm factory for the Dragonfly topology
  static InjectionAlgorithm* create(DRAGONFLY_INJECTIONALGORITHM_ARGS);
};

}  // namespace Dragonfly

#endif  // NETWORK_DRAGONFLY_INJECTIONALGORITHM_H_

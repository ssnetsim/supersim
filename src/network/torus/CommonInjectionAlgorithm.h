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
#ifndef NETWORK_TORUS_COMMONINJECTIONALGORITHM_H_
#define NETWORK_TORUS_COMMONINJECTIONALGORITHM_H_

#include <string>

#include "network/torus/InjectionAlgorithm.h"
#include "nlohmann/json.hpp"
#include "prim/prim.h"

namespace Torus {

class CommonInjectionAlgorithm : public InjectionAlgorithm {
 public:
  CommonInjectionAlgorithm(const std::string& _name, const Component* _parent,
                           Interface* _interface, u32 _baseVc, u32 _numVcs,
                           u32 _inputPc, nlohmann::json _settings);
  ~CommonInjectionAlgorithm();

  void processMessage(Message* _message) override;

 private:
  bool adaptive_;    // choose injection VC adaptively
  bool fixedMsgVc_;  // all pkts of a msg have same VC
};

}  // namespace Torus

#endif  // NETWORK_TORUS_COMMONINJECTIONALGORITHM_H_

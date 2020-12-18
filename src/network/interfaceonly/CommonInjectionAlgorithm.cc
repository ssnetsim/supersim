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
#include "network/interfaceonly/CommonInjectionAlgorithm.h"

#include <factory/ObjectFactory.h>

#include "network/common/injection.h"

namespace InterfaceOnly {

CommonInjectionAlgorithm::CommonInjectionAlgorithm(
    const std::string& _name, const Component* _parent, Interface* _interface,
    u32 _baseVc, u32 _numVcs, u32 _inputPc, nlohmann::json _settings)
    : InjectionAlgorithm(_name, _parent, _interface, _baseVc, _numVcs,
                           _inputPc, _settings) {
  assert(_settings.contains("adaptive"));
  adaptive_ = _settings["adaptive"].get<bool>();

  assert(_settings.contains("fixed_msg_vc"));
  fixedMsgVc_ = _settings["fixed_msg_vc"].get<bool>();
}

CommonInjectionAlgorithm::~CommonInjectionAlgorithm() {}

void CommonInjectionAlgorithm::processMessage(Message* _message) {
  Common::injection(interface_, this, baseVc_, numVcs_, adaptive_, fixedMsgVc_,
                    _message);
}

}  // namespace InterfaceOnly

registerWithObjectFactory(
    "common", InterfaceOnly::InjectionAlgorithm,
    InterfaceOnly::CommonInjectionAlgorithm,
    INTERFACEONLY_INJECTIONALGORITHM_ARGS);

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
#include "network/mesh/CommonInjectionAlgorithm.h"

#include <factory/ObjectFactory.h>

#include "network/common/injection.h"

namespace Mesh {

CommonInjectionAlgorithm::CommonInjectionAlgorithm(
    const std::string& _name, const Component* _parent, Interface* _interface,
    u32 _baseVc, u32 _numVcs, u32 _inputPc, Json::Value _settings)
    : InjectionAlgorithm(_name, _parent, _interface, _baseVc, _numVcs,
                           _inputPc, _settings) {
  assert(_settings.isMember("adaptive"));
  adaptive_ = _settings["adaptive"].asBool();

  assert(_settings.isMember("fixed_msg_vc"));
  fixedMsgVc_ = _settings["fixed_msg_vc"].asBool();
}

CommonInjectionAlgorithm::~CommonInjectionAlgorithm() {}

void CommonInjectionAlgorithm::processMessage(Message* _message) {
  Common::injection(interface_, this, baseVc_, numVcs_, adaptive_, fixedMsgVc_,
                    _message);
}

}  // namespace Mesh

registerWithObjectFactory(
    "common", Mesh::InjectionAlgorithm,
    Mesh::CommonInjectionAlgorithm,
    MESH_INJECTIONALGORITHM_ARGS);

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
#include "network/torus/InjectionAlgorithm.h"

#include <factory/ObjectFactory.h>

namespace Torus {

InjectionAlgorithm::InjectionAlgorithm(
    const std::string& _name, const Component* _parent, Interface* _interface,
    u32 _baseVc, u32 _numVcs, u32 _inputPc, nlohmann::json _settings)
    : ::InjectionAlgorithm(_name, _parent, _interface, _baseVc, _numVcs,
                           _inputPc, _settings) {}

InjectionAlgorithm::~InjectionAlgorithm() {}

InjectionAlgorithm* InjectionAlgorithm::create(
    const std::string& _name, const Component* _parent, Interface* _interface,
    u32 _baseVc, u32 _numVcs, u32 _inputPc, nlohmann::json _settings) {
  // retrieve the algorithm
  const std::string& algorithm = _settings["algorithm"].get<std::string>();

  // attempt to create the injection algorithm
  InjectionAlgorithm* ia = factory::ObjectFactory<
    InjectionAlgorithm, TORUS_INJECTIONALGORITHM_ARGS>::create(
        algorithm, _name, _parent, _interface, _baseVc, _numVcs, _inputPc,
        _settings);

  // check that the factory had this type
  if (ia == nullptr) {
    fprintf(stderr, "invalid Torus injection algorithm: %s\n",
            algorithm.c_str());
    assert(false);
  }
  return ia;
}

}  // namespace Torus

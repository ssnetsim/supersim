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
#include "routing/InjectionAlgorithm.h"

#include <cassert>

#include "event/Simulator.h"
#include "interface/Interface.h"

InjectionAlgorithm::InjectionAlgorithm(
    const std::string& _name, const Component* _parent, Interface* _interface,
    u32 _baseVc, u32 _numVcs, u32 _inputPc, Json::Value _settings)
    : Component(_name, _parent), interface_(_interface), baseVc_(_baseVc),
      numVcs_(_numVcs), inputPc_(_inputPc) {
  assert(interface_ != nullptr);
  assert(numVcs_ <= interface_->numVcs());
  assert(baseVc_ <= interface_->numVcs() - numVcs_);
}

InjectionAlgorithm::~InjectionAlgorithm() {}

u32 InjectionAlgorithm::baseVc() const {
  return baseVc_;
}

u32 InjectionAlgorithm::numVcs() const {
  return numVcs_;
}

u32 InjectionAlgorithm::inputPc() const {
  return inputPc_;
}

void InjectionAlgorithm::injectPacket(Packet* _packet, u32 _port, u32 _vc) {
  assert(_port < interface_->numPorts());
  assert(_vc >= baseVc_);
  assert(_vc < baseVc_ + numVcs_);

  // informs the interface that the packet is being injected
  interface_->injectingPacket(_packet, _port, _vc);
}

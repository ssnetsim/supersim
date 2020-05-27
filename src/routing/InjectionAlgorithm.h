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
#ifndef ROUTING_INJECTIONALGORITHM_H_
#define ROUTING_INJECTIONALGORITHM_H_

#include <prim/prim.h>
#include <json/json.h>

#include <string>
#include <utility>
#include <vector>

#include "event/Component.h"
#include "types/Message.h"
#include "types/Packet.h"

class Interface;

#define INJECTIONALGORITHM_ARGS const std::string&, const Component*, \
    Interface*, u32, u32, u32, Json::Value

class InjectionAlgorithm : public Component {
 public:
  /*
   * This defines the InjectionAlgorithm interface. Specific implementations
   *  must override the processMessage() function.
   */
  InjectionAlgorithm(
      const std::string& _name, const Component* _parent, Interface* _interface,
      u32 _baseVc, u32 _numVcs, u32 _inputPc, Json::Value _settings);
  virtual ~InjectionAlgorithm();
  u32 baseVc() const;
  u32 numVcs() const;
  u32 inputPc() const;
  virtual void processMessage(Message* _message) = 0;

  // use this to set the port and VC for each packet
  // this should be called by subclasses or utility functions
  void injectPacket(Packet* _packet, u32 _port, u32 _vc);

 protected:
  Interface* interface_;
  const u32 baseVc_;
  const u32 numVcs_;
  const u32 inputPc_;
};

#endif  // ROUTING_INJECTIONALGORITHM_H_

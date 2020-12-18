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
#include "interface/Interface.h"

#include <factory/ObjectFactory.h>

#include <cassert>

Interface::Interface(
    const std::string& _name, const Component* _parent, Network* _network,
    u32 _id, const std::vector<u32>& _address, u32 _numPorts, u32 _numVcs,
    MetadataHandler* _metadataHandler, nlohmann::json _settings)
    : Component(_name, _parent),
      PortedDevice(_id, _address, _numPorts, _numVcs), network_(_network),
      messageReceiver_(nullptr), metadataHandler_(_metadataHandler) {}

Interface::~Interface() {}

Interface* Interface::create(
    const std::string& _name, const Component* _parent, Network* _network,
    u32 _id, const std::vector<u32>& _address, u32 _numPorts, u32 _numVcs,
    MetadataHandler* _metadataHandler, nlohmann::json _settings) {
  // retrieve the type
  const std::string& type = _settings["type"].get<std::string>();

  // attempt to build the interface
  Interface* interface = factory::ObjectFactory<
    Interface, INTERFACE_ARGS>::create(
        type, _name, _parent, _network, _id, _address, _numPorts, _numVcs,
        _metadataHandler, _settings);

  // check that the factory had this type
  if (interface == nullptr) {
    fprintf(stderr, "unknown interface type: %s\n", type.c_str());
    assert(false);
  }
  return interface;
}

void Interface::setMessageReceiver(MessageReceiver* _receiver) {
  assert(messageReceiver_ == nullptr);
  messageReceiver_ = _receiver;
}

MessageReceiver* Interface::messageReceiver() const {
  return messageReceiver_;
}

void Interface::packetArrival(Packet* _packet) const {
  metadataHandler_->packetInterfaceArrival(this, _packet);
}

void Interface::packetDeparture(Packet* _packet) const {
  metadataHandler_->packetInterfaceDeparture(this, _packet);
}

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
#ifndef INTERFACE_INTERFACE_H_
#define INTERFACE_INTERFACE_H_

#include <nlohmann/json.hpp>
#include <prim/prim.h>

#include <string>
#include <tuple>
#include <vector>

#include "architecture/PortedDevice.h"
#include "event/Component.h"
#include "metadata/MetadataHandler.h"
#include "types/CreditReceiver.h"
#include "types/CreditSender.h"
#include "types/FlitReceiver.h"
#include "types/FlitSender.h"
#include "types/Message.h"
#include "types/MessageReceiver.h"
#include "network/Channel.h"

class PacketReassembler;
class MessageReassembler;

#define INTERFACE_ARGS const std::string&, const Component*, Network*, u32,  \
    const std::vector<u32>&, u32, u32, MetadataHandler*, nlohmann::json

class Interface : public Component, public PortedDevice, public FlitSender,
                  public FlitReceiver, public CreditSender,
                  public CreditReceiver, public MessageReceiver {
 public:
  Interface(const std::string& _name, const Component* _parent,
            Network* _network, u32 _id, const std::vector<u32>& _address,
            u32 _numPorts, u32 _numVcs, MetadataHandler* _metadataHandler,
            nlohmann::json _settings);
  virtual ~Interface();

  // this is an interface factory
  static Interface* create(INTERFACE_ARGS);

  void setMessageReceiver(MessageReceiver* _receiver);
  MessageReceiver* messageReceiver() const;

  // this must be called by all subclasses when a packet's head flit arrives
  //  on an input from a terminal.
  void packetArrival(Packet* _packet) const;

  // this must be called by all subclasses when a packet's head flit departs
  //  on an output port.
  void packetDeparture(Packet* _packet) const;

  // this is called by an InjectionAlgorithm after it decides to inject a packet
  // on a particular port and VC.
  virtual void injectingPacket(Packet* _packet, u32 _port, u32 _vc) = 0;

  // this must be implemented so that an InjectionAlgorithm can ask about the
  // occupancy level of particular output VCs.
  virtual u32 occupancy(u32 _port, u32 _vc) const = 0;

 protected:
  Network* network_;

 private:
  MessageReceiver* messageReceiver_;
  std::vector<PacketReassembler*> packetReassemblers_;
  MessageReassembler* messageReassembler_;
  MetadataHandler* metadataHandler_;
};

#endif  // INTERFACE_INTERFACE_H_

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
#ifndef INTERFACE_STANDARD_INTERFACE_H_
#define INTERFACE_STANDARD_INTERFACE_H_

#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "architecture/Crossbar.h"
#include "architecture/CrossbarScheduler.h"
#include "interface/Interface.h"
#include "network/Channel.h"
#include "nlohmann/json.hpp"
#include "prim/prim.h"
#include "routing/InjectionAlgorithm.h"
#include "types/Credit.h"
#include "types/CreditReceiver.h"
#include "types/Flit.h"
#include "types/FlitReceiver.h"
#include "types/Message.h"
#include "types/MessageReceiver.h"
#include "types/Packet.h"

// architectural drawing here: https://bit.ly/2LK3uF4

namespace Standard {

class OutputQueue;
class Ejector;
class PacketReassembler;
class MessageReassembler;

class Interface : public ::Interface {
 public:
  Interface(const std::string& _name, const Component* _parent,
            Network* _network, u32 _id, const std::vector<u32>& _address,
            u32 _numPorts, u32 _numVcs, MetadataHandler* _metadataHandler,
            nlohmann::json _settings);
  ~Interface();

  void setInputChannel(u32 _port, Channel* _channel) override;
  Channel* getInputChannel(u32 _port) const override;
  void setOutputChannel(u32 _port, Channel* _channel) override;
  Channel* getOutputChannel(u32 _port) const override;

  void initialize() override;

  void receiveMessage(Message* _message) override;

  void injectingPacket(Packet* _packet, u32 _port, u32 _vc) override;
  u32 occupancy(u32 _port, u32 _vc) const override;

  void sendFlit(u32 _port, Flit* _flit) override;
  void receiveFlit(u32 _port, Flit* _flit) override;
  void sendCredit(u32 _port, u32 _vc) override;
  void receiveCredit(u32 _port, Credit* _credit) override;

  void incrementCredit(u32 _port, u32 _vc);

  void processEvent(void* _event, s32 _type) override;

 private:
  void injectMessage(Message* _message);

  std::vector<Channel*> inputChannels_;
  std::vector<Channel*> outputChannels_;

  u32 initCredits_;
  // input queue tailoring
  bool inputQueueTailored_;
  f64 inputQueueMult_;
  u32 inputQueueMax_;
  u32 inputQueueMin_;

  std::vector<InjectionAlgorithm*> injectionAlgorithms_;
  std::vector<OutputQueue*> outputQueues_;
  std::vector<u32> queueOccupancy_;  // used for adaptive injection
  std::vector<Crossbar*> crossbars_;
  std::vector<CrossbarScheduler*> crossbarSchedulers_;
  std::vector<Ejector*> ejectors_;

  std::vector<PacketReassembler*> packetReassemblers_;
  MessageReassembler* messageReassembler_;

  // this holds temporary injection information
  std::unordered_map<Packet*, std::tuple<u32, u32> > injectionInfo_;
};

}  // namespace Standard

#endif  // INTERFACE_STANDARD_INTERFACE_H_

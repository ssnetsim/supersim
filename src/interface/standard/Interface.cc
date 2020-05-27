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
#include "interface/standard/Interface.h"

#include <factory/ObjectFactory.h>

#include <cassert>

#include "architecture/util.h"
#include "workload/Application.h"
#include "interface/standard/Ejector.h"
#include "interface/standard/MessageReassembler.h"
#include "interface/standard/OutputQueue.h"
#include "interface/standard/PacketReassembler.h"
#include "network/Network.h"
#include "types/MessageOwner.h"

// event types
#define INJECT_MESSAGE (0x45)

namespace Standard {

Interface::Interface(
    const std::string& _name, const Component* _parent, Network* _network,
    u32 _id, const std::vector<u32>& _address, u32 _numPorts, u32 _numVcs,
    MetadataHandler* _metadataHandler, Json::Value _settings)
    : ::Interface(_name, _parent, _network, _id, _address, _numPorts, _numVcs,
                  _metadataHandler, _settings) {
  // init credits
  initCredits_ = 0;
  inputQueueTailored_ = false;
  inputQueueMult_ = 0;
  inputQueueMax_ = 0;
  inputQueueMin_ = 0;
  assert(_settings.isMember("init_credits_mode"));

  if (_settings["init_credits_mode"].asString() == "tailored") {
    inputQueueTailored_ = true;
    inputQueueMult_ = _settings["init_credits"].asDouble();
    assert(inputQueueMult_ > 0.0);
    // max and min queue depth
    assert(_settings.isMember("credits_min"));
    inputQueueMin_ = _settings["credits_min"].asUInt();
    assert(_settings.isMember("credits_max"));
    inputQueueMax_ = _settings["credits_max"].asUInt();
    assert(inputQueueMin_ <= inputQueueMax_);
  } else if (_settings["init_credits_mode"].asString() == "fixed") {
    inputQueueTailored_ = false;
    initCredits_ = _settings["init_credits"].asUInt();
    assert(initCredits_ > 0);
  } else {
    fprintf(stderr, "Wrong init credits mode, options: tailor or fixed\n");
    assert(false);
  }

  // create the injection algorithm
  u32 numPcs = network_->numPcs();
  injectionAlgorithms_.resize(numPcs, nullptr);
  for (u32 pc = 0; pc < numPcs; pc++) {
    std::string injName = "InjectionAlgorithm_" + std::to_string(pc);
    injectionAlgorithms_.at(pc) = network_->createInjectionAlgorithm(
        pc, injName, this, this);
  }

  // create the queues, crossbars, schedulers, ejectors, and packet reassemblers
  crossbars_.resize(numPorts_, nullptr);
  crossbarSchedulers_.resize(numPorts_, nullptr);
  outputQueues_.resize(numPorts_ * numVcs_, nullptr);
  queueOccupancy_.resize(numPorts_ * numVcs_, 0);
  ejectors_.resize(numPorts_, nullptr);
  packetReassemblers_.resize(numPorts_ * numVcs_, nullptr);
  for (u32 port = 0; port < numPorts_; port++) {
    // scheduler
    std::string crossbarSchedulerName =
        "CrossbarScheduler_" + std::to_string(port);
    crossbarSchedulers_.at(port) = new CrossbarScheduler(
        crossbarSchedulerName, this, numVcs_, numVcs_, 1, port * numVcs_,
        Simulator::Clock::CHANNEL, _settings["crossbar_scheduler"]);

    // crossbar
    std::string crossbarName = "Crossbar_" + std::to_string(port);
    crossbars_.at(port) = new Crossbar(
        crossbarName, this, numVcs_, 1, Simulator::Clock::CHANNEL,
        _settings["crossbar"]);

    // ejector
    std::string ejectorName = "Ejector_" + std::to_string(port);
    ejectors_.at(port) = new Ejector(ejectorName, this, port);
    crossbars_.at(port)->setReceiver(0, ejectors_.at(port), 0);

    // per VC output queues
    for (u32 vc = 0; vc < numVcs_; vc++) {
      u32 vcIdx = vcIndex(port, vc);

      // queue
      std::string queueName = "OutputQueue_" + std::to_string(port) + "_" +
                              std::to_string(vc);
      outputQueues_.at(vcIdx) = new OutputQueue(
          queueName, this, crossbarSchedulers_.at(port), vc,
          crossbars_.at(port), vc, port, vc);

      // link queue to scheduler
      crossbarSchedulers_.at(port)->setClient(vc, outputQueues_.at(vcIdx));

      // packet reassembler
      std::string packetReassemblerName =
          "PacketReassembler_" + std::to_string(port) + "_" +
          std::to_string(vc);
      packetReassemblers_.at(vcIdx) = new PacketReassembler(
          packetReassemblerName, this);
    }
  }

  // create message reassembler
  messageReassembler_ = new MessageReassembler("MessageReassembler", this);

  // allocate slots for I/O channels
  inputChannels_.resize(numPorts_, nullptr);
  outputChannels_.resize(numPorts_, nullptr);
}

Interface::~Interface() {
  for (u32 pc = 0; pc < network_->numPcs(); pc++) {
    delete injectionAlgorithms_.at(pc);
  }
  for (u32 port = 0; port < numPorts_; port++) {
    delete ejectors_.at(port);
    delete crossbarSchedulers_.at(port);
    delete crossbars_.at(port);
    for (u32 vc = 0; vc < numVcs_; vc++) {
      u32 vcIdx = vcIndex(port, vc);
      delete outputQueues_.at(vcIdx);
      delete packetReassemblers_.at(vcIdx);
    }
  }
  delete messageReassembler_;
}

void Interface::setInputChannel(u32 _port, Channel* _channel) {
  assert(inputChannels_.at(_port) == nullptr);
  inputChannels_.at(_port) = _channel;
  _channel->setSink(this, _port);
}

Channel* Interface::getInputChannel(u32 _port) const {
  assert(_port < numPorts_);
  return inputChannels_.at(_port);
}

void Interface::setOutputChannel(u32 _port, Channel* _channel) {
  assert(outputChannels_.at(_port) == nullptr);
  outputChannels_.at(_port) = _channel;
  _channel->setSource(this, _port);
}

Channel* Interface::getOutputChannel(u32 _port) const {
  assert(_port < numPorts_);
  return outputChannels_.at(_port);
}

void Interface::initialize() {
  // init credits
  for (u32 ch = 0; ch < outputChannels_.size(); ch++) {
    assert(outputChannels_.at(ch)->latency());
    // compute tailored queue depth for donwstream channel
    u32 credits = initCredits_;
    u32 channelLatency = outputChannels_.at(ch)->latency();
    if (inputQueueTailored_) {
      credits = computeTailoredBufferLength(
          inputQueueMult_, inputQueueMin_, inputQueueMax_, channelLatency);
    }
    for (u32 vc = 0; vc < numVcs_; vc++) {
      // initialize the credit count in the CrossbarScheduler
      crossbarSchedulers_.at(ch)->initCredits(vc, credits);
    }
  }
}

void Interface::receiveMessage(Message* _message) {
  assert(_message != nullptr);
  assert(gSim->epsilon() == 0);

  u64 now = gSim->time();

  // mark all flit send times
  for (u32 p = 0; p < _message->numPackets(); p++) {
    Packet* packet = _message->packet(p);
    packetArrival(packet);  // inform the base class of arrival
    for (u32 f = 0; f < packet->numFlits(); f++) {
      Flit* flit = packet->getFlit(f);
      flit->setSendTime(now);
    }
  }

  // retrieve the protocol class of the message
  u32 pc = _message->getProtocolClass();
  assert(pc < network_->numPcs());

  // process the message
  InjectionAlgorithm* inj = injectionAlgorithms_.at(pc);
  inj->processMessage(_message);

  // create an event to inject the message into the queues
  assert(gSim->epsilon() == 0);
  addEvent(gSim->time(), 1, _message, INJECT_MESSAGE);
}

void Interface::injectingPacket(Packet* _packet, u32 _port, u32 _vc) {
  // update credit counts
  u32 vcIdx = vcIndex(_port, _vc);
  assert(queueOccupancy_.at(vcIdx) < U32_MAX - _packet->numFlits());
  dbgprintf("INJ (%u,%u) %u+%u=%u", _port, _vc, queueOccupancy_.at(vcIdx),
            _packet->numFlits(),
            queueOccupancy_.at(vcIdx) + _packet->numFlits());
  queueOccupancy_.at(vcIdx) += _packet->numFlits();

  // store the information about the injection decision
  assert(injectionInfo_.find(_packet) == injectionInfo_.end());
  injectionInfo_.insert(std::make_pair(_packet, std::make_tuple(_port, _vc)));
}

u32 Interface::occupancy(u32 _port, u32 _vc) const {
  u32 vcIdx = vcIndex(_port, _vc);
  return queueOccupancy_.at(vcIdx);
}

void Interface::sendFlit(u32 _port, Flit* _flit) {
  assert(_port < numPorts_);
  assert(outputChannels_.at(_port)->getNextFlit() == nullptr);
  outputChannels_.at(_port)->setNextFlit(_flit);

  // inform the base class of departure
  if (_flit->isHead()) {
    packetDeparture(_flit->packet());
  }

  // check source is correct
  u32 src = _flit->packet()->message()->getSourceId();
  assert(src == id_);
}

void Interface::receiveFlit(u32 _port, Flit* _flit) {
  assert(_port < numPorts_);
  assert(_flit != nullptr);

  // send a credit back
  sendCredit(_port, _flit->getVc());

  // check destination is correct
  u32 dest = _flit->packet()->message()->getDestinationId();
  assert(dest == id_);

  // mark the receive time
  _flit->setReceiveTime(gSim->time());

  // process flit, attempt to create packet
  u32 vcIdx = vcIndex(_port, _flit->getVc());
  Packet* packet = packetReassemblers_.at(vcIdx)->receiveFlit(_flit);
  // if a packet was completed, process it
  if (packet) {
    // process packet, attempt to create message
    Message* message = messageReassembler_->receivePacket(packet);
    if (message) {
      messageReceiver()->receiveMessage(message);
    }
  }
}

void Interface::sendCredit(u32 _port, u32 _vc) {
  assert(_port < numPorts_);
  assert(_vc < numVcs_);

  // send credit
  Credit* credit = inputChannels_.at(_port)->getNextCredit();
  if (credit == nullptr) {
    credit = new Credit(numVcs_);
    inputChannels_.at(_port)->setNextCredit(credit);
  }
  credit->putNum(_vc);
}

void Interface::receiveCredit(u32 _port, Credit* _credit) {
  assert(_port < numPorts_);
  while (_credit->more()) {
    u32 vc = _credit->getNum();
    crossbarSchedulers_.at(_port)->incrementCredit(vc);
  }
  delete _credit;
}

void Interface::incrementCredit(u32 _port, u32 _vc) {
  assert(_port < numPorts_);
  assert(_vc < numVcs_);
  u32 vcIdx = vcIndex(_port, _vc);
  dbgprintf("DEC (%u,%u) %u-1=%u", _port, _vc, queueOccupancy_.at(vcIdx), 1,
            queueOccupancy_.at(vcIdx) - 1);
  assert(queueOccupancy_.at(vcIdx) > 0);
  queueOccupancy_.at(vcIdx)--;
}

void Interface::processEvent(void* _event, s32 _type) {
  switch (_type) {
    case INJECT_MESSAGE:
      assert(gSim->epsilon() == 1);
      injectMessage(reinterpret_cast<Message*>(_event));
      break;

    default:
      assert(false);
  }
}

void Interface::injectMessage(Message* _message) {
  for (u32 p = 0; p < _message->numPackets(); p++) {
    Packet* packet = _message->packet(p);

    // lookup the port and VC information for this packet
    u32 port = std::get<0>(injectionInfo_.at(packet));
    u32 vc = std::get<1>(injectionInfo_.at(packet));
    u32 vcIdx = vcIndex(port, vc);
    injectionInfo_.erase(packet);

    // apply VC to each flit in the packet
    u32 flits = packet->numFlits();
    for (u32 f = 0; f < flits; f++) {
      Flit* flit = packet->getFlit(f);
      flit->setVc(vc);
    }

    // inject the flits of the packet
    dbgprintf("injecting into %u,%u", port, vc);
    for (u32 f = 0; f < flits; f++) {
      Flit* flit = packet->getFlit(f);
      outputQueues_.at(vcIdx)->receiveFlit(0, flit);  // single port queues
    }
  }
}

}  // namespace Standard

registerWithObjectFactory("standard", ::Interface,
                          Standard::Interface, INTERFACE_ARGS);

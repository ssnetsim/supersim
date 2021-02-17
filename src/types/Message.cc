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
#include "types/Message.h"

#include <cassert>

#include "types/Packet.h"
#include "workload/Terminal.h"

Message::Message(u32 _numPackets, void* _data)
    : data_(_data),
      transaction_(U32_MAX),
      protocolClass_(U32_MAX),
      sourceId_(U32_MAX),
      destinationId_(U32_MAX) {
  packets_.resize(_numPackets);
}

Message::~Message() {
  for (u32 p = 0; p < packets_.size(); p++) {
    if (packets_.at(p)) {
      delete packets_.at(p);
    }
  }
}

Terminal* Message::getOwner() const {
  return owner_;
}

void Message::setOwner(Terminal* _owner) {
  owner_ = _owner;
}

u32 Message::id() const {
  return id_;
}

void Message::setId(u32 _id) {
  id_ = _id;
}

u32 Message::numPackets() const {
  return packets_.size();
}

u32 Message::numFlits() const {
  u32 numFlits = 0;
  for (u32 p = 0; p < packets_.size(); p++) {
    numFlits += packets_.at(p)->numFlits();
  }
  return numFlits;
}

Packet* Message::packet(u32 _index) const {
  return packets_.at(_index);
}

void Message::setPacket(u32 _index, Packet* _packet) {
  packets_.at(_index) = _packet;
}

void* Message::getData() const {
  return data_;
}

void Message::setData(void* _data) {
  data_ = _data;
}

u64 Message::getTransaction() const {
  return transaction_;
}

void Message::setTransaction(u64 _trans) {
  transaction_ = _trans;
}

u32 Message::getProtocolClass() const {
  return protocolClass_;
}

void Message::setProtocolClass(u32 _class) {
  protocolClass_ = _class;
}

u32 Message::getOpCode() const {
  return opCode_;
}

void Message::setOpCode(u32 _opCode) {
  opCode_ = _opCode;
}

u32 Message::getSourceId() const {
  return sourceId_;
}

void Message::setSourceId(u32 _sourceId) {
  sourceId_ = _sourceId;
}

u32 Message::getDestinationId() const {
  return destinationId_;
}

void Message::setDestinationId(u32 _destinationId) {
  destinationId_ = _destinationId;
}

u32 Message::getMinimalHopCount() const {
  return minimalHopCount_;
}

void Message::setMinimalHopCount(u32 _minimalHopCount) {
  minimalHopCount_ = _minimalHopCount;
}

void Message::setSourceAddress(const std::vector<u32>* _address) {
  sourceAddress_ = _address;
}

const std::vector<u32>* Message::getSourceAddress() const {
  return sourceAddress_;
}

void Message::setDestinationAddress(const std::vector<u32>* _address) {
  destinationAddress_ = _address;
}

const std::vector<u32>* Message::getDestinationAddress() const {
  return destinationAddress_;
}

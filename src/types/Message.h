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
#ifndef TYPES_MESSAGE_H_
#define TYPES_MESSAGE_H_

#include <vector>

#include "prim/prim.h"

class Packet;
class Terminal;

class Message {
 public:
  Message(u32 _numPackets, void* _data);

  // this deletes all packet data as well (as long as packets aren't nullptr)
  virtual ~Message();

  Terminal* getOwner() const;
  void setOwner(Terminal* _owner);

  u32 id() const;
  void setId(u32 _id);

  u32 numPackets() const;
  u32 numFlits() const;
  Packet* packet(u32 _index) const;
  void setPacket(u32 _index, Packet* _packet);

  void* getData() const;
  void setData(void* _data);

  u64 getTransaction() const;
  void setTransaction(u64 _trans);

  u32 getProtocolClass() const;
  void setProtocolClass(u32 _class);

  u32 getOpCode() const;
  void setOpCode(u32 _opCode);

  u32 getSourceId() const;
  void setSourceId(u32 _sourceId);

  u32 getDestinationId() const;
  void setDestinationId(u32 _destinationId);

  u32 getMinimalHopCount() const;
  void setMinimalHopCount(u32 _minimalHopCount);

  void setSourceAddress(const std::vector<u32>* _address);
  const std::vector<u32>* getSourceAddress() const;

  void setDestinationAddress(const std::vector<u32>* _address);
  const std::vector<u32>* getDestinationAddress() const;

 private:
  Terminal* owner_;
  u32 id_;
  std::vector<Packet*> packets_;
  void* data_;
  u64 transaction_;
  u32 protocolClass_;
  u32 opCode_;

  u32 sourceId_;
  u32 destinationId_;
  u32 minimalHopCount_;

  const std::vector<u32>* sourceAddress_;
  const std::vector<u32>* destinationAddress_;
};

#endif  // TYPES_MESSAGE_H_

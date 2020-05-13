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
#include "workload/pulse/PulseTerminal.h"

#include <fio/InFile.h>
#include <mut/mut.h>
#include <strop/strop.h>

#include <cassert>
#include <cmath>

#include <algorithm>
#include <utility>

#include "network/Network.h"
#include "stats/MessageLog.h"
#include "types/Flit.h"
#include "types/Packet.h"
#include "workload/pulse/Application.h"
#include "workload/util.h"

// these are event types
#define kRequestEvt (0xFA)
#define kResponseEvt (0x82)

// this app defines the following message OpCodes
static const u32 kRequestMsg = kRequestEvt;
static const u32 kResponseMsg = kResponseEvt;

namespace Pulse {

PulseTerminal::PulseTerminal(const std::string& _name, const Component* _parent,
                             u32 _id, const std::vector<u32>& _address,
                             ::Application* _app, Json::Value _settings)
    : ::Terminal(_name, _parent, _id, _address, _app) {
  // get the injection rate
  assert(_settings.isMember("request_injection_rate") &&
         _settings["request_injection_rate"].isDouble());
  requestInjectionRate_ = _settings["request_injection_rate"].asDouble();
  assert(requestInjectionRate_ >= 0.0 && requestInjectionRate_ <= 1.0);

  // if relative injection is specified, modify the injection accordingly
  if (_settings.isMember("relative_injection")) {
    // if a file is given, it is a csv of injection rates
    fio::InFile inf(_settings["relative_injection"].asString());
    std::string line;
    u32 lineNum = 0;
    fio::InFile::Status sts = fio::InFile::Status::OK;
    bool foundMe = false;
    for (lineNum = 0; sts == fio::InFile::Status::OK;) {
      sts = inf.getLine(&line);
      assert(sts != fio::InFile::Status::ERROR);
      if (sts == fio::InFile::Status::OK) {
        if (line.size() > 0) {
          std::vector<std::string> strs = strop::split(line, ',');
          assert(strs.size() == 1);
          f64 ri = std::stod(strs.at(0));
          assert(ri >= 0.0);
          if (lineNum == id_) {
            requestInjectionRate_ *= ri;
            foundMe = true;
            break;
          }
          lineNum++;
        }
      }
    }
    assert(foundMe);
  }

  // transaction quantity limitation
  assert(_settings.isMember("num_transactions"));
  numTransactions_ = _settings["num_transactions"].asUInt();

  // max packet size
  maxPacketSize_  = _settings["max_packet_size"].asUInt();
  assert(maxPacketSize_ > 0);

  // transaction size
  transactionSize_ = _settings["transaction_size"].asUInt();
  assert(transactionSize_ > 0);

  // multiple destinations within transactions
  multiDestinationTransactions_ = false;
  if (transactionSize_ > 1) {
    assert(_settings.isMember("multi_destination_transactions"));
    assert(_settings["multi_destination_transactions"].isBool());
    multiDestinationTransactions_ =
        _settings["multi_destination_transactions"].asBool();
  }

  // create a traffic pattern
  trafficPattern_ = ContinuousTrafficPattern::create(
      "TrafficPattern", this, application()->numTerminals(), id_,
      _settings["traffic_pattern"]);

  // create a message size distribution
  messageSizeDistribution_ = MessageSizeDistribution::create(
      "MessageSizeDistribution", this, _settings["message_size_distribution"]);

  // protocol class of injection of requests
  assert(_settings.isMember("request_protocol_class"));
  requestProtocolClass_ = _settings["request_protocol_class"].asUInt();

  // enablement of request/response flows
  assert(_settings.isMember("enable_responses") &&
         _settings["enable_responses"].isBool());
  enableResponses_ = _settings["enable_responses"].asBool();

  // latency of request processing
  assert(!enableResponses_ ||
         _settings.isMember("request_processing_latency"));
  requestProcessingLatency_ = _settings["request_processing_latency"].asUInt();

  // protocol class of injection of responses
  assert(!enableResponses_ || _settings.isMember("response_protocol_class"));
  responseProtocolClass_ = _settings["response_protocol_class"].asUInt();

  // start time delay
  assert(_settings.isMember("delay"));
  delay_ = _settings["delay"].asUInt();

  // initialize the counters
  transactionsSent_ = 0;
  loggableCompleteCount_ = 0;
}

PulseTerminal::~PulseTerminal() {
  assert(outstandingTransactions_.size() == 0);

  delete trafficPattern_;
  delete messageSizeDistribution_;
}

void PulseTerminal::processEvent(void* _event, s32 _type) {
  switch (_type) {
    case kRequestEvt:
      assert(_event == nullptr);
      startTransaction();
      break;

    case kResponseEvt:
      sendResponse(reinterpret_cast<Message*>(_event));
      break;

    default:
      assert(false);
      break;
  }
}

f64 PulseTerminal::percentComplete() const {
  if (numTransactions_ == 0) {
    return 1.0;
  } else {
    u32 count = std::min(loggableCompleteCount_, numTransactions_);
    return (f64)count / (f64)numTransactions_;
  }
}

f64 PulseTerminal::requestInjectionRate() const {
  return requestInjectionRate_;
}

void PulseTerminal::start() {
  Application* app = reinterpret_cast<Application*>(application());

  if (numTransactions_ == 0) {
    dbgprintf("complete");
    app->terminalComplete(id_);
  } else {
    // choose a random number of cycles in the future to start
    // make an event to start the PulseTerminal in the future
    if (requestInjectionRate_ > 0.0) {
      u32 maxMsg = messageSizeDistribution_->maxMessageSize();
      u32 maxTrans = maxMsg * transactionSize_;
      u64 cycles = cyclesToSend(requestInjectionRate_, maxTrans);
      cycles = gSim->rnd.nextU64(delay_, delay_ + cycles * 3);
      u64 time = gSim->futureCycle(Simulator::Clock::CHANNEL, 1) +
                 ((cycles - 1) * gSim->cycleTime(Simulator::Clock::CHANNEL));
      dbgprintf("start time is %lu", time);
      addEvent(time, 0, nullptr, kRequestEvt);
    } else {
      dbgprintf("not running");
    }
  }
}

void PulseTerminal::handleDeliveredMessage(Message* _message) {
  // handle request only transaction tracking
  u32 msgType = _message->getOpCode();
  u64 transId = _message->getTransaction();
  if (msgType == kRequestMsg) {
    // complete transaction, determine if last
    bool lastOfTrans = false;
    if (!enableResponses_) {
      lastOfTrans = completeTracking(transId);
    }

    // log message if tagged
    if (transactionsToLog_.count(transId) == 1) {
      Application* app = reinterpret_cast<Application*>(application());
      app->workload()->messageLog()->logMessage(_message);

      // end this transaction in the log if appropriate
      if (!enableResponses_ && lastOfTrans) {
        completeLoggable(transId);
      }
    }
  }
}

void PulseTerminal::handleReceivedMessage(Message* _message) {
  Application* app = reinterpret_cast<Application*>(application());
  u32 msgType = _message->getOpCode();
  u64 transId = _message->getTransaction();

  // handle request/response transaction tracking
  if (msgType == kResponseMsg) {
    assert(enableResponses_);

    // complete the tracking of this transaction
    bool lastOfTrans = completeTracking(transId);

    // log message if tagged
    if (transactionsToLog_.count(transId) == 1) {
      // log the message
      app->workload()->messageLog()->logMessage(_message);

      // end this transaction in the log if this is the last message
      if (lastOfTrans) {
        completeLoggable(transId);
      }
    }
  }

  if (enableResponses_ && msgType == kRequestMsg) {
    // signal for requests to generate responses when responses are enabled
    // register an event to process the request
    if (requestProcessingLatency_ == 0) {
      sendResponse(_message);
    } else {
      u64 respTime = gSim->futureCycle(Simulator::Clock::CHANNEL,
                                       requestProcessingLatency_);
      addEvent(respTime, 0, _message, kResponseEvt);
    }
  }

  // delete the message if no longer needed
  if ((!enableResponses_ && msgType == kRequestMsg) ||
      (msgType == kResponseMsg)) {
    delete _message;
  }
}

bool PulseTerminal::completeTracking(u64 _transId) {
  // decrement the counter for this transaction
  assert(outstandingTransactions_.at(_transId) > 0);
  outstandingTransactions_.at(_transId)--;

  // if this is the last expected message, end tracking of this transaction,
  // and end the transaction
  if (outstandingTransactions_.at(_transId) == 0) {
    u32 res = outstandingTransactions_.erase(_transId);
    assert(res == 1);

    // end the transaction
    endTransaction(_transId);
    return true;
  }
  return false;
}

void PulseTerminal::completeLoggable(u64 _transId) {
  // clear the logging entry
  assert(outstandingTransactions_.find(_transId) ==
         outstandingTransactions_.end());
  u64 res = transactionsToLog_.erase(_transId);
  assert(res == 1);

  // log the message/transaction
  Application* app = reinterpret_cast<Application*>(application());
  app->workload()->messageLog()->endTransaction(_transId);
  loggableCompleteCount_++;

  // detect when logging complete
  //  this also completes the terminal
  if (loggableCompleteCount_ == numTransactions_) {
    dbgprintf("complete");
    app->terminalComplete(id_);
  }
}

void PulseTerminal::startTransaction() {
  Application* app = reinterpret_cast<Application*>(application());

  // start a new transaction
  u32 protocolClass = requestProtocolClass_;
  u64 transaction = createTransaction();
  u32 msgType = kRequestMsg;

  // start tracking the transaction
  bool res = outstandingTransactions_.insert(
      std::make_pair(transaction, transactionSize_)).second;
  assert(res);

  // register the transaction for logging
  bool res2 = transactionsToLog_.insert(transaction).second;
  assert(res2);
  app->workload()->messageLog()->startTransaction(transaction);

  // create N requests for this transaction
  u32 destination = U32_MAX;
  u32 messageSize = U32_MAX;
  for (u32 req = 0; req < transactionSize_; req++) {
    // the destination and message size either stay the same or are varied with
    // each request of the transaction based on multiDestinationTransactions_.
    if (destination == U32_MAX || multiDestinationTransactions_) {
      destination = trafficPattern_->nextDestination();
      assert(destination != U32_MAX);
      messageSize = messageSizeDistribution_->nextMessageSize();
    }

    // determine the number of packets
    u32 numPackets = messageSize / maxPacketSize_;
    if ((messageSize % maxPacketSize_) > 0) {
      numPackets++;
    }

    // create the message object
    Message* message = new Message(numPackets, nullptr);
    message->setProtocolClass(protocolClass);
    message->setTransaction(transaction);
    message->setOpCode(msgType);

    // create the packets
    u32 flitsLeft = messageSize;
    for (u32 p = 0; p < numPackets; p++) {
      u32 packetLength = flitsLeft > maxPacketSize_ ?
                         maxPacketSize_ : flitsLeft;

      Packet* packet = new Packet(p, packetLength, message);
      message->setPacket(p, packet);

      // create flits
      for (u32 f = 0; f < packetLength; f++) {
        bool headFlit = f == 0;
        bool tailFlit = f == (packetLength - 1);
        Flit* flit = new Flit(f, headFlit, tailFlit, packet);
        packet->setFlit(f, flit);
      }
      flitsLeft -= packetLength;
    }

    // send the message
    u32 msgId = sendMessage(message, destination);
    (void)msgId;  // unused
  }

  // determine when to send the next transaction
  transactionsSent_++;
  if (transactionsSent_ < numTransactions_) {
    u64 transSize = messageSize * transactionSize_;
    u64 cycles = cyclesToSend(requestInjectionRate_, transSize);
    u64 time = gSim->futureCycle(Simulator::Clock::CHANNEL, cycles);
    if (time == gSim->time()) {
      startTransaction();
    } else {
      addEvent(time, 0, nullptr, kRequestEvt);
    }
  }
}

void PulseTerminal::sendResponse(Message* _request) {
  assert(enableResponses_);

  // process the request received to make a response
  u32 destination = _request->getSourceId();
  u32 messageSize = messageSizeDistribution_->nextMessageSize(_request);
  u32 protocolClass = responseProtocolClass_;
  u64 transaction = _request->getTransaction();
  u32 msgType = kResponseMsg;

  // delete the request
  delete _request;

  // determine the number of packets
  u32 numPackets = messageSize / maxPacketSize_;
  if ((messageSize % maxPacketSize_) > 0) {
    numPackets++;
  }

  // create the message object
  Message* message = new Message(numPackets, nullptr);
  message->setProtocolClass(protocolClass);
  message->setTransaction(transaction);
  message->setOpCode(msgType);

  // create the packets
  u32 flitsLeft = messageSize;
  for (u32 p = 0; p < numPackets; p++) {
    u32 packetLength = flitsLeft > maxPacketSize_ ?
                       maxPacketSize_ : flitsLeft;

    Packet* packet = new Packet(p, packetLength, message);
    message->setPacket(p, packet);

    // create flits
    for (u32 f = 0; f < packetLength; f++) {
      bool headFlit = f == 0;
      bool tailFlit = f == (packetLength - 1);
      Flit* flit = new Flit(f, headFlit, tailFlit, packet);
      packet->setFlit(f, flit);
    }
    flitsLeft -= packetLength;
  }

  // send the message
  u32 msgId = sendMessage(message, destination);
  (void)msgId;  // unused
}

}  // namespace Pulse

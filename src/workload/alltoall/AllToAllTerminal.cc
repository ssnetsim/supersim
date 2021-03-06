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
#include "workload/alltoall/AllToAllTerminal.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <utility>

#include "mut/mut.h"
#include "network/Network.h"
#include "stats/MessageLog.h"
#include "types/Flit.h"
#include "types/Packet.h"
#include "workload/alltoall/Application.h"
#include "workload/util.h"

// these are event types
#define kRequestEvt (0xFA)
#define kResponseEvt (0x82)

// this app defines the following message OpCodes
static const u32 kRequestMsg = kRequestEvt;
static const u32 kResponseMsg = kResponseEvt;

// this app sends the following data with each request
namespace {
struct RequestData {
  u32 iteration;
};
}  // namespace

namespace AllToAll {

AllToAllTerminal::AllToAllTerminal(const std::string& _name,
                                   const Component* _parent, u32 _id,
                                   const std::vector<u32>& _address,
                                   ::Application* _app,
                                   nlohmann::json _settings)
    : ::Terminal(_name, _parent, _id, _address, _app) {
  // get the injection rate
  assert(_settings.contains("request_injection_rate") &&
         _settings["request_injection_rate"].is_number_float());
  requestInjectionRate_ = _settings["request_injection_rate"].get<f64>();
  assert(requestInjectionRate_ >= 0.0 && requestInjectionRate_ <= 1.0);

  // iteration quantity limitation
  assert(_settings.contains("num_iterations"));
  numIterations_ = _settings["num_iterations"].get<u32>();
  assert(_settings.contains("perform_barriers") &&
         _settings["perform_barriers"].is_boolean());
  performBarriers_ = _settings["perform_barriers"].get<bool>();
  inBarrier_ = false;

  // max packet size
  maxPacketSize_ = _settings["max_packet_size"].get<u32>();
  assert(maxPacketSize_ > 0);

  // transaction size
  transactionSize_ = _settings["transaction_size"].get<u32>();
  assert(transactionSize_ > 0);

  // create a traffic pattern
  trafficPattern_ = DistributionTrafficPattern::create(
      "TrafficPattern", this, application()->numTerminals(), id_,
      _settings["traffic_pattern"]);

  // create a message size distribution
  messageSizeDistribution_ = MessageSizeDistribution::create(
      "MessageSizeDistribution", this, _settings["message_size_distribution"]);

  // protocol class of injection of requests
  assert(_settings.contains("request_protocol_class"));
  requestProtocolClass_ = _settings["request_protocol_class"].get<u32>();

  // enablement of request/response flows
  assert(_settings.contains("enable_responses") &&
         _settings["enable_responses"].is_boolean());
  enableResponses_ = _settings["enable_responses"].get<bool>();

  // latency of request processing
  assert(!enableResponses_ || _settings.contains("request_processing_latency"));
  requestProcessingLatency_ =
      _settings["request_processing_latency"].get<u32>();

  // protocol class of injection of responses
  assert(!enableResponses_ || _settings.contains("response_protocol_class"));
  responseProtocolClass_ = _settings["response_protocol_class"].get<u32>();

  // start time delay
  assert(_settings.contains("delay"));
  delay_ = _settings["delay"].get<u32>();

  // initialize the counters
  loggableCompleteCount_ = 0;

  // initialize the iteration state
  sendIteration_ = 0;
  recvIteration_ = 0;
  sendWaitingForRecv_ = false;
}

AllToAllTerminal::~AllToAllTerminal() {
  assert(sendWaitingForRecv_ == false);
  assert(outstandingTransactions_.size() == 0);

  assert((sendIteration_ == recvIteration_) &&
         (sendIteration_ == numIterations_) &&
         (outstandingTransactions_.size() == 0));

  delete trafficPattern_;
  delete messageSizeDistribution_;
}

void AllToAllTerminal::processEvent(void* _event, s32 _type) {
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

f64 AllToAllTerminal::percentComplete() const {
  if (numIterations_ == 0) {
    return 1.0;
  } else {
    u32 totalTransactions = numIterations_ * trafficPattern_->size();
    u32 count = loggableCompleteCount_;
    return (f64)count / (f64)totalTransactions;
  }
}

f64 AllToAllTerminal::requestInjectionRate() const {
  return requestInjectionRate_;
}

void AllToAllTerminal::start() {
  Application* app = reinterpret_cast<Application*>(application());

  if (numIterations_ == 0) {
    dbgprintf("complete");
    app->terminalComplete(id_);
  } else {
    // choose a random number of cycles in the future to start
    // make an event to start the AllToAllTerminal in the future
    if (requestInjectionRate_ > 0.0) {
      u32 maxMsg = messageSizeDistribution_->maxMessageSize();
      u32 maxTrans = maxMsg * transactionSize_;
      u64 cycles = cyclesToSend(requestInjectionRate_, maxTrans);
      cycles = gSim->rnd.nextU64(delay_, delay_ + cycles * 3);
      u64 time = gSim->futureCycle(Simulator::Clock::TERMINAL, 1) +
                 ((cycles - 1) * gSim->cycleTime(Simulator::Clock::TERMINAL));
      dbgprintf("start time is %lu", time);
      addEvent(time, 0, nullptr, kRequestEvt);
    } else {
      dbgprintf("not running");
    }
  }
}

void AllToAllTerminal::exitBarrier() {
  assert(performBarriers_);
  assert(!sendWaitingForRecv_);

  // make sure we are in a barrier
  assert(inBarrier_);
  assert(recvIteration_ == sendIteration_);

  // start sending again
  dbgprintf("unwaiting");
  inBarrier_ = false;
  if (sendIteration_ != numIterations_) {
    u64 reqTime = gSim->futureCycle(Simulator::Clock::TERMINAL, 1);
    addEvent(reqTime, 0, nullptr, kRequestEvt);
  }
}

void AllToAllTerminal::handleDeliveredMessage(Message* _message) {
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

    // check if complete (send was last requests only)
    if (!enableResponses_) {
      checkCompletion();
    }
  }
}

void AllToAllTerminal::handleReceivedMessage(Message* _message) {
  Application* app = reinterpret_cast<Application*>(application());
  u32 msgType = _message->getOpCode();
  u64 transId = _message->getTransaction();

  // handle requests (sends)
  if (msgType == kRequestMsg) {
    // pull out the request's iteration and delete the RequestData
    RequestData* reqData = reinterpret_cast<RequestData*>(_message->getData());
    u32 reqIteration = reqData->iteration;
    delete reqData;
    _message->setData(nullptr);

    // verify proper iteration received
    assert(reqIteration >= recvIteration_);
    assert(reqIteration <= recvIteration_ + 1);

    // add the received message to the appropriate iteration
    std::unordered_map<u32, u32>& iterationReceived1 =
        iterationReceived_[reqIteration];  // creates if not present
    u32 sourceId = _message->getSourceId();
    if (iterationReceived1.find(sourceId) == iterationReceived1.end()) {
      iterationReceived1[sourceId] = 1;
    } else {
      iterationReceived1[sourceId]++;
    }

    // try to clean up iterations in order
    bool advRecvIter = false;
    while (true) {
      // retrieve the set we care about (may not exist)
      std::unordered_map<u32, u32>& iterationReceived2 =
          iterationReceived_[recvIteration_];

      // check if recv is full (received from enough sources)
      if (iterationReceived2.size() != trafficPattern_->size()) {
        break;
      }

      // check if recv is full (received enough from sources)
      bool full = true;
      for (const std::pair<u32, u32>& p : iterationReceived2) {
        if (p.second != transactionSize_) {
          full = false;
          break;
        }
      }
      if (!full) {
        break;
      }

      // remove the current iteration state
      u32 cnt = iterationReceived_.erase(recvIteration_);
      assert(cnt == 1);

      // advance to the next iteration
      recvIteration_++;
      advRecvIter = true;
    }

    // perform barrier entrance
    if (advRecvIter && performBarriers_) {
      app->terminalAtBarrier(id());
    }

    // handle the case where the send operation is stalled by the recv operation
    if (sendWaitingForRecv_ && (recvIteration_ >= sendIteration_)) {
      dbgprintf("unwaiting");
      assert(!performBarriers_);

      // disable the waiting flag
      sendWaitingForRecv_ = false;

      // schedule the next request
      u64 reqTime = gSim->futureCycle(Simulator::Clock::TERMINAL, 1);
      addEvent(reqTime, 0, nullptr, kRequestEvt);
    }
  }

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

  // check if complete (recv came last) or
  // check if complete (send was last requests and responses)
  checkCompletion();

  if (enableResponses_ && msgType == kRequestMsg) {
    // signal for requests to generate responses when responses are enabled
    // register an event to process the request
    if (requestProcessingLatency_ == 0) {
      sendResponse(_message);
    } else {
      u64 respTime = gSim->futureCycle(Simulator::Clock::TERMINAL,
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

bool AllToAllTerminal::completeTracking(u64 _transId) {
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

void AllToAllTerminal::completeLoggable(u64 _transId) {
  // clear the logging entry
  assert(outstandingTransactions_.find(_transId) ==
         outstandingTransactions_.end());
  u64 res = transactionsToLog_.erase(_transId);
  assert(res == 1);

  // log the message/transaction
  Application* app = reinterpret_cast<Application*>(application());
  app->workload()->messageLog()->endTransaction(_transId);
  loggableCompleteCount_++;
}

void AllToAllTerminal::checkCompletion() {
  // detect when done
  if ((sendIteration_ == recvIteration_) &&
      (sendIteration_ == numIterations_) &&
      (outstandingTransactions_.size() == 0)) {
    dbgprintf("complete");
    Application* app = reinterpret_cast<Application*>(application());
    app->terminalComplete(id_);
  }
}

void AllToAllTerminal::startTransaction() {
  Application* app = reinterpret_cast<Application*>(application());

  // determine if another request can be generated
  assert(!inBarrier_);
  if (sendIteration_ > recvIteration_) {
    // the send operation has completed before the recv operation
    dbgprintf("waiting");
    assert(!performBarriers_);
    sendWaitingForRecv_ = true;
  } else {
    assert(!sendWaitingForRecv_);

    // generate a new request
    u32 destination = trafficPattern_->nextDestination();
    u32 messageSize = messageSizeDistribution_->nextMessageSize();
    u32 protocolClass = requestProtocolClass_;
    u64 transaction = createTransaction();
    u32 msgType = kRequestMsg;
    u32 sendIteration = sendIteration_;

    // handle this iteration's distribution
    if (trafficPattern_->complete()) {
      // if barriers used, wait for barrier to start next send iteration
      if (performBarriers_) {
        inBarrier_ = true;
      }
      sendIteration_++;
      trafficPattern_->reset();
    }

    // start tracking the transaction
    bool res = outstandingTransactions_
                   .insert(std::make_pair(transaction, transactionSize_))
                   .second;
    assert(res);

    // register the transaction for logging
    bool res2 = transactionsToLog_.insert(transaction).second;
    assert(res2);
    app->workload()->messageLog()->startTransaction(transaction);

    // determine the number of packets
    u32 numPackets = messageSize / maxPacketSize_;
    if ((messageSize % maxPacketSize_) > 0) {
      numPackets++;
    }

    // create N requests for this transaction
    for (u32 req = 0; req < transactionSize_; req++) {
      // create the message object
      Message* message = new Message(numPackets, nullptr);
      message->setProtocolClass(protocolClass);
      message->setTransaction(transaction);
      message->setOpCode(msgType);

      // set the data
      RequestData* reqData = new RequestData();
      reqData->iteration = sendIteration;
      message->setData(reqData);

      // create the packets
      u32 flitsLeft = messageSize;
      for (u32 p = 0; p < numPackets; p++) {
        u32 packetLength =
            flitsLeft > maxPacketSize_ ? maxPacketSize_ : flitsLeft;

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

    // determine when to send the next request
    if (!inBarrier_ && sendIteration_ < numIterations_) {
      u64 transSize = messageSize * transactionSize_;
      u64 cycles = cyclesToSend(requestInjectionRate_, transSize);
      u64 time = gSim->futureCycle(Simulator::Clock::TERMINAL, cycles);
      if (time == gSim->time()) {
        startTransaction();
      } else {
        addEvent(time, 0, nullptr, kRequestEvt);
      }
    } else {
      dbgprintf("waiting");
    }
  }
}

void AllToAllTerminal::sendResponse(Message* _request) {
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
    u32 packetLength = flitsLeft > maxPacketSize_ ? maxPacketSize_ : flitsLeft;

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

}  // namespace AllToAll

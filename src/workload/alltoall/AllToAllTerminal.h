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
#ifndef WORKLOAD_ALLTOALL_ALLTOALLTERMINAL_H_
#define WORKLOAD_ALLTOALL_ALLTOALLTERMINAL_H_

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "event/Component.h"
#include "nlohmann/json.hpp"
#include "prim/prim.h"
#include "traffic/distribution/DistributionTrafficPattern.h"
#include "traffic/size/MessageSizeDistribution.h"
#include "workload/Terminal.h"

class Application;

namespace AllToAll {

class Application;

class AllToAllTerminal : public Terminal {
 public:
  AllToAllTerminal(const std::string& _name, const Component* _parent, u32 _id,
                   const std::vector<u32>& _address, ::Application* _app,
                   nlohmann::json _settings);
  ~AllToAllTerminal();
  void processEvent(void* _event, s32 _type) override;
  f64 percentComplete() const;
  f64 requestInjectionRate() const;
  void start();
  void exitBarrier();

 protected:
  void handleDeliveredMessage(Message* _message) override;
  void handleReceivedMessage(Message* _message) override;

 private:
  bool completeTracking(u64 _transId);
  void completeLoggable(u64 _transId);
  void checkCompletion();
  void startTransaction();
  void sendResponse(Message* _request);

  // traffic generation
  f64 requestInjectionRate_;
  u32 numIterations_;
  u32 maxPacketSize_;    // flits
  u32 transactionSize_;  // requests
  DistributionTrafficPattern* trafficPattern_;
  MessageSizeDistribution* messageSizeDistribution_;

  // requests
  u32 requestProtocolClass_;

  // responses
  bool enableResponses_;
  std::unordered_map<u64, u32> outstandingTransactions_;  // recv count
  u32 responseProtocolClass_;
  u64 requestProcessingLatency_;  // cycles

  // start time delay
  u64 delay_;

  // logging and message generation
  std::unordered_set<u32> transactionsToLog_;
  u32 loggableCompleteCount_;

  // these track the state of the iterations
  u32 sendIteration_;
  u32 recvIteration_;
  std::unordered_map<u32, std::unordered_map<u32, u32>> iterationReceived_;
  bool sendWaitingForRecv_;
  bool performBarriers_;
  bool inBarrier_;
};

}  // namespace AllToAll

#endif  // WORKLOAD_ALLTOALL_ALLTOALLTERMINAL_H_

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
#ifndef WORKLOAD_PARAGRAPH_GRAPHTERMINAL_H_
#define WORKLOAD_PARAGRAPH_GRAPHTERMINAL_H_

#include <memory>
#include <queue>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "colhash/tuplehash.h"
#include "event/Component.h"
#include "nlohmann/json.hpp"
#include "paragraph/scheduling/graph_scheduler.h"
#include "prim/prim.h"
#include "types/Message.h"
#include "workload/Terminal.h"

class Application;

namespace ParaGraph {

class GraphTerminal : public Terminal {
 public:
  GraphTerminal(const std::string& _name, const Component* _parent, u32 _id,
                const std::vector<u32>& _address, ::Application* _app,
                u64 _seed, nlohmann::json _settings);
  ~GraphTerminal();
  void processEvent(void* _event, s32 _type) override;
  f64 percentComplete() const;
  void start();

 protected:
  void handleDeliveredMessage(Message* _message) override;
  void handleReceivedMessage(Message* _message) override;

 private:
  void loadReadyInstructions();
  void schedule();
  void finishExecution(paragraph::Instruction* _instruction);
  u32 bytesToFlits(u32 _bytes) const;
  void generateMessage(u32 _destination, u64 _sequence_number, u32 _size);
  void processReceivedMessage(Message* _message);

  u32 availableCores_;
  u32 protocolClass_;
  u32 maxPacketSize_;
  u32 bytesPerFlit_;
  u64 unitsPerSecond_;

  std::unique_ptr<paragraph::Graph> graph_;
  std::unique_ptr<paragraph::GraphScheduler> scheduler_;
  std::queue<paragraph::Instruction*> readyInstructions_;
  std::unordered_set<paragraph::Instruction*> executingInstructions_;

  // registered expecting messages: (src, seq)->(inst, size)
  std::unordered_map<std::tuple<u32, u64>,
                     std::tuple<paragraph::Instruction*, u32>>
      expecting_;
  // messages received before expecting: (src, seq)->(size)
  std::unordered_map<std::tuple<u32, u64>, u32> received_;
};

}  // namespace ParaGraph

#endif  // WORKLOAD_PARAGRAPH_GRAPHTERMINAL_H_

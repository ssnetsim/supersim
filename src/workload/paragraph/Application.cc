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
#include "workload/paragraph/Application.h"

#include <bits/bits.h>
#include <factory/ObjectFactory.h>

#include <cassert>
#include <vector>

#include "event/Simulator.h"
#include "network/Network.h"
#include "workload/paragraph/GraphTerminal.h"

namespace ParaGraph {

Application::Application(const std::string& _name, const Component* _parent,
                         u32 _id, Workload* _workload,
                         MetadataHandler* _metadataHandler,
                         nlohmann::json _settings)
    : ::Application(_name, _parent, _id, _workload, _metadataHandler,
                    _settings) {
  // create terminals
  remainingTerminals_ = numTerminals();
  u64 seed = gSim->rnd.nextU64();
  for (u32 t = 0; t < numTerminals(); t++) {
    std::vector<u32> address;
    gSim->getNetwork()->translateInterfaceIdToAddress(t, &address);
    std::string tname = "GraphTerminal_" + std::to_string(t);
    GraphTerminal* terminal = new GraphTerminal(
        tname, this, t, address, this, seed, _settings["graph_terminal"]);
    setTerminal(t, terminal);
  }

  // this application is immediately ready
  addEvent(0, 0, nullptr, 0);
}

Application::~Application() {
  assert(remainingTerminals_ == 0);
}

f64 Application::percentComplete() const {
  f64 percentSum = 0.0;
  for (u32 idx = 0; idx < numTerminals(); idx++) {
    GraphTerminal* pt = reinterpret_cast<GraphTerminal*>(getTerminal(idx));
    percentSum += pt->percentComplete();
  }
  return percentSum / numTerminals();
}

void Application::start() {
  dbgprintf("starting");
  // tell the terminals to start
  for (u32 idx = 0; idx < numTerminals(); idx++) {
    GraphTerminal* pt = reinterpret_cast<GraphTerminal*>(getTerminal(idx));
    pt->start();
  }
}

void Application::stop() {
  // this application is done
  workload_->applicationDone(id_);
}

void Application::kill() {
  // terminals have already ended
}

void Application::terminalComplete(u32 _id) {
  remainingTerminals_--;
  if (remainingTerminals_ == 0) {
    dbgprintf("complete");
    workload_->applicationComplete(id_);
  }
}

void Application::processEvent(void* _event, s32 _type) {
  dbgprintf("application ready");
  workload_->applicationReady(id_);
}

}  // namespace ParaGraph

registerWithObjectFactory("paragraph", ::Application, ParaGraph::Application,
                          APPLICATION_ARGS);

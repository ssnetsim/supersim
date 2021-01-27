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
#ifndef ROUTING_ROUTINGALGORITHM_TESTLIB_H_
#define ROUTING_ROUTINGALGORITHM_TESTLIB_H_

#include <string>

#include "prim/prim.h"
#include "router/Router.h"

/*
 * This class is useful for basic routing algorithm unit tests
 *  where you need a dummy router so that you can simply construct
 *  a routing algorithm.
 */
class RoutingAlgorithmTestRouter : public Router {
 public:
  RoutingAlgorithmTestRouter(
      const std::string& _name, u32 _numPorts, u32 _numVcs);
  ~RoutingAlgorithmTestRouter();

  void setInputChannel(u32 _port, Channel* _channel) override;
  Channel* getInputChannel(u32 _port) const override;
  void setOutputChannel(u32 _port, Channel* _channel) override;
  Channel* getOutputChannel(u32 _port) const override;

  void receiveFlit(u32 _port, Flit* _flit) override;
  void receiveCredit(u32 _port, Credit* _credit) override;

  void sendCredit(u32 _port, u32 _vc) override;
  void sendFlit(u32 _port, Flit* _flit) override;

  f64 congestionStatus(u32 _inputPort, u32 _inputVc, u32 _outputPort,
                       u32 _outputVc) const override;
};

#endif  // ROUTING_ROUTINGALGORITHM_TESTLIB_H_

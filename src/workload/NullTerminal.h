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
#ifndef WORKLOAD_NULLTERMINAL_H_
#define WORKLOAD_NULLTERMINAL_H_

#include <string>
#include <vector>

#include "event/Component.h"
#include "prim/prim.h"
#include "workload/Terminal.h"

class NullTerminal : public Terminal {
 public:
  NullTerminal(const std::string& _name, const Component* _parent, u32 _id,
               const std::vector<u32>& _address, Application* _app);
  virtual ~NullTerminal();

 protected:
  void handleDeliveredMessage(Message* _message) override;
  void handleReceivedMessage(Message* _message) override;
};

#endif  // WORKLOAD_NULLTERMINAL_H_

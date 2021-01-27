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
#ifndef NETWORK_COMMON_INJECTION_H_
#define NETWORK_COMMON_INJECTION_H_

#include "interface/Interface.h"
#include "prim/prim.h"
#include "routing/InjectionAlgorithm.h"
#include "types/Message.h"

namespace Common {

void injection(
    Interface* _interface, InjectionAlgorithm* _algorithm, u32 _baseVc,
    u32 _numVcs, bool _adaptive, bool _fixedMsgVc, Message* _message);

}  // namespace Common

#endif  // NETWORK_COMMON_INJECTION_H_

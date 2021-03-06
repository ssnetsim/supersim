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
#ifndef ALLOCATOR_ALLOCATOR_TESTLIB_H_
#define ALLOCATOR_ALLOCATOR_TESTLIB_H_

#include "nlohmann/json.hpp"
#include "prim/prim.h"

typedef void (*AllocatorVerifier)(u32 _numClients, u32 _numResources,
                                  const bool* _request, const u64* _metadata,
                                  const bool* _grant);

u64 AllocatorIndex(u64 _numClients, u64 _client, u64 _resource);

void AllocatorTest(nlohmann::json _settings, AllocatorVerifier _verifier,
                   bool _singleRequest);
void AllocatorLoadBalanceTest(nlohmann::json _settings);

#endif  // ALLOCATOR_ALLOCATOR_TESTLIB_H_

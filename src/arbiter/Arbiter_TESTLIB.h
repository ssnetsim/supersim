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
#ifndef ARBITER_ARBITER_TESTLIB_H_
#define ARBITER_ARBITER_TESTLIB_H_

#include <vector>

#include "arbiter/Arbiter.h"
#include "prim/prim.h"

u32 hotCount(bool* _bools, u32 _len);
u32 winnerId(bool* _bools, u32 _len);

#endif  // ARBITER_ARBITER_TESTLIB_H_

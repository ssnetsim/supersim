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
#include "routing/util.h"

#include <cassert>

u32 vcToRc(u32 _baseVc, u32 _numVcs, u32 _vc, u32 _rcs) {
  assert(_vc >= _baseVc && _vc < _baseVc + _numVcs);
  u32 relVc = _vc - _baseVc;
  return relVc % _rcs;
}

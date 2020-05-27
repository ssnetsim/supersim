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
#include "network/common/injection.h"

#include <cassert>

#include <tuple>
#include <vector>

namespace Common {

void injection(
    Interface* _interface, InjectionAlgorithm* _algorithm, u32 _baseVc,
    u32 _numVcs, bool _adaptive, bool _fixedMsgVc, Message* _message) {
  // use the protocol class to set the injection VC(s)
  u32 pktPort = U32_MAX;
  u32 pktVc = U32_MAX;
  for (u32 p = 0; p < _message->numPackets(); p++) {
    Packet* packet = _message->packet(p);

    // get the packet's VC
    if (!_fixedMsgVc || pktVc == U32_MAX) {
      // choose VC
      if (_adaptive) {
        // find all minimally congested VCs within the protocol class
        std::vector<std::tuple<u32, u32> > minOutputs;
        u32 minOccupancy = U32_MAX;
        for (u32 port = 0; port < _interface->numPorts(); port++) {
          for (u32 vc = _baseVc; vc < _baseVc + _numVcs; vc++) {
            u32 occupancy = _interface->occupancy(port, vc);
            if (occupancy < minOccupancy) {
              minOccupancy = occupancy;
              minOutputs.clear();
            }
            if (occupancy <= minOccupancy) {
              minOutputs.push_back(std::make_tuple(port, vc));
            }
          }
        }

        // choose randomly among the minimally congested VCs
        assert(minOutputs.size() > 0);
        u32 rnd = gSim->rnd.nextU64(0, minOutputs.size() - 1);
        pktPort = std::get<0>(minOutputs.at(rnd));
        pktVc = std::get<1>(minOutputs.at(rnd));
      } else {
        // choose a random VC within the protocol class
        pktPort = gSim->rnd.nextU64(0, _interface->numPorts() - 1);
        pktVc = gSim->rnd.nextU64(_baseVc, _baseVc + _numVcs - 1);
      }
    }

    // inform the algorithm of the packet's injection
    _algorithm->injectPacket(packet, pktPort, pktVc);
  }
}

}  // namespace Common

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
#include "network/fattree/CommonAncestorRoutingAlgorithm.h"

#include <cassert>

#include <tuple>
#include <unordered_set>
#include <vector>

#include "factory/ObjectFactory.h"
#include "network/fattree/util.h"
#include "strop/strop.h"
#include "types/Message.h"
#include "types/Packet.h"

namespace FatTree {

CommonAncestorRoutingAlgorithm::CommonAncestorRoutingAlgorithm(
    const std::string& _name, const Component* _parent, Router* _router,
    u32 _baseVc, u32 _numVcs, u32 _inputPort, u32 _inputVc,
    const std::vector<std::tuple<u32, u32, u32> >* _radices,
    u32 _interfacePorts, nlohmann::json _settings)
    : RoutingAlgorithm(_name, _parent, _router, _baseVc, _numVcs, _inputPort,
                       _inputVc, _radices, _interfacePorts, _settings),
      mode_(parseRoutingMode(_settings["mode"].get<std::string>())),
      leastCommonAncestor_(_settings["least_common_ancestor"].get<bool>()),
      selection_(parseSelection(_settings["selection"].get<std::string>())),
      randomId_(gSim->rnd.nextU64()) {
  assert(!_settings["least_common_ancestor"].is_null());
  assert(!_settings["mode"].is_null());
  assert(!_settings["selection"].is_null());

  // create the random number generator
  random_.seed(randomId_);

  // create the reduction
  reduction_ = Reduction::create("Reduction", this, _router, mode_,
                                 false, _settings["reduction"]);
}

CommonAncestorRoutingAlgorithm::~CommonAncestorRoutingAlgorithm() {
  delete reduction_;
}

void CommonAncestorRoutingAlgorithm::processRequest(
    Flit* _flit, RoutingAlgorithm::Response* _response) {
  // addresses
  const std::vector<u32>* sourceAddress =
      _flit->packet()->message()->getSourceAddress();
  const std::vector<u32>* destinationAddress =
      _flit->packet()->message()->getDestinationAddress();
  assert(sourceAddress->size() == destinationAddress->size());

  // topology info
  const u32 level = router_->address().at(0);
  const u32 numLevels = sourceAddress->size();
  const u32 downPorts = std::get<0>(radices_->at(level));
  const u32 upPorts = std::get<1>(radices_->at(level));

  // current location info
  bool atTopLevel = (level == (numLevels - 1));
  bool movingUpward = (!atTopLevel) && (inputPort_ < downPorts);
  u32 lca = leastCommonAncestor(sourceAddress, destinationAddress);

  // determine if an early turn around will occur
  if (movingUpward && leastCommonAncestor_) {
    // determine if this router is an ancester of the destination
    assert(lca >= level);
    if (lca == level) {
      movingUpward = false;
    }
  }

  // determine hop to destination
  u32 hops;
  if (movingUpward) {
    assert(level < numLevels - 1);
    if (leastCommonAncestor_) {
      hops = (lca + 1) + (lca - level);
    } else {
      hops = numLevels + (numLevels - level - 1);
    }
  } else {
    hops = level + 1;
  }

  // select the outputs
  if (!movingUpward) {
    // moving downward on a deterministic path
    if (level == 0) {
      // at destination router
      u32 basePort = destinationAddress->at(level) * interfacePorts_;
      for (u32 offset = 0; offset < interfacePorts_; offset++) {
        addPort(basePort + offset, hops);
      }
    } else {
      // at intermediate router
      u32 port = destinationAddress->at(level);
      addPort(port, hops);
    }
  } else {
    // moving upward
    if (selection_ == CommonAncestorRoutingAlgorithm::Selection::kAll) {
      // choose all upward ports
      for (u32 up = 0; up < upPorts; up++) {
        u32 port = downPorts + up;
        addPort(port, hops);
      }
    } else if (selection_ ==
               CommonAncestorRoutingAlgorithm::Selection::kFlowCache) {
      // choose a random path for each flow, cache the result
      u32 port = downPorts + (flowCache(_flit) % upPorts);
      addPort(port, hops);
    } else if (selection_ ==
               CommonAncestorRoutingAlgorithm::Selection::kFlowHash) {
      // hash the flow information to choose a deterministic upward path
      u32 port = downPorts + (flowHash(_flit) % upPorts);
      addPort(port, hops);
    } else {
      assert(false);
    }
  }

  // reduction phase
  const std::unordered_set<std::tuple<u32, u32> >* outputs =
      reduction_->reduce(nullptr);
  for (const auto& t : *outputs) {
    u32 port = std::get<0>(t);
    u32 vc = std::get<1>(t);
    if (vc == U32_MAX) {
      for (u32 vc = baseVc_; vc < baseVc_ + numVcs_; vc++) {
        _response->add(port, vc);
      }
    } else {
      _response->add(port, vc);
    }
  }
}

void CommonAncestorRoutingAlgorithm::addPort(u32 _port, u32 _hops) {
  if (routingModeIsPort(mode_)) {
    // add the port as a whole
    f64 cong = portCongestion(mode_, router_, inputPort_, inputVc_, _port);
    reduction_->add(_port, U32_MAX, _hops, cong);
  } else {
    // add all VCs in the port
    for (u32 vc = baseVc_; vc < baseVc_ + numVcs_; vc++) {
      f64 cong = router_->congestionStatus(inputPort_, inputVc_, _port, vc);
      reduction_->add(_port, vc, _hops, cong);
    }
  }
}

CommonAncestorRoutingAlgorithm::Selection
CommonAncestorRoutingAlgorithm::parseSelection(
    const std::string& _selection) const {
  if (_selection == "all") {
    return CommonAncestorRoutingAlgorithm::Selection::kAll;
  } else if (_selection == "flow_cache") {
    return CommonAncestorRoutingAlgorithm::Selection::kFlowCache;
  } else if (_selection == "flow_hash") {
    return CommonAncestorRoutingAlgorithm::Selection::kFlowHash;
  } else {
    fprintf(stderr, "Unknown fat-tree selecion: %s\n", _selection.c_str());
    assert(false);
  }
}

u64 CommonAncestorRoutingAlgorithm::flowCache(const Flit* _flit) {
  u32 sourceId = _flit->packet()->message()->getSourceId();
  u32 destinationId = _flit->packet()->message()->getDestinationId();
  u64 srcDst = (static_cast<u64>(sourceId) << 32) | destinationId;
  if (flowCache_.find(srcDst) == flowCache_.end()) {
    u64 rand = random_.nextU64();
    flowCache_[srcDst] = rand;
  }
  return flowCache_.at(srcDst);
}

// from http://burtleburtle.net/bob/c/lookup3.c
#define rot(x, k) (((x) << (k)) | ((x) >> (32 - (k))))
#define final(a, b, c) {                        \
    c ^= b; c -= rot(b, 14);                    \
    a ^= c; a -= rot(c, 11);                    \
    b ^= a; b -= rot(a, 25);                    \
    c ^= b; c -= rot(b, 16);                    \
    a ^= c; a -= rot(c, 4);                     \
    b ^= a; b -= rot(a, 14);                    \
    c ^= b; c -= rot(b, 24);                    \
  }

u64 CommonAncestorRoutingAlgorithm::flowHash(const Flit* _flit) {
  u32 sourceId = _flit->packet()->message()->getSourceId();
  u32 destinationId = _flit->packet()->message()->getDestinationId();

  // using elements from hashword() http://burtleburtle.net/bob/c/lookup3.c
  // customed to hash exactly 2 u32s.
  u32 a, b, c;
  a = b = c = 0xdeadbeef + (2 << 2) + static_cast<u32>(randomId_);
  b += sourceId;
  a += destinationId;
  final(a, b, c);
  return c;
}

}  // namespace FatTree

registerWithObjectFactory("common_ancestor", FatTree::RoutingAlgorithm,
                          FatTree::CommonAncestorRoutingAlgorithm,
                          FATTREE_ROUTINGALGORITHM_ARGS);

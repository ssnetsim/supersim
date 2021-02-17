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
#include "network/hyperx/ValiantsRoutingAlgorithm.h"

#include <cassert>

#include "factory/ObjectFactory.h"
#include "types/Message.h"
#include "types/Packet.h"

namespace HyperX {

ValiantsRoutingAlgorithm::ValiantsRoutingAlgorithm(
    const std::string& _name, const Component* _parent, Router* _router,
    u32 _baseVc, u32 _numVcs, u32 _inputPort, u32 _inputVc,
    const std::vector<u32>& _dimensionWidths,
    const std::vector<u32>& _dimensionWeights, u32 _concentration,
    u32 _interfacePorts, nlohmann::json _settings)
    : RoutingAlgorithm(_name, _parent, _router, _baseVc, _numVcs, _inputPort,
                       _inputVc, _dimensionWidths, _dimensionWeights,
                       _concentration, _interfacePorts, _settings) {
  // VC set mapping:
  //  0 = injection from terminal port, to intermediate destination
  //  1 = switching dimension increments VC count
  //  ...
  //  N = last hop to dimension N to intermediate destination
  //  N + 1 = first hop from intermediate to final destination
  //  ...
  //  2N = last hop to dimension N
  //  we can eject flit to destination terminal using any VC

  assert(_settings.contains("intermediate_node") &&
         _settings["intermediate_node"].is_string());
  assert(_settings.contains("minimal") && _settings["minimal"].is_string());
  assert(_settings.contains("output_type") &&
         _settings["output_type"].is_string());
  assert(_settings.contains("max_outputs") &&
         _settings["max_outputs"].is_number_integer());

  assert(_settings.contains("output_algorithm") &&
         _settings["output_algorithm"].is_string());
  if (_settings["output_algorithm"].get<std::string>() == "random") {
    outputAlg_ = OutputAlg::Rand;
  } else if (_settings["output_algorithm"].get<std::string>() == "minimal") {
    outputAlg_ = OutputAlg::Min;
  } else {
    fprintf(stderr, "Unknown output algorithm:");
    fprintf(stderr, " '%s'\n",
            _settings["output_algorithm"].get<std::string>().c_str());
    assert(false);
  }

  maxOutputs_ = _settings["max_outputs"].get<u32>();

  std::string intermediateNode =
      _settings["intermediate_node"].get<std::string>();
  std::string minimalType = _settings["minimal"].get<std::string>();
  std::string outputType = _settings["output_type"].get<std::string>();
  shortCut_ = _settings["short_cut"].get<bool>();

  if (intermediateNode == "regular") {
    intNodeAlg_ = IntNodeAlg::REG;
  } else if (intermediateNode == "source") {
    intNodeAlg_ = IntNodeAlg::SRC;
  } else if (intermediateNode == "dest") {
    intNodeAlg_ = IntNodeAlg::DST;
  } else if (intermediateNode == "source_dest") {
    intNodeAlg_ = IntNodeAlg::SRCDST;
  } else if (intermediateNode == "unaligned") {
    intNodeAlg_ = IntNodeAlg::UNALIGNED;
  } else if (intermediateNode == "minimal_vc") {
    intNodeAlg_ = IntNodeAlg::MINV;
  } else if (intermediateNode == "minimal_port") {
    intNodeAlg_ = IntNodeAlg::MINP;
  } else {
    fprintf(stderr, "Unknown inter node algorithm:");
    fprintf(stderr, " '%s'\n", intermediateNode.c_str());
    assert(false);
  }

  if (minimalType == "dimension_order") {
    assert(numVcs_ >= 2);
    if (outputType == "vc") {
      routingAlg_ = BaseRoutingAlg::DORV;
    } else if (outputType == "port") {
      routingAlg_ = BaseRoutingAlg::DORP;
    } else {
      fprintf(stderr, "Unknown output type:");
      fprintf(stderr, " '%s'\n", outputType.c_str());
      assert(false);
    }
  } else if (minimalType == "random") {
    assert(numVcs_ >= (2 * _dimensionWidths.size()));
    if (outputType == "vc") {
      routingAlg_ = BaseRoutingAlg::RMINV;
    } else if (outputType == "port") {
      routingAlg_ = BaseRoutingAlg::RMINP;
    } else {
      fprintf(stderr, "Unknown output type:");
      fprintf(stderr, " '%s'\n", outputType.c_str());
      assert(false);
    }
  } else if (minimalType == "adaptive") {
    assert(numVcs_ >= (2 * _dimensionWidths.size()));
    if (outputType == "vc") {
      routingAlg_ = BaseRoutingAlg::AMINV;
    } else if (outputType == "port") {
      routingAlg_ = BaseRoutingAlg::AMINP;
    } else {
      fprintf(stderr, "Unknown output type:");
      fprintf(stderr, " '%s'\n", outputType.c_str());
      assert(false);
    }
  } else {
    fprintf(stderr, "Unknown inter minimal algorithm:");
    fprintf(stderr, " '%s'\n", minimalType.c_str());
    assert(false);
  }

  if ((routingAlg_ == BaseRoutingAlg::DORV) ||
      (routingAlg_ == BaseRoutingAlg::DORP)) {
    assert(_numVcs >= 2);
  } else if ((intNodeAlg_ == IntNodeAlg::REG) ||
             (intNodeAlg_ == IntNodeAlg::UNALIGNED)) {
    assert(_numVcs >= _dimensionWidths.size() * 2);
  } else {
    assert(_numVcs >= _dimensionWidths.size() + 1);
  }
}

ValiantsRoutingAlgorithm::~ValiantsRoutingAlgorithm() {}

void ValiantsRoutingAlgorithm::processRequest(
    Flit* _flit, RoutingAlgorithm::Response* _response) {
  const std::vector<u32>* destinationAddress =
      _flit->packet()->message()->getDestinationAddress();

  Packet* packet = _flit->packet();
  u32 vcSet = U32_MAX;
  u32 numVcSets = (routingAlg_ == BaseRoutingAlg::DORP) ||
                          (routingAlg_ == BaseRoutingAlg::DORV)
                      ? 1
                      : dimensionWidths_.size();
  if ((intNodeAlg_ == IntNodeAlg::REG) ||
      (intNodeAlg_ == IntNodeAlg::UNALIGNED)) {
    numVcSets *= 2;
  } else {
    numVcSets += 1;
  }

  if (packet->getHopCount() == 0) {
    if ((routingAlg_ == BaseRoutingAlg::DORP) ||
        (routingAlg_ == BaseRoutingAlg::DORV)) {
      vcSet = baseVc_ + 1;
    } else {
      vcSet = baseVc_;
    }
  } else {
    if ((routingAlg_ == BaseRoutingAlg::DORP) ||
        (routingAlg_ == BaseRoutingAlg::DORV)) {
      vcSet = baseVc_ + (_flit->getVc() - baseVc_) % numVcSets;
    } else {
      vcSet = baseVc_ + (_flit->getVc() - baseVc_ + 1) % numVcSets;
    }
  }

  valiantsRoutingOutput(router_, inputPort_, inputVc_, dimensionWidths_,
                        dimensionWeights_, concentration_, interfacePorts_,
                        destinationAddress, vcSet, numVcSets, baseVc_ + numVcs_,
                        shortCut_, intNodeAlg_, routingAlg_, _flit, &vcPool_);

  if ((routingAlg_ == BaseRoutingAlg::DORP) ||
      (routingAlg_ == BaseRoutingAlg::DORV)) {
    // get a const pointer to the address (with leading dummy)
    const std::vector<u32>* intermediateAddress =
        reinterpret_cast<const std::vector<u32>*>(
            packet->getRoutingExtension());
    if (intermediateAddress == nullptr) {
      vcSet = baseVc_;
    } else {
      vcSet = baseVc_ + 1;
    }
  }

  if ((routingAlg_ == BaseRoutingAlg::DORP) ||
      (routingAlg_ == BaseRoutingAlg::RMINP) ||
      (routingAlg_ == BaseRoutingAlg::AMINP)) {
    makeOutputPortSet(&vcPool_, {vcSet}, numVcSets, baseVc_ + numVcs_,
                      maxOutputs_, outputAlg_, &outputPorts_);
  } else if ((routingAlg_ == BaseRoutingAlg::DORV) ||
             (routingAlg_ == BaseRoutingAlg::RMINV) ||
             (routingAlg_ == BaseRoutingAlg::AMINV)) {
    makeOutputVcSet(&vcPool_, maxOutputs_, outputAlg_, &outputPorts_);
  } else {
    fprintf(stderr, "Unknown routing algorithm\n");
    assert(false);
  }

  if (outputPorts_.empty()) {
    // we can use any VC to eject packet
    u32 basePort = destinationAddress->at(0) * interfacePorts_;
    for (u32 offset = 0; offset < interfacePorts_; offset++) {
      u32 port = basePort + offset;
      for (u64 vc = baseVc_; vc < baseVc_ + numVcs_; vc++) {
        _response->add(port, vc);
      }
    }
    return;
  }

  for (auto it : outputPorts_) {
    if ((packet->getHopCount() > 0) && (routingAlg_ != BaseRoutingAlg::DORP) &&
        (routingAlg_ != BaseRoutingAlg::DORV)) {
      // For DOR we increment vcSet only after reaching intermediate node,
      // node after first hop as we do for minimal
      assert(vcSet > 0);
    }
    _response->add(std::get<0>(it), std::get<1>(it));
  }
}

}  // namespace HyperX

registerWithObjectFactory("valiants", HyperX::RoutingAlgorithm,
                          HyperX::ValiantsRoutingAlgorithm,
                          HYPERX_ROUTINGALGORITHM_ARGS);

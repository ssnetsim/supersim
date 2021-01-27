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
#include "network/hyperx/DalRoutingAlgorithm.h"

#include <cassert>

#include "factory/ObjectFactory.h"
#include "types/Message.h"
#include "types/Packet.h"

namespace HyperX {

DalRoutingAlgorithm::DalRoutingAlgorithm(
    const std::string& _name, const Component* _parent, Router* _router,
    u32 _baseVc, u32 _numVcs, u32 _inputPort, u32 _inputVc,
    const std::vector<u32>& _dimensionWidths,
    const std::vector<u32>& _dimensionWeights,
    u32 _concentration, u32 _interfacePorts, nlohmann::json _settings)
    : RoutingAlgorithm(_name, _parent, _router, _baseVc, _numVcs,
                       _inputPort, _inputVc, _dimensionWidths,
                       _dimensionWeights, _concentration, _interfacePorts,
                       _settings) {
  assert(_settings.contains("adaptivity_type") &&
         _settings["adaptivity_type"].is_string());
  assert(_settings.contains("output_type") &&
         _settings["output_type"].is_string());
  assert(_settings.contains("decision_scheme") &&
         _settings["decision_scheme"].is_string());

  assert(_settings.contains("max_outputs") &&
         _settings["max_outputs"].is_number_integer());
  maxOutputs_ = _settings["max_outputs"].get<u32>();

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

  std::string outputType = _settings["output_type"].get<std::string>();
  if (outputType == "port") {
    outputTypePort_ = true;
  } else if (outputType == "vc") {
    outputTypePort_ = false;
  } else {
    fprintf(stderr, "Unknown output type:");
    fprintf(stderr, " '%s'\n", outputType.c_str());
    assert(false);
  }

  if (_settings["adaptivity_type"].get<std::string>() == "dimension_adaptive") {
    if (outputType == "vc") {
      adaptivityType_ = AdaptiveRoutingAlg::DDALV;
    } else if (outputType == "port") {
      adaptivityType_ = AdaptiveRoutingAlg::DDALP;
    } else {
      fprintf(stderr, "Unknown output type:");
      fprintf(stderr, " '%s'\n", outputType.c_str());
      assert(false);
    }
  } else if (_settings["adaptivity_type"].get<std::string>() ==
             "dimension_order") {
    if (outputType == "vc") {
      adaptivityType_ = AdaptiveRoutingAlg::DOALV;
    } else if (outputType == "port") {
      adaptivityType_ = AdaptiveRoutingAlg::DOALP;
    } else {
      fprintf(stderr, "Unknown output type:");
      fprintf(stderr, " '%s'\n", outputType.c_str());
      assert(false);
    }
  } else if (_settings["adaptivity_type"].get<std::string>() == "variable") {
    assert(_settings.contains("max_deroutes"));
    maxDeroutesAllowed_ = _settings["max_deroutes"].get<u32>();
    assert(_settings.contains("multi_deroute"));
    multiDeroute_ = _settings["multi_deroute"].get<bool>();
    if (outputType == "vc") {
      adaptivityType_ = AdaptiveRoutingAlg::VDALV;
    } else if (outputType == "port") {
      adaptivityType_ = AdaptiveRoutingAlg::VDALP;
    } else {
      fprintf(stderr, "Unknown output type:");
      fprintf(stderr, " '%s'\n", outputType.c_str());
      assert(false);
    }
  } else {
    fprintf(stderr, "Unknown adaptive algorithm:");
    fprintf(stderr, " '%s'\n",
            _settings["adaptivity_type"].get<std::string>().c_str());
    assert(false);
  }

  if (_settings["decision_scheme"].get<std::string>() ==
      "monolithic_weighted") {
    decisionScheme_ = DecisionScheme::MW;
    assert(_settings.contains("independent_bias"));
    iBias_ = _settings["independent_bias"].get<f64>();
    assert(_settings.contains("congestion_bias"));
    cBias_ = _settings["congestion_bias"].get<f64>();
    if (!_settings.contains("bias_mode")) {
      biasMode_ = BiasScheme::REGULAR;
    } else if (_settings["bias_mode"].get<std::string>() == "regular") {
      biasMode_ = BiasScheme::REGULAR;
    } else if (_settings["bias_mode"].get<std::string>() == "bimodal") {
      biasMode_ = BiasScheme::BIMODAL;
    } else if (_settings["bias_mode"].get<std::string>() == "proportional") {
      biasMode_ = BiasScheme::PROPORTIONAL;
    } else if (_settings["bias_mode"].get<std::string>() == "differential") {
      biasMode_ = BiasScheme::DIFFERENTIAL;
    } else if (_settings["bias_mode"].get<std::string>() ==
               "proportional_differential") {
      biasMode_ = BiasScheme::PROPORTIONALDIF;
    } else {
      fprintf(stderr, "Unknown weighting scheme:");
      fprintf(stderr, " '%s'\n",
              _settings["bias_mode"].get<std::string>().c_str());
      assert(false);
    }
  } else if (_settings["decision_scheme"].get<std::string>() ==
             "staged_threshold") {
    decisionScheme_ = DecisionScheme::ST;
    assert(_settings.contains("threshold_min"));
    assert(_settings.contains("threshold_nonmin"));
    thresholdMin_ = _settings["threshold_min"].get<f64>();
    thresholdNonMin_ = _settings["threshold_nonmin"].get<f64>();
  } else if (_settings["decision_scheme"].get<std::string>() ==
             "threshold_weighted") {
    decisionScheme_ = DecisionScheme::TW;
    assert(_settings.contains("threshold"));
    threshold_ = _settings["threshold"].get<f64>();
  } else {
    fprintf(stderr, "Unknown decision scheme:");
    fprintf(stderr, " '%s'\n",
            _settings["decision_scheme"].get<std::string>().c_str());
    assert(false);
  }

  if (decisionScheme_ == DecisionScheme::MW ||
      decisionScheme_ == DecisionScheme::TW) {
    // ensure hop_count_mode if using weights

    assert(_settings.contains("hop_count_mode"));
    if (_settings["hop_count_mode"].get<std::string>() == "absolute") {
      hopCountMode_ = HopCountMode::ABS;
    } else if (_settings["hop_count_mode"].get<std::string>() == "normalized") {
      hopCountMode_ = HopCountMode::NORM;
    } else {
      fprintf(stderr, "Unknown hop_count scheme:");
      fprintf(stderr, " '%s'\n",
              _settings["hop_count_mode"].get<std::string>().c_str());
      assert(false);
    }
  }

  if ((adaptivityType_ == AdaptiveRoutingAlg::DOALV) ||
      (adaptivityType_ == AdaptiveRoutingAlg::DOALP)) {
    assert(numVcs_ >= 2);
  } else if ((adaptivityType_ == AdaptiveRoutingAlg::DDALV) ||
             (adaptivityType_ == AdaptiveRoutingAlg::DDALP)) {
    assert(numVcs_ >= dimensionWidths_.size() * 2);
  } else if ((adaptivityType_ == AdaptiveRoutingAlg::VDALV) ||
             (adaptivityType_ == AdaptiveRoutingAlg::VDALP)) {
    assert(numVcs_ >= dimensionWidths_.size() + maxDeroutesAllowed_);
  }

  if ((adaptivityType_ == AdaptiveRoutingAlg::DOALP) ||
      (adaptivityType_ == AdaptiveRoutingAlg::DOALV)) {
    numVcSets_ = 2;
  } else if ((adaptivityType_ == AdaptiveRoutingAlg::DDALP) ||
             (adaptivityType_ == AdaptiveRoutingAlg::DDALV)) {
    numVcSets_ = 2 * dimensionWidths_.size();
  } else if ((adaptivityType_ == AdaptiveRoutingAlg::VDALP) ||
             (adaptivityType_ == AdaptiveRoutingAlg::VDALV)) {
    numVcSets_ = dimensionWidths_.size() + maxDeroutesAllowed_;
  }
}

DalRoutingAlgorithm::~DalRoutingAlgorithm() {}

void DalRoutingAlgorithm::processRequest(
    Flit* _flit, RoutingAlgorithm::Response* _response) {
  Packet* packet = _flit->packet();
  const std::vector<u32>* destinationAddress =
      packet->message()->getDestinationAddress();
  const std::vector<u32>& routerAddress = router_->address();

  u32 vcSet = U32_MAX;
  if ((adaptivityType_ == AdaptiveRoutingAlg::DOALP) ||
      (adaptivityType_ == AdaptiveRoutingAlg::DOALV)) {
    // first hop in dimension is always VC = 0
    // if incoming dimension is un aligned, hence it is a deroute, then VC = 1
    u32 inDim = computeInputPortDim(dimensionWidths_, dimensionWeights_,
                                    concentration_, inputPort_);
    if ((inDim == U32_MAX) ||
        (routerAddress.at(inDim) == destinationAddress->at(inDim + 1))) {
      vcSet = baseVc_ + 0;
    } else {
      vcSet = baseVc_ + 1;
    }
  } else if ((adaptivityType_ == AdaptiveRoutingAlg::DDALP) ||
             (adaptivityType_ == AdaptiveRoutingAlg::DDALV)) {
    if (packet->getHopCount() == 0) {
      vcSet = baseVc_;
    } else {
      vcSet = baseVc_ + (_flit->getVc() - baseVc_ + 1) % numVcSets_;
    }
  } else if ((adaptivityType_ == AdaptiveRoutingAlg::VDALP) ||
             (adaptivityType_ == AdaptiveRoutingAlg::VDALV)) {
    if (packet->getHopCount() == 0) {
      vcSet = baseVc_;
    } else {
      vcSet = baseVc_ + (_flit->getVc() - baseVc_ + 1) % numVcSets_;
    }
  }

  if (adaptivityType_ == AdaptiveRoutingAlg::DOALP) {
    doalPortRoutingOutput(
        router_, inputPort_, inputVc_, dimensionWidths_, dimensionWeights_,
        concentration_, interfacePorts_, destinationAddress, baseVc_, vcSet,
        numVcSets_, baseVc_ + numVcs_, _flit, &outputVcsMin_, &outputVcsDer_);
  } else if (adaptivityType_ == AdaptiveRoutingAlg::DOALV) {
    doalVcRoutingOutput(
        router_, inputPort_, inputVc_, dimensionWidths_, dimensionWeights_,
        concentration_, interfacePorts_, destinationAddress, baseVc_, vcSet,
        numVcSets_, baseVc_ + numVcs_, _flit, &outputVcsMin_, &outputVcsDer_);
  } else if (adaptivityType_ == AdaptiveRoutingAlg::DDALP) {
    ddalPortRoutingOutput(
        router_, inputPort_, inputVc_, dimensionWidths_, dimensionWeights_,
        concentration_, interfacePorts_, destinationAddress, vcSet, numVcSets_,
        baseVc_ + numVcs_, _flit, &outputVcsMin_, &outputVcsDer_);
  } else if (adaptivityType_ == AdaptiveRoutingAlg::DDALV) {
    ddalVcRoutingOutput(
        router_, inputPort_, inputVc_, dimensionWidths_, dimensionWeights_,
        concentration_, interfacePorts_, destinationAddress, vcSet, numVcSets_,
        baseVc_ + numVcs_, _flit, &outputVcsMin_, &outputVcsDer_);
  } else if (adaptivityType_ == AdaptiveRoutingAlg::VDALP) {
    vdalPortRoutingOutput(
        router_, inputPort_, inputVc_, dimensionWidths_, dimensionWeights_,
        concentration_, interfacePorts_, destinationAddress, baseVc_, vcSet,
        numVcSets_, baseVc_ + numVcs_, _flit, multiDeroute_, &outputVcsMin_,
        &outputVcsDer_);
  } else if (adaptivityType_ == AdaptiveRoutingAlg::VDALV) {
    vdalVcRoutingOutput(
        router_, inputPort_, inputVc_, dimensionWidths_, dimensionWeights_,
        concentration_, interfacePorts_, destinationAddress, baseVc_, vcSet,
        numVcSets_, baseVc_ + numVcs_, _flit, multiDeroute_, &outputVcsMin_,
        &outputVcsDer_);
  } else {
    fprintf(stderr, "Unknown adaptive algorithm\n");
    assert(false);
  }

  u32 hops = hopsLeft(router_, destinationAddress);
  u32 hopIncr = U32_MAX;
  if (hopCountMode_ == HopCountMode::ABS) {
    hopIncr = 1;
  } else if (hopCountMode_ == HopCountMode::NORM) {
    hopIncr = hops;
  }

  bool takingDeroute = false;
  if (decisionScheme_ == DecisionScheme::MW) {
    monolithicWeighted(outputVcsMin_, outputVcsDer_,
                       hops, hopIncr, iBias_, cBias_, biasMode_,
                       &vcPool_, &takingDeroute);
    if (outputTypePort_) {
      makeOutputPortSet(&vcPool_, {vcSet}, numVcSets_, baseVc_ + numVcs_,
                        maxOutputs_, outputAlg_, &outputPorts_);
    } else {
      makeOutputVcSet(&vcPool_, maxOutputs_, outputAlg_, &outputPorts_);
    }
  } else if (decisionScheme_ == DecisionScheme::ST) {
    stagedThreshold(outputVcsMin_, outputVcsDer_,
                    thresholdMin_, thresholdNonMin_,
                    &vcPool_, &takingDeroute);
    if (outputTypePort_) {
      makeOutputPortSet(&vcPool_, {vcSet}, numVcSets_, baseVc_ + numVcs_,
                        maxOutputs_, outputAlg_, &outputPorts_);
    } else {
      makeOutputVcSet(&vcPool_, maxOutputs_, outputAlg_, &outputPorts_);
    }
  } else if (decisionScheme_ == DecisionScheme::TW) {
      thresholdWeighted(outputVcsMin_, outputVcsDer_,
                        hops, hopIncr, threshold_,
                        &vcPool_, &takingDeroute);
      if (outputTypePort_) {
      makeOutputPortSet(&vcPool_, {vcSet}, numVcSets_, baseVc_ + numVcs_,
                        maxOutputs_, outputAlg_, &outputPorts_);
    } else {
      makeOutputVcSet(&vcPool_, maxOutputs_, outputAlg_, &outputPorts_);
    }
  } else {
    fprintf(stderr, "Unknown decision scheme\n");
    assert(false);
  }

  if (outputPorts_.empty()) {
    u32 basePort = destinationAddress->at(0) * interfacePorts_;
    for (u32 offset = 0; offset < interfacePorts_; offset++) {
      u32 port = basePort + offset;
      for (u64 vc = baseVc_; vc < baseVc_ + numVcs_; vc++) {
        _response->add(port, vc);
      }
    }
    if ((adaptivityType_ == AdaptiveRoutingAlg::DDALP) ||
        (adaptivityType_ == AdaptiveRoutingAlg::DDALV)) {
      delete reinterpret_cast<const std::vector<u32>*>(
          packet->getRoutingExtension());
      packet->setRoutingExtension(nullptr);
    }
  } else {
    for (auto& it : outputPorts_) {
      _response->add(std::get<0>(it), std::get<1>(it));
    }
  }
}

void DalRoutingAlgorithm::vcScheduled(Flit* _flit, u32 _port, u32 _vc) {
  Packet* packet = _flit->packet();
  const std::vector<u32>* destinationAddress =
      packet->message()->getDestinationAddress();
  const std::vector<u32>& routerAddress = router_->address();

  if ((adaptivityType_ != AdaptiveRoutingAlg::DDALP) &&
      (adaptivityType_ != AdaptiveRoutingAlg::DDALV)) {
    return;
  }

  bool derouted = true;
  if (_port < concentration_) {
    return;
  }

  u32 port = _port - concentration_;
  u32 minOffset = 0;
  u32 dim;
  for (dim = 0; dim < dimensionWidths_.size(); dim++) {
    u32 src = routerAddress.at(dim);
    u32 dst = destinationAddress->at(dim + 1);

    if (dst > src) {
      minOffset = dst - src;
    } else {
      minOffset = ((dst + dimensionWidths_.at(dim)) - src);
    }
    u32 dimPorts = (dimensionWidths_.at(dim) - 1) * dimensionWeights_.at(dim);
    if (port < dimPorts) {
      break;
    } else {
      port -= dimPorts;
    }
  }
  // offset starts at 1
  u32 offsetTaken = port / dimensionWeights_.at(dim) + 1;
  if (offsetTaken == minOffset) {
    // port goes to minimal router no deroute
    derouted = false;
  }


  // mark deroute
  std::vector<u32>* deroutedDims =
      reinterpret_cast<std::vector<u32>*>(packet->getRoutingExtension());
  if (derouted) {
    assert(deroutedDims->at(dim) == 0);
    deroutedDims->at(dim)++;
  }
}

}  // namespace HyperX

registerWithObjectFactory("dal", HyperX::RoutingAlgorithm,
                    HyperX::DalRoutingAlgorithm,
                    HYPERX_ROUTINGALGORITHM_ARGS);

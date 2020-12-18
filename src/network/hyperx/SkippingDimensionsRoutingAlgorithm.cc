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
#include "network/hyperx/SkippingDimensionsRoutingAlgorithm.h"

#include <factory/ObjectFactory.h>

#include <cassert>

#include "types/Message.h"
#include "types/Packet.h"

namespace HyperX {

SkippingDimensionsRoutingAlgorithm::SkippingDimensionsRoutingAlgorithm(
    const std::string& _name, const Component* _parent, Router* _router,
    u32 _baseVc, u32 _numVcs, u32 _inputPort, u32 _inputVc,
    const std::vector<u32>& _dimensionWidths,
    const std::vector<u32>& _dimensionWeights,
    u32 _concentration, u32 _interfacePorts, nlohmann::json _settings)
    : RoutingAlgorithm(_name, _parent, _router, _baseVc, _numVcs,
                       _inputPort, _inputVc, _dimensionWidths,
                       _dimensionWeights, _concentration, _interfacePorts,
                       _settings) {
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

  // skipping rounds + finishing round
  assert(_settings.contains("num_rounds") &&
         _settings["num_rounds"].is_number_integer());
  numRounds_ = _settings["num_rounds"].get<u32>();
  assert(numRounds_ > 0);

  // step = dimensional bias
  assert(_settings.contains("step"));
  step_ = _settings["step"].get<f64>();


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

  assert(_settings.contains("skipping_algorithm"));
  if (_settings["skipping_algorithm"].get<std::string>() == "dimension_adaptive") {
    if (outputType == "vc") {
      skippingType_ = SkippingRoutingAlg::DOALV;
    } else if (outputType == "port") {
      skippingType_ = SkippingRoutingAlg::DOALP;
    } else {
      fprintf(stderr, "Unknown output type:");
      fprintf(stderr, " '%s'\n", outputType.c_str());
      assert(false);
    }
  } else if (_settings["skipping_algorithm"].get<std::string>() == "dimension_order") {
    if (outputType == "vc") {
      skippingType_ = SkippingRoutingAlg::DORV;
    } else if (outputType == "port") {
      skippingType_ = SkippingRoutingAlg::DORP;
    } else {
      fprintf(stderr, "Unknown output type:");
      fprintf(stderr, " '%s'\n", outputType.c_str());
      assert(false);
    }
  } else {
    fprintf(stderr, "Unknown adaptive algorithm:");
    fprintf(stderr, " '%s'\n",
            _settings["skipping_algorithm"].get<std::string>().c_str());
    assert(false);
  }

  assert(_settings.contains("finishing_algorithm"));
  if (_settings["finishing_algorithm"].get<std::string>() == "dimension_adaptive") {
    if (outputType == "vc") {
      finishingType_ = SkippingRoutingAlg::DOALV;
    } else if (outputType == "port") {
      finishingType_ = SkippingRoutingAlg::DOALP;
    } else {
      fprintf(stderr, "Unknown output type:");
      fprintf(stderr, " '%s'\n", outputType.c_str());
      assert(false);
    }
  } else if (_settings["finishing_algorithm"].get<std::string>() == "dimension_order") {
    if (outputType == "vc") {
      finishingType_ = SkippingRoutingAlg::DORV;
    } else if (outputType == "port") {
      finishingType_ = SkippingRoutingAlg::DORP;
    } else {
      fprintf(stderr, "Unknown output type:");
      fprintf(stderr, " '%s'\n", outputType.c_str());
      assert(false);
    }
  } else {
    fprintf(stderr, "Unknown adaptive algorithm:");
    fprintf(stderr, " '%s'\n",
            _settings["finishing_algorithm"].get<std::string>().c_str());
    assert(false);
  }

  if (_settings["decision_scheme"].get<std::string>() == "monolithic_weighted") {
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
      fprintf(stderr, " '%s'\n", _settings["bias_mode"].get<std::string>().c_str());
      assert(false);
    }
  } else if (_settings["decision_scheme"].get<std::string>() == "staged_threshold") {
    decisionScheme_ = DecisionScheme::ST;
    assert(_settings.contains("threshold_min"));
    assert(_settings.contains("threshold_nonmin"));
    thresholdMin_ = _settings["threshold_min"].get<f64>();
    thresholdNonMin_ = _settings["threshold_nonmin"].get<f64>();
  } else if (_settings["decision_scheme"].get<std::string>() == "threshold_weighted") {
    decisionScheme_ = DecisionScheme::TW;
    assert(_settings.contains("threshold"));
    threshold_ = _settings["threshold"].get<f64>();
  } else {
    fprintf(stderr, "Unknown decision scheme:");
    fprintf(stderr, " '%s'\n", _settings["decision_scheme"].get<std::string>().c_str());
    assert(false);
  }

  if ((skippingType_ == SkippingRoutingAlg::DORV) ||
      (skippingType_ == SkippingRoutingAlg::DORP)) {
    numVcSets_ = 1 * (numRounds_ - 1);
  } else if ((skippingType_ == SkippingRoutingAlg::DOALV) ||
             (skippingType_ == SkippingRoutingAlg::DOALP)) {
    numVcSets_ = 2 * (numRounds_ - 1);
  } else {
    fprintf(stderr, "Unknown skipping algorithm\n");
    assert(false);
  }

  if ((finishingType_ == SkippingRoutingAlg::DORV) ||
      (finishingType_ == SkippingRoutingAlg::DORP)) {
    numVcSets_ += 1;
  } else if ((finishingType_ == SkippingRoutingAlg::DOALV) ||
             (finishingType_ == SkippingRoutingAlg::DOALP)) {
    numVcSets_ += 2;
  } else {
    fprintf(stderr, "Unknown skipping algorithm\n");
    assert(false);
  }

  assert(_numVcs >= numVcSets_);
}

SkippingDimensionsRoutingAlgorithm::~SkippingDimensionsRoutingAlgorithm() {}

void SkippingDimensionsRoutingAlgorithm::processRequest(
    Flit* _flit, RoutingAlgorithm::Response* _response) {
  Packet* packet = _flit->packet();
  const std::vector<u32>* destinationAddress =
      packet->message()->getDestinationAddress();
  const std::vector<u32>& routerAddress = router_->address();

  u32 numRound = 0;
  if (packet->getHopCount() > 0) {
    numRound = (_flit->getVc() - baseVc_) % numVcSets_;
  }
  if ((skippingType_ == SkippingRoutingAlg::DOALV) ||
      (skippingType_ == SkippingRoutingAlg::DOALP)) {
    numRound /= 2;
  }

  u32 inDim = computeInputPortDim(dimensionWidths_, dimensionWeights_,
                                  concentration_, inputPort_);
  if (inDim == U32_MAX) {
    inDim = 0;
  } else {
    inDim += 1;
  }

  u32 hops = hopsLeft(router_, destinationAddress);
  u32 hopIncr = U32_MAX;
  if (hopCountMode_ == HopCountMode::ABS) {
    hopIncr = 1;
  } else if (hopCountMode_ == HopCountMode::NORM) {
    hopIncr = hops;
  }

  // We need a new base VC with offset to determine if we have taken non-minimal
  // route in case of DOAL
  u32 vcSet = U32_MAX;
  u32 baseVc = U32_MAX;

  if (numRound < (numRounds_ - 1)) {
    // Skipping dimensions round
    if ((skippingType_ == SkippingRoutingAlg::DOALV) ||
        (skippingType_ == SkippingRoutingAlg::DOALP)) {
      baseVc = baseVc_ + numRound * 2;
      // first hop in dimension is always VC = 0
      // if incoming dimension is unaligned, hence it is a deroute, than VC = 1
      if ((inDim == 0) ||
          (routerAddress.at(inDim - 1) == destinationAddress->at(inDim))) {
        vcSet = baseVc;
        skippingDimOrderRoutingOutput(
            router_, inputPort_, inputVc_, dimensionWidths_, dimensionWeights_,
            concentration_, interfacePorts_, destinationAddress, inDim, baseVc,
            vcSet, numVcSets_, baseVc_ + numVcs_, _flit, iBias_, cBias_, step_,
            threshold_, thresholdMin_, thresholdNonMin_,
            skippingType_, decisionScheme_, hopCountMode_,
            &outputVcs1_, &outputVcs2_, &outputVcs3_, &vcPool_);
      } else {
        vcSet = baseVc + 1;
        // We need to use fake destination here the same way we use it in
        // skipping util function
        std::vector<u32> fakeDestinationAddress(*destinationAddress);
        if (inDim > 0) {
          for (u32 dim = 1; dim < inDim; dim++) {
            fakeDestinationAddress.at(dim) = routerAddress.at(dim - 1);
          }
        }
        if (skippingType_ == SkippingRoutingAlg::DOALV) {
          doalVcRoutingOutput(
              router_, inputPort_, inputVc_, dimensionWidths_,
              dimensionWeights_, concentration_, interfacePorts_,
              &fakeDestinationAddress, baseVc, vcSet, numVcSets_,
              baseVc_ + numVcs_, _flit, &outputVcs1_, &outputVcs2_);
        } else if (skippingType_ == SkippingRoutingAlg::DOALP) {
          doalPortRoutingOutput(
              router_, inputPort_, inputVc_, dimensionWidths_,
              dimensionWeights_, concentration_, interfacePorts_,
              destinationAddress, baseVc, vcSet, numVcSets_, baseVc_ + numVcs_,
              _flit, &outputVcs1_, &outputVcs2_);
        } else {
          fprintf(stderr, "Invalid skipping routing algorithm\n");
          assert(false);
        }

        bool NAtakingDeroute = false;
        if (decisionScheme_ == DecisionScheme::MW) {
          monolithicWeighted(outputVcs1_, outputVcs2_,
                             hops, hopIncr, iBias_, cBias_, biasMode_,
                             &vcPool_, &NAtakingDeroute);
        } else if (decisionScheme_ == DecisionScheme::ST) {
          stagedThreshold(outputVcs1_, outputVcs2_,
                          thresholdMin_, thresholdNonMin_,
                          &vcPool_, &NAtakingDeroute);
        } else if (decisionScheme_ == DecisionScheme::TW) {
          thresholdWeighted(outputVcs1_, outputVcs2_,
                            hops, hopIncr, threshold_,
                            &vcPool_, &NAtakingDeroute);
        } else {
          fprintf(stderr, "Invalid decision scheme\n");
          assert(false);
        }
        assert(!vcPool_.empty());
      }
    } else if ((skippingType_ == SkippingRoutingAlg::DORV) ||
               (skippingType_ == SkippingRoutingAlg::DORP)) {
      baseVc = baseVc_ + numRound;
      vcSet = baseVc;
      skippingDimOrderRoutingOutput(
          router_, inputPort_, inputVc_, dimensionWidths_, dimensionWeights_,
          concentration_, interfacePorts_, destinationAddress, inDim, baseVc,
          vcSet, numVcSets_, baseVc_ + numVcs_, _flit, iBias_, cBias_, step_,
          threshold_, thresholdMin_, thresholdNonMin_,
          skippingType_, decisionScheme_, hopCountMode_,
          &outputVcs1_, &outputVcs2_, &outputVcs3_, &vcPool_);
    }

    // Round increment if out of dimensions in this round
    if (vcPool_.empty() && !isDestinationRouter(router_, destinationAddress)) {
      numRound += 1;
      if ((skippingType_ == SkippingRoutingAlg::DORV) ||
          (skippingType_ == SkippingRoutingAlg::DORP)) {
        baseVc = baseVc_ + numRound;
      } else if ((skippingType_ == SkippingRoutingAlg::DOALV) ||
                 (skippingType_ == SkippingRoutingAlg::DOALP)) {
        baseVc = baseVc_ + numRound * 2;
      } else {
        fprintf(stderr, "Unknown skipping algorithm\n");
        assert(false);
      }
      vcSet = baseVc;

      if (numRound < (numRounds_ - 1)) {
        skippingDimOrderRoutingOutput(
            router_, inputPort_, inputVc_, dimensionWidths_, dimensionWeights_,
            concentration_, interfacePorts_, destinationAddress, 0, baseVc,
            vcSet, numVcSets_, baseVc_ + numVcs_, _flit, iBias_, cBias_, step_,
            threshold_, thresholdMin_, thresholdNonMin_,
            skippingType_, decisionScheme_, hopCountMode_, &outputVcs1_,
            &outputVcs2_, &outputVcs3_, &vcPool_);

        bool NAtakingDeroute = false;
        if (decisionScheme_ == DecisionScheme::MW) {
          monolithicWeighted(outputVcs1_, outputVcs2_,
                             hops, hopIncr, iBias_, cBias_, biasMode_,
                             &vcPool_, &NAtakingDeroute);
        } else if (decisionScheme_ == DecisionScheme::ST) {
          stagedThreshold(outputVcs1_, outputVcs2_,
                          thresholdMin_, thresholdNonMin_,
                          &vcPool_, &NAtakingDeroute);
        } else if (decisionScheme_ == DecisionScheme::TW) {
          thresholdWeighted(outputVcs1_, outputVcs2_,
                            hops, hopIncr, threshold_,
                            &vcPool_, &NAtakingDeroute);
        } else {
          fprintf(stderr, "Invalid decision scheme\n");
          assert(false);
        }
      } else {
        assert(numRound < numRounds_);
        if ((skippingType_ == SkippingRoutingAlg::DOALV) ||
            (skippingType_ == SkippingRoutingAlg::DOALP)) {
          baseVc = baseVc_ + (numRounds_ - 1) * 2;
        } else if ((skippingType_ == SkippingRoutingAlg::DORV) ||
                   (skippingType_ == SkippingRoutingAlg::DORP)) {
          baseVc = baseVc_ + numRounds_ - 1;
        } else {
          fprintf(stderr, "Unknown skipping algorithm\n");
          assert(false);
        }
        vcSet = baseVc;
        finishingDimOrderRoutingOutput(
            router_, inputPort_, inputVc_, dimensionWidths_, dimensionWeights_,
            concentration_, interfacePorts_, destinationAddress, baseVc, vcSet,
            numVcSets_, baseVc_ + numVcs_, _flit, iBias_, cBias_,
            threshold_, thresholdMin_, thresholdNonMin_,
            finishingType_, decisionScheme_, hopCountMode_, &outputVcs1_,
            &outputVcs2_, &vcPool_);
      }
      // check (what is this?)
      assert(((decisionScheme_ == DecisionScheme::ST) && (inDim == 0)) ||
             ((inDim != 0) &&
              (routerAddress.at(inDim - 1) == destinationAddress->at(inDim))));
    }
  } else {
    // Finishing round
    if ((skippingType_ == SkippingRoutingAlg::DOALV) ||
        (skippingType_ == SkippingRoutingAlg::DOALP)) {
      baseVc = baseVc_ + (numRounds_ - 1) * 2;
    } else if ((skippingType_ == SkippingRoutingAlg::DORV) ||
               (skippingType_ == SkippingRoutingAlg::DORP)) {
      baseVc = baseVc_ + numRounds_ - 1;
    } else {
      fprintf(stderr, "Unknown skipping algorithm\n");
      assert(false);
    }

    if ((finishingType_ == SkippingRoutingAlg::DORV) ||
        (finishingType_ == SkippingRoutingAlg::DORP)) {
      vcSet = baseVc;
    } else if ((finishingType_ == SkippingRoutingAlg::DOALV) ||
               (finishingType_ == SkippingRoutingAlg::DOALP)) {
      if ((inDim == 0) ||
          (routerAddress.at(inDim - 1) == destinationAddress->at(inDim))) {
        vcSet = baseVc;
      } else {
        vcSet = baseVc + 1;
      }
    } else {
      fprintf(stderr, "Unknown finishing algorithm\n");
      assert(false);
    }

    finishingDimOrderRoutingOutput(
        router_, inputPort_, inputVc_, dimensionWidths_, dimensionWeights_,
        concentration_, interfacePorts_, destinationAddress, baseVc, vcSet,
        numVcSets_, baseVc_ + numVcs_, _flit, iBias_, cBias_,
        threshold_, thresholdMin_, thresholdNonMin_,
        finishingType_, decisionScheme_, hopCountMode_,
        &outputVcs1_, &outputVcs2_, &vcPool_);
  }

  if ((finishingType_ == SkippingRoutingAlg::DOALP) ||
      (finishingType_ == SkippingRoutingAlg::DORP)) {
    makeOutputPortSet(&vcPool_, {vcSet}, numVcs_, baseVc_ + numVcs_,
                      maxOutputs_, outputAlg_, &outputPorts_);
  } else if ((finishingType_ == SkippingRoutingAlg::DOALV) ||
             (finishingType_ == SkippingRoutingAlg::DORV)) {
    makeOutputVcSet(&vcPool_, maxOutputs_, outputAlg_, &outputPorts_);
  } else {
    fprintf(stderr, "Unknown finishing algorithm\n");
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
  } else {
    for (auto& it : outputPorts_) {
      _response->add(std::get<0>(it), std::get<1>(it));
    }
  }
}

}  // namespace HyperX

registerWithObjectFactory("skipping_dimensions", HyperX::RoutingAlgorithm,
                    HyperX::SkippingDimensionsRoutingAlgorithm,
                    HYPERX_ROUTINGALGORITHM_ARGS);

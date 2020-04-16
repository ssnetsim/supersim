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
#include "congestion/BufferOccupancy.h"

#include <factory/ObjectFactory.h>

#include <cassert>
#include <cmath>

#include <algorithm>

namespace {
const s32 INCR = 0x50;
const s32 DECR = 0xAF;
const s32 PHANTOM = 0x87;
}  // namespace

BufferOccupancy::BufferOccupancy(
    const std::string& _name, const Component* _parent, PortedDevice* _device,
    Json::Value _settings)
    : CongestionSensor(_name, _parent, _device, _settings),
      latency_(_settings["latency"].asUInt()),
      mode_(parseMode(_settings["mode"].asString())) {
  assert(latency_ > 0);
  u32 totalVcs = numPorts_ * numVcs_;
  normalizationDivisors_.resize(totalVcs, 0);
  outstandingFlits_.resize(totalVcs, 0);

  // phantom is an optional setting
  phantom_ = false;
  if (_settings.isMember("phantom")) {
    assert(_settings["phantom"].isBool());
    phantom_ = _settings["phantom"].asBool();
    if (phantom_) {
      assert(_settings.isMember("value_coeff") &&
             _settings["value_coeff"].isDouble());
      assert(_settings.isMember("length_coeff") &&
             _settings["length_coeff"].isDouble());
      valueCoeff_ = _settings["value_coeff"].asDouble();
      lengthCoeff_ = _settings["length_coeff"].asDouble();
      windows_.resize(totalVcs, 0);
    }
  }
}

BufferOccupancy::~BufferOccupancy() {
  u32 totalVcs = numPorts_ * numVcs_;
  for (u32 vc = 0; vc < totalVcs; vc++) {
    assert(outstandingFlits_.at(vc) == 0);
    assert(!phantom_ || windows_.at(vc) == 0);
  }
}

void BufferOccupancy::initCredits(u32 _vcIdx, u32 _credits) {
  u32 port, vc;
  device_->vcIndexInv(_vcIdx, &port, &vc);

  assert(port < numPorts_);
  assert(vc < numVcs_);
  if (style() == CongestionSensor::Style::kNormalized) {
    assert(_credits > 0);
  }

  normalizationDivisors_.at(_vcIdx) = _credits;
}

void BufferOccupancy::incrementCredit(u32 _vcIdx) {
  createEvent(_vcIdx, INCR);
}

void BufferOccupancy::decrementCredit(u32 _vcIdx) {
  createEvent(_vcIdx, DECR);
}

void BufferOccupancy::processEvent(void* _event, s32 _type) {
  assert(gSim->epsilon() > 0);
  u32 vcIdx = static_cast<u32>(reinterpret_cast<u64>(_event));
  switch (_type) {
    case INCR:
      performIncrementCredit(vcIdx);
      break;
    case DECR:
      performDecrementCredit(vcIdx);
      break;
    case PHANTOM:
      performDecrementWindow(vcIdx);
      break;
    default:
      assert(false);
  }
}

CongestionSensor::Style BufferOccupancy::style() const {
  switch (mode_) {
    case BufferOccupancy::Mode::kVcNorm:
    case BufferOccupancy::Mode::kPortNorm:
    case BufferOccupancy::Mode::kMinNorm:
    case BufferOccupancy::Mode::kMaxNorm:
      return CongestionSensor::Style::kNormalized;
      break;
    case BufferOccupancy::Mode::kVcAbs:
    case BufferOccupancy::Mode::kPortAbs:
    case BufferOccupancy::Mode::kMinAbs:
    case BufferOccupancy::Mode::kMaxAbs:
      return CongestionSensor::Style::kAbsolute;
      break;
    default:
      assert(false);
      break;
  }
}

CongestionSensor::Resolution BufferOccupancy::resolution() const {
  switch (mode_) {
    case BufferOccupancy::Mode::kVcNorm:
    case BufferOccupancy::Mode::kVcAbs:
    case BufferOccupancy::Mode::kMinNorm:
    case BufferOccupancy::Mode::kMinAbs:
    case BufferOccupancy::Mode::kMaxNorm:
    case BufferOccupancy::Mode::kMaxAbs:
      return CongestionSensor::Resolution::kVc;
      break;
    case BufferOccupancy::Mode::kPortNorm:
    case BufferOccupancy::Mode::kPortAbs:
      return CongestionSensor::Resolution::kPort;
      break;
    default:
      assert(false);
      break;
  }
}

f64 BufferOccupancy::computeStatus(
    u32 _inputPort, u32 _inputVc, u32 _outputPort, u32 _outputVc) const {
  switch (mode_) {
    case BufferOccupancy::Mode::kVcNorm: {
      return vcStatus(_outputPort, _outputVc, true);
      break;
    }
    case BufferOccupancy::Mode::kVcAbs: {
      return vcStatus(_outputPort, _outputVc, false);
      break;
    }
    case BufferOccupancy::Mode::kPortNorm: {
      return portAverageStatus(_outputPort, true);
      break;
    }
    case BufferOccupancy::Mode::kPortAbs: {
      return portAverageStatus(_outputPort, false);
      break;
    }
    case BufferOccupancy::Mode::kMinNorm: {
      f64 vcSts = vcStatus(_outputPort, _outputVc, true);
      f64 portSts = portAverageStatus(_outputPort, true);
      return std::min(vcSts, portSts);
      break;
    }
    case BufferOccupancy::Mode::kMinAbs: {
      f64 vcSts = vcStatus(_outputPort, _outputVc, false);
      f64 portSts = portAverageStatus(_outputPort, false);
      return std::min(vcSts, portSts);
    }
    case BufferOccupancy::Mode::kMaxNorm: {
      f64 vcSts = vcStatus(_outputPort, _outputVc, true);
      f64 portSts = portAverageStatus(_outputPort, true);
      return std::max(vcSts, portSts);
      break;
    }
    case BufferOccupancy::Mode::kMaxAbs: {
      f64 vcSts = vcStatus(_outputPort, _outputVc, false);
      f64 portSts = portAverageStatus(_outputPort, false);
      return std::max(vcSts, portSts);
    }
    default:
      assert(false);
      break;
  }
}

BufferOccupancy::Mode BufferOccupancy::parseMode(const std::string& _mode) {
  if (_mode == "normalized_vc") {
    return BufferOccupancy::Mode::kVcNorm;
  } else if (_mode == "absolute_vc") {
    return  BufferOccupancy::Mode::kVcAbs;
  } else if (_mode == "normalized_port") {
    return  BufferOccupancy::Mode::kPortNorm;
  } else if (_mode == "absolute_port") {
    return  BufferOccupancy::Mode::kPortAbs;
  } else if (_mode == "normalized_min") {
    return  BufferOccupancy::Mode::kMinNorm;
  } else if (_mode == "absolute_min") {
    return  BufferOccupancy::Mode::kMinAbs;
  } else if (_mode == "normalized_max") {
    return  BufferOccupancy::Mode::kMaxNorm;
  } else if (_mode == "absolute_max") {
    return  BufferOccupancy::Mode::kMaxAbs;
  } else {
    assert(false);
  }
}

void BufferOccupancy::createEvent(u32 _vcIdx, s32 _type) {
  assert(gSim->epsilon() > 0);
  u64 time = latency_ == 1 ? gSim->time() :
             gSim->futureCycle(Simulator::Clock::ROUTER, latency_ - 1);
  addEvent(time, gSim->epsilon() + 1, reinterpret_cast<void*>(_vcIdx), _type);
}

void BufferOccupancy::performIncrementCredit(u32 _vcIdx) {
  outstandingFlits_.at(_vcIdx)--;
}

void BufferOccupancy::performDecrementCredit(u32 _vcIdx) {
  outstandingFlits_.at(_vcIdx)++;

  if (phantom_) {
    windows_.at(_vcIdx)++;
    u32 port, vc;
    device_->vcIndexInv(_vcIdx, &port, &vc);
    Channel* ch = device_->getOutputChannel(port);
    u32 windowLength = (u32)(ch->latency() * lengthCoeff_);
    u64 time = gSim->futureCycle(Simulator::Clock::CHANNEL, windowLength);
    addEvent(time, gSim->epsilon(), reinterpret_cast<void*>(_vcIdx), PHANTOM);
  }
}

void BufferOccupancy::performDecrementWindow(u32 _vcIdx) {
  assert(phantom_);
  assert(windows_.at(_vcIdx) > 0);
  windows_.at(_vcIdx)--;
}

f64 BufferOccupancy::vcStatus(
    u32 _outputPort, u32 _outputVc, bool _normalize) const {
  // return this VC's status
  u32 vcIdx = device_->vcIndex(_outputPort, _outputVc);
  f64 status = outstandingFlits_.at(vcIdx);
  if (phantom_) {
    status = std::max(0.0, status - (windows_.at(vcIdx) * valueCoeff_));
  }
  if (_normalize) {
    assert(normalizationDivisors_.at(vcIdx) > 0);
    status /= normalizationDivisors_.at(vcIdx);
  }
  return status;
}

f64 BufferOccupancy::portAverageStatus(u32 _outputPort, bool _normalize) const {
  // return the average status of all VCs in this port (normalized)
  f64 status = 0.0;
  for (u32 vc = 0; vc < numVcs_; vc++) {
    status += vcStatus(_outputPort, vc, _normalize);
  }
  return status / numVcs_;
}

registerWithObjectFactory("buffer_occupancy", CongestionSensor,
                          BufferOccupancy, CONGESTIONSENSOR_ARGS);

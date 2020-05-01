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
#ifndef CONGESTION_BUFFEROCCUPANCY_H_
#define CONGESTION_BUFFEROCCUPANCY_H_

#include <json/json.h>
#include <prim/prim.h>

#include <string>
#include <vector>

#include "congestion/CongestionSensor.h"

class BufferOccupancy : public CongestionSensor {
 public:
  BufferOccupancy(const std::string& _name, const Component* _parent,
                  PortedDevice* _device, Json::Value _settings);
  ~BufferOccupancy();

  // CreditWatcher interface
  void initCredits(u32 _vcIdx, u32 _credits) override;
  void incrementCredit(u32 _vcIdx) override;  // a credit came from downstream
  void decrementCredit(u32 _vcIdx) override;  // a credit was consumed locally

  // this creates INCR and DECR events to simulate a fixed latency between all
  //  input and output ports (IOW, input port and VC are ignored in the calc).
  void processEvent(void* _event, s32 _type) override;

  // style and resolution reporting
  CongestionSensor::Style style() const override;
  CongestionSensor::Resolution resolution() const override;

 protected:
  // see CongestionSensor::computeStatus
  f64 computeStatus(u32 _inputPort, u32 _inputVc, u32 _outputPort,
                    u32 _outputVc) const override;

 private:
  enum class Mode {kVcNorm, kPortNorm, kMinNorm, kMaxNorm, kVcAbs, kPortAbs,
                   kMinAbs, kMaxAbs};

  static Mode parseMode(const std::string& _mode);

  void createEvent(u32 _vcIdx, s32 _type);
  void performIncrementCredit(u32 _vcIdx);
  void performDecrementCredit(u32 _vcIdx);
  void performDecrementWindow(u32 _vcIdx);

  f64 vcStatus(u32 _outputPort, u32 _outputVc, bool _normalize) const;
  f64 portAverageStatus(u32 _outputPort, bool _normalize) const;

  const u32 latency_;
  const Mode mode_;

  // 64-bit to hold U32_MAX
  std::vector<s64> normalizationDivisors_;
  std::vector<s64> outstandingFlits_;

  // phantom congestion awareness
  bool phantom_;
  f64 valueCoeff_;
  f64 lengthCoeff_;
  std::vector<u32> windows_;
};

#endif  // CONGESTION_BUFFEROCCUPANCY_H_

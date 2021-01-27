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
#ifndef CONGESTION_CONGESTIONSENSOR_H_
#define CONGESTION_CONGESTIONSENSOR_H_

#include <string>
#include <vector>

#include "architecture/CreditWatcher.h"
#include "architecture/PortedDevice.h"
#include "event/Component.h"
#include "nlohmann/json.hpp"
#include "prim/prim.h"

#define CONGESTIONSENSOR_ARGS const std::string&, const Component*, \
    PortedDevice*, nlohmann::json

class CongestionSensor : public Component, public CreditWatcher {
 public:
  // Defines the style of measurements
  enum class Style {
    kNull,       // no measurements taken
    kAbsolute,   // positive values of outstanding flits
    kNormalized  // positive values of normalized outstanding flits
  };

  // Defines the resolution of measurement
  enum class Resolution {
    kNull,  // no measurements taken
    kVc,    // values specified per VC
    kPort   // values specified per port (_outputVc is meaningless)
  };

  CongestionSensor(const std::string& _name, const Component* _parent,
                   PortedDevice* _device, nlohmann::json _settings);
  virtual ~CongestionSensor();

  // this is a congestion status factory
  static CongestionSensor* create(CONGESTIONSENSOR_ARGS);

  // this returns congestion status (i.e. 0=empty 1=congested)
  f64 status(u32 _inputPort, u32 _inputVc, u32 _outputPort,
             u32 _outputVc) const;  // (must be epsilon >= 1)

  // must tell your style and mode
  virtual Style style() const = 0;
  virtual Resolution resolution() const = 0;

 protected:
  // this must be implemented by subclasses to yield the congestion status
  //  this MUST return a value >= 0.0
  virtual f64 computeStatus(u32 _inputPort, u32 _inputVc,
                            u32 _outputPort, u32 _outputVc) const = 0;

  PortedDevice* device_;
  const u32 numPorts_;
  const u32 numVcs_;

 private:
  const u32 granularity_;
  const f64 minimum_;
  const f64 offset_;
};

#endif  // CONGESTION_CONGESTIONSENSOR_H_

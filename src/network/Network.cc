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
#include "network/Network.h"

#include <cassert>

#include <utility>

#include "factory/ObjectFactory.h"

static u32 computeNumVcs(const nlohmann::json& _pcs) {
  u32 sum = 0;
  for (u32 pc = 0; pc < _pcs.size(); pc++) {
    assert(_pcs[pc].contains("num_vcs") &&
           _pcs[pc]["num_vcs"].is_number_integer() &&
           _pcs[pc]["num_vcs"].get<u32>() > 0);
    sum += _pcs[pc]["num_vcs"].get<u32>();
  }
  return sum;
}

Network::Network(const std::string& _name, const Component* _parent,
                 MetadataHandler* _metadataHandler, nlohmann::json _settings)
    : Component(_name, _parent),
      numVcs_(computeNumVcs(_settings["protocol_classes"])),
      metadataHandler_(_metadataHandler),
      monitoring_(false) {
  // check settings
  assert(numVcs_ > 0);

  // create a channel log object
  channelLog_ = new ChannelLog(numVcs_, _settings["channel_log"]);

  // create a traffic log object
  trafficLog_ = new TrafficLog(_settings["traffic_log"]);
}

Network::~Network() {
  delete channelLog_;
  delete trafficLog_;
}

Network* Network::create(
    const std::string& _name, const Component* _parent,
    MetadataHandler* _metadataHandler, nlohmann::json _settings) {
  // retrieve the topology
  const std::string& topology = _settings["topology"].get<std::string>();

  // attempt to build the network topology
  Network* network = factory::ObjectFactory<Network, NETWORK_ARGS>::create(
      topology, _name, _parent, _metadataHandler, _settings);

  // check that the factory had the topology
  if (network == nullptr) {
    fprintf(stderr, "unknown network topology: %s\n", topology.c_str());
    assert(false);
  }
  return network;
}

MetadataHandler* Network::metadataHandler() const {
  return metadataHandler_;
}

void Network::startMonitoring() {
  monitoring_ = true;
  std::vector<Channel*> channels;
  collectChannels(&channels);
  for (auto it = channels.begin(); it != channels.end(); ++it) {
    Channel* c = *it;
    c->startMonitoring();
  }
}

void Network::endMonitoring() {
  monitoring_ = false;
  std::vector<Channel*> channels;
  collectChannels(&channels);
  for (auto it = channels.begin(); it != channels.end(); ++it) {
    Channel* c = *it;
    c->endMonitoring();
    channelLog_->logChannel(c);
  }
}

bool Network::monitoring() const {
  return monitoring_;
}

u32 Network::numPcs() const {
  return pcVcs_.size();
}

u32 Network::numVcs() const {
  return numVcs_;
}

Network::PcVcInfo Network::pcVcs(u32 _pc) const {
  assert(_pc < pcVcs_.size());
  return pcVcs_.at(_pc);
}

u32 Network::vcToPc(u32 _vc) const {
  assert(_vc < numVcs_);
  return vcToPc_.at(_vc);
}

void Network::logTraffic(const Component* _device, u32 _inputPort, u32 _inputVc,
                         u32 _outputPort, u32 _outputVc, u32 _flits) {
  if (monitoring_) {
    trafficLog_->logTraffic(_device, _inputPort, _inputVc, _outputPort,
                            _outputVc, _flits);
  }
}

void Network::loadProtocolClassInfo(nlohmann::json _settings) {
  // parse the protocol classes description
  for (u32 pc = 0, vcs = 0; pc < _settings.size(); pc++) {
    Network::PcVcInfo pcVcInfo;
    pcVcInfo.numVcs = _settings[pc]["num_vcs"].get<u32>();
    pcVcInfo.baseVc = vcs;
    pcVcs_.push_back(pcVcInfo);
    Network::PcSettings pcCfg;
    pcCfg.numVcs = pcVcInfo.numVcs;
    pcCfg.baseVc = pcVcInfo.baseVc;
    pcCfg.injection = _settings[pc]["injection"];
    assert(!pcCfg.injection.is_null());
    pcCfg.routing = _settings[pc]["routing"];
    assert(!pcCfg.routing.is_null());
    pcSettings_.push_back(pcCfg);
    for (u32 vc = 0; vc < pcVcInfo.numVcs; vc++, vcs++) {
      bool ins = vcToPc_.insert(std::make_pair(vcs, pc)).second;
      (void)ins;  // UNUSED
      assert(ins);
    }
  }
  assert(pcVcs_.size() == _settings.size());
  assert(vcToPc_.size() == numVcs_);
  assert(pcSettings_.size() == _settings.size());
}

const Network::PcSettings& Network::pcSettings(u32 _pc) const {
  assert(_pc < pcVcs_.size());
  return pcSettings_.at(_pc);
}

void Network::clearProtocolClassInfo() {
  pcSettings_.clear();
}

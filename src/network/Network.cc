/*
 * Copyright 2016 Hewlett Packard Enterprise Development LP
 *
 * Licensed under the Apache License, Version 2.0 (the 'License');
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
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

static u32 computeNumVcs(const Json::Value& _trafficClasses) {
  u32 sum = 0;
  for (u32 idx = 0; idx < _trafficClasses.size(); idx++) {
    const Json::Value& trafficClass = _trafficClasses[idx];
    assert(trafficClass.isMember("num_vcs") &&
           trafficClass["num_vcs"].isUInt() &&
           trafficClass["num_vcs"].asUInt() > 0);
    sum += trafficClass["num_vcs"].asUInt();
  }
  return sum;
}

Network::Network(const std::string& _name, const Component* _parent,
                 MetadataHandler* _metadataHandler, Json::Value _settings)
    : Component(_name, _parent),
      numVcs_(computeNumVcs(_settings["traffic_classes"])),
      metadataHandler_(_metadataHandler) {
  // check settings
  assert(numVcs_ > 0);

  // create a channel log object
  channelLog_ = new ChannelLog(numVcs_, _settings["channel_log"]);
}

Network::~Network() {
  delete channelLog_;
}

u32 Network::numVcs() const {
  return numVcs_;
}

MetadataHandler* Network::metadataHandler() const {
  return metadataHandler_;
}

void Network::startMonitoring() {
  std::vector<Channel*> channels;
  collectChannels(&channels);
  for (auto it = channels.begin(); it != channels.end(); ++it) {
    Channel* c = *it;
    c->startMonitoring();
  }
}

void Network::endMonitoring() {
  std::vector<Channel*> channels;
  collectChannels(&channels);
  for (auto it = channels.begin(); it != channels.end(); ++it) {
    Channel* c = *it;
    c->endMonitoring();
    channelLog_->logChannel(c);
  }
}

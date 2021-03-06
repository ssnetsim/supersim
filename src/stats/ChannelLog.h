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
#ifndef STATS_CHANNELLOG_H_
#define STATS_CHANNELLOG_H_

#include <sstream>

#include "fio/OutFile.h"
#include "network/Channel.h"
#include "nlohmann/json.hpp"
#include "prim/prim.h"

class ChannelLog {
 public:
  ChannelLog(u32 _numVcs, nlohmann::json _settings);
  ~ChannelLog();
  void logChannel(const Channel* _channel);

 private:
  const u32 numVcs_;
  fio::OutFile* outFile_;
  std::stringstream ss_;
};

#endif  // STATS_CHANNELLOG_H_

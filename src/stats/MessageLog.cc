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
#include "stats/MessageLog.h"

#include <cassert>
#include <sstream>
#include <string>

#include "event/Simulator.h"
#include "types/Flit.h"
#include "types/Packet.h"

MessageLog::MessageLog(nlohmann::json _settings) : outFile_(nullptr) {
  if (!_settings["file"].is_null()) {
    // create file
    outFile_ = new fio::OutFile(_settings["file"].get<std::string>());
  }
}

MessageLog::~MessageLog() {
  if (outFile_) {
    delete outFile_;
  }
}

void MessageLog::logMessage(const Message* _message) {
  if (outFile_) {
    std::stringstream ss;
    ss << "+M" << ',';
    ss << _message->id() << ',';
    ss << _message->getSourceId() << ',';
    ss << _message->getDestinationId() << ',';
    ss << _message->getTransaction() << ',';
    ss << _message->getProtocolClass() << ',';
    ss << _message->getMinimalHopCount() << ',';
    ss << _message->getOpCode() << '\n';
    for (u32 p = 0; p < _message->numPackets(); p++) {
      Packet* packet = _message->packet(p);
      ss << " +P" << ',';
      ss << packet->id() << ',';
      ss << packet->getHopCount() << '\n';
      for (u32 f = 0; f < packet->numFlits(); f++) {
        Flit* flit = packet->getFlit(f);
        ss << "   F" << ',';
        ss << flit->id() << ',';
        ss << flit->getSendTime() << ',';
        ss << flit->getReceiveTime() << '\n';
      }
      ss << " -P\n";
    }
    ss << "-M\n";
    outFile_->write(ss.str());
  }
}

void MessageLog::startTransaction(u64 _trans) {
  if (outFile_) {
    std::stringstream ss;
    ss << "+T" << ',' << _trans << ',' << gSim->time() << '\n';
    outFile_->write(ss.str());
  }
}

void MessageLog::endTransaction(u64 _trans) {
  if (outFile_) {
    std::stringstream ss;
    ss << "-T" << ',' << _trans << ',' << gSim->time() << '\n';
    outFile_->write(ss.str());
  }
}

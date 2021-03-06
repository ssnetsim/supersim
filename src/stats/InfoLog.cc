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
#include "stats/InfoLog.h"

#include <cassert>

InfoLog::InfoLog(nlohmann::json _settings) : outFile_(nullptr) {
  if (!_settings["file"].is_null()) {
    // create file
    outFile_ = new fio::OutFile(_settings["file"].get<std::string>());
  }
}

InfoLog::~InfoLog() {
  if (outFile_) {
    delete outFile_;
  }
}

void InfoLog::logInfo(const std::string& _name, const std::string& _value) {
  if (outFile_) {
    outFile_->write(_name + ',' + _value + '\n');
  }
}

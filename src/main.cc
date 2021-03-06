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
#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "event/Simulator.h"
#include "event/VectorQueue.h"
#include "metadata/MetadataHandler.h"
#include "network/Network.h"
#include "nlohmann/json.hpp"
#include "settings/settings.h"
#include "workload/Terminal.h"
#include "workload/Workload.h"

s32 main(s32 _argc, char** _argv) {
  // turn off buffered output on stdout and stderr
  setbuf(stdout, nullptr);
  setbuf(stderr, nullptr);

  // get JSON settings
  printf("Reading settings\n");
  nlohmann::json settings;
  settings::commandLine(_argc, _argv, &settings);
  printf("%s\n", settings::toString(settings).c_str());

  // enable debugging on select components
  for (u32 i = 0; i < settings["debug"].size(); i++) {
    std::string componentName = settings["debug"][i].get<std::string>();
    Component::addDebugName(componentName);
  }

  // initialize the discrete event simulator
  printf("Building components\n");
  gSim = new VectorQueue(settings["simulator"]);

  // create a metadata handler
  MetadataHandler* metadataHandler =
      MetadataHandler::create(settings["metadata_handler"]);

  // create a network
  Network* network =
      Network::create("Network", nullptr, metadataHandler, settings["network"]);
  gSim->setNetwork(network);
  u32 numInterfaces = network->numInterfaces();
  u32 numRouters = network->numRouters();
  std::map<u32, u32> routerRadices;
  for (u32 routerId = 0; routerId < numRouters; routerId++) {
    routerRadices[network->getRouter(routerId)->numPorts()] += 1;
  }
  u32 numVcs = network->numVcs();
  u64 numComponents = Component::numComponents();

  gSim->infoLog.logInfo("Endpoints", std::to_string(numInterfaces));
  gSim->infoLog.logInfo("Routers", std::to_string(numRouters));
  for (auto& p : routerRadices) {
    gSim->infoLog.logInfo("Router radix " + std::to_string(p.first),
                          std::to_string(p.second));
  }
  gSim->infoLog.logInfo("VCs", std::to_string(numVcs));
  gSim->infoLog.logInfo("Components", std::to_string(numComponents));

  // create the workload
  Workload* workload =
      new Workload("Workload", nullptr, metadataHandler, settings["workload"]);
  gSim->setWorkload(workload);

  // check that all debug names were authentic
  Component::debugCheck();

  // initialize the components
  printf("Initializing components\n");
  gSim->initialize();

  // run the simulation!
  if (!settings.contains("no_sim") || settings["no_sim"].get<bool>() == false) {
    printf("Simulation beginning\n");
    gSim->simulate();
    printf("Simulation complete\n");
  } else {
    printf("Simulation skipped\n");
  }

  // cleanup the elements created here
  delete network;
  delete workload;
  delete metadataHandler;

  // cleanup the global simulator components
  delete gSim;

  return 0;
}

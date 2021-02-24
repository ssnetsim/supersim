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
#include "workload/paragraph/GraphTerminal.h"

#include <cassert>
#include <utility>

#include "absl/strings/str_format.h"
#include "paragraph/graph/graph.h"
#include "paragraph/graph/instruction.h"
#include "paragraph/graph/opcode.h"
#include "paragraph/shim/macros.h"
#include "types/Flit.h"
#include "types/Packet.h"
#include "workload/paragraph/Application.h"

#define kInstruction (0xAE)
#define kReceive (0x34)

namespace ParaGraph {

GraphTerminal::GraphTerminal(const std::string& _name, const Component* _parent,
                             u32 _id, const std::vector<u32>& _address,
                             ::Application* _app, u64 _seed,
                             nlohmann::json _settings)
    : Terminal(_name, _parent, _id, _address, _app) {
  // Processing cores
  availableCores_ = _settings["cores"].get<u32>();
  assert(availableCores_ >= 1);

  // Protocol class of injection
  assert(!_settings["protocol_class"].is_null());
  protocolClass_ = _settings["protocol_class"].get<u32>();

  // Maximum packet size
  maxPacketSize_ = _settings["max_packet_size"].get<u32>();
  assert(maxPacketSize_ >= 1);

  // Bytes per flit
  bytesPerFlit_ = _settings["bytes_per_flit"].get<u32>();
  assert(bytesPerFlit_ >= 1);

  // Units per second
  unitsPerSecond_ = _settings["units_per_second"].get<u64>();
  assert(unitsPerSecond_ >= 1);

  // Loads the graph
  auto format =
      absl::ParsedFormat<'d'>::New(_settings["graph_file"].get<std::string>());
  assert(format);
  CHECK_OK_AND_ASSIGN(
      std::unique_ptr<paragraph::Graph> graph,
      paragraph::Graph::ReadFromFile(absl::StrFormat(*format, id_)));
  assert(graph->ValidateIndividualized().ok());
  graph_ = std::move(graph);

  // Constructs the scheduler
  CHECK_OK_AND_ASSIGN(std::unique_ptr<paragraph::GraphScheduler> scheduler,
                      paragraph::GraphScheduler::Create(graph_.get()));
  scheduler_ = std::move(scheduler);
  scheduler_->SeedRandom(_seed);
}

GraphTerminal::~GraphTerminal() {}

void GraphTerminal::processEvent(void* _event, s32 _type) {
  assert(_event);
  switch (_type) {
    case kInstruction:
      finishExecution(reinterpret_cast<paragraph::Instruction*>(_event));
      schedule();
      break;
    case kReceive:
      processReceivedMessage(reinterpret_cast<Message*>(_event));
      break;
    default:
      assert(false);
      break;
  }
}

f64 GraphTerminal::percentComplete() const {
  return 0;
}

void GraphTerminal::start() {
  dbgprintf("starting");
  CHECK_OK(scheduler_->Initialize(
      gSim->time() / static_cast<f64>(unitsPerSecond_)));
  schedule();
}

void GraphTerminal::handleDeliveredMessage(Message* _message) {
  application()->workload()->messageLog()->startTransaction(
      _message->getTransaction());
}

void GraphTerminal::handleReceivedMessage(Message* _message) {
  // Schedules an event to handle this on the next epsilon 0
  assert(gSim->epsilon() == 1);
  addEvent(gSim->time() + 1, 0, _message, kReceive);
}

void GraphTerminal::loadReadyInstructions() {
  // Gets all available instructions.
  for (paragraph::Instruction* instruction :
       scheduler_->GetReadyInstructions()) {
    readyInstructions_.push(instruction);
  }
}

void GraphTerminal::schedule() {
  loadReadyInstructions();

  // Schedules instructions on available cores.
  while (!readyInstructions_.empty() && availableCores_ > 0) {
    // Gets the next instruction.
    paragraph::Instruction* instruction = readyInstructions_.front();
    readyInstructions_.pop();
    paragraph::Opcode opcode = instruction->GetOpcode();
    if (!(opcode == paragraph::Opcode::kDelay ||
          opcode == paragraph::Opcode::kInfeed ||
          opcode == paragraph::Opcode::kOutfeed ||
          opcode == paragraph::Opcode::kRecvDone ||
          opcode == paragraph::Opcode::kRecvStart ||
          opcode == paragraph::Opcode::kSendDone ||
          opcode == paragraph::Opcode::kSendStart)) {
      printf("BAD INST: %s(%lu) %s\n", instruction->GetName().c_str(),
             instruction->GetId(),
             paragraph::OpcodeToString(instruction->GetOpcode()).c_str());
    }
    assert(opcode == paragraph::Opcode::kDelay ||
           opcode == paragraph::Opcode::kInfeed ||
           opcode == paragraph::Opcode::kOutfeed ||
           opcode == paragraph::Opcode::kRecvDone ||
           opcode == paragraph::Opcode::kRecvStart ||
           opcode == paragraph::Opcode::kSendDone ||
           opcode == paragraph::Opcode::kSendStart);

    // Informs the scheduler the instruction has started execution.
    scheduler_->InstructionStarted(
        instruction, gSim->time() / static_cast<f64>(unitsPerSecond_));

    // Sets the instruction in the executing set and reserves a core, if needed,
    // for execution of the instruction.
    bool res = executingInstructions_.insert(instruction).second;
    assert(res);

    // handles each instruction type accordingly
    f64 seconds = instruction->GetSeconds();
    if (opcode == paragraph::Opcode::kDelay ||
        opcode == paragraph::Opcode::kInfeed ||
        opcode == paragraph::Opcode::kOutfeed) {
      // Executes the instruction, either now or in the future.
      availableCores_--;
      f64 seconds = instruction->GetSeconds();
      assert(seconds >= 0);
      u64 units = static_cast<u64>(unitsPerSecond_ * seconds);
      if (units == 0) {
        finishExecution(instruction);
        loadReadyInstructions();
      } else {
        addEvent(gSim->time() + units, 0,
                 const_cast<paragraph::Instruction*>(instruction),
                 kInstruction);
      }
    } else {
      // assuming only kDelay, kInfeed, kOutfeed have time
      assert(seconds == 0.0);
      if (opcode == paragraph::Opcode::kRecvStart ||
          opcode == paragraph::Opcode::kSendDone) {
        // RecvStart and SendDone aren't used in this model. This just finishes
        // their execution them immediately.
        finishExecution(instruction);
        loadReadyInstructions();
      } else if (opcode == paragraph::Opcode::kRecvDone) {
        // Registers a 'bytes_out' receive operation from the source.
        // If the message has already been received this finishes the
        // instruction immediately. If not, this registers the expected message
        // to receive.
        u32 source = instruction->PeerId().value();
        assert(source != id_);
        u64 sequence_number = instruction->GetCommunicationTag();
        u32 size = bytesToFlits(instruction->GetBytesOut());
        dbgprintf("Recv from src=%u seq=%lu size=%u inst=%li inst=%s", source,
                  sequence_number, size, instruction->GetId(),
                  instruction->GetName().c_str());
        auto iter = received_.find(std::make_tuple(source, sequence_number));
        if (iter != received_.end()) {
          u32 exp_size = iter->second;
          if (exp_size != size) {
            printf("[%u] instruction id=%lu seq=%lu size=%u exp=%u\n", id_,
                   instruction->GetId(), sequence_number, size, exp_size);
          }
          assert(exp_size == size);
          received_.erase(iter);
          finishExecution(instruction);
          loadReadyInstructions();
        } else {
          assert(expecting_
                     .emplace(std::make_tuple(source, sequence_number),
                              std::make_tuple(instruction, size))
                     .second);
        }
      } else if (opcode == paragraph::Opcode::kSendStart) {
        // Sends a 'bytes_in' sized message to the destination
        u32 destination = instruction->PeerId().value();
        u64 sequence_number = instruction->GetCommunicationTag();
        u32 size = bytesToFlits(instruction->GetBytesIn());
        dbgprintf("Send to dst=%u seq=%lu size=%u inst=%li inst=%s",
                  destination, sequence_number, size, instruction->GetId(),
                  instruction->GetName().c_str());
        generateMessage(destination, sequence_number, size);
        finishExecution(instruction);
        loadReadyInstructions();
      } else {
        assert(false);  // programmer error!
      }
    }
  }

  // Determines if the simulation is over, if so, signals completion.
  if (readyInstructions_.empty() && executingInstructions_.empty()) {
    dbgprintf("done");
    Application* app = reinterpret_cast<Application*>(application());
    app->terminalComplete(id_);
  }
}

void GraphTerminal::finishExecution(paragraph::Instruction* _instruction) {
  // Removes the instruction from the executing set, frees up the core, and
  // informs the scheduler that the instruction was executed.
  assert(executingInstructions_.erase(_instruction) == 1);
  paragraph::Opcode opcode = _instruction->GetOpcode();
  if (opcode == paragraph::Opcode::kDelay ||
      opcode == paragraph::Opcode::kInfeed ||
      opcode == paragraph::Opcode::kOutfeed) {
    availableCores_++;
  }
  scheduler_->InstructionFinished(
      _instruction, gSim->time() / static_cast<f64>(unitsPerSecond_));
}

u32 GraphTerminal::bytesToFlits(u32 _bytes) const {
  // Rounds the size up to the closest number of flits. Returned value is > 0.
  f64 flitsFp = _bytes / static_cast<f64>(bytesPerFlit_);
  u32 flits = flitsFp == 0.0 ? 1 : static_cast<u32>(std::ceil(flitsFp));
  return flits;
}

void GraphTerminal::generateMessage(u32 _destination, u64 _sequence_number,
                                    u32 _size) {
  assert(gSim->epsilon() == 0);

  // Starts the transaction in the application
  u64 transaction = createTransaction();
  application()->workload()->messageLog()->startTransaction(transaction);

  // Determine the number of packets.
  u32 numPackets = _size / maxPacketSize_;
  if ((_size % maxPacketSize_) > 0) {
    numPackets++;
  }

  // Creates the message object
  u64* sequence_number = new u64;
  *sequence_number = _sequence_number;
  Message* message = new Message(numPackets, sequence_number);
  message->setProtocolClass(protocolClass_);
  message->setTransaction(transaction);

  // Creates the packets
  u32 flitsLeft = _size;
  for (u32 p = 0; p < numPackets; p++) {
    u32 packetLength = flitsLeft > maxPacketSize_ ? maxPacketSize_ : flitsLeft;

    Packet* packet = new Packet(p, packetLength, message);
    message->setPacket(p, packet);

    // create flits
    for (u32 f = 0; f < packetLength; f++) {
      bool headFlit = f == 0;
      bool tailFlit = f == (packetLength - 1);
      Flit* flit = new Flit(f, headFlit, tailFlit, packet);
      packet->setFlit(f, flit);
    }
    flitsLeft -= packetLength;
  }

  // Sends the message
  u32 msgId = sendMessage(message, _destination);
  (void)msgId;  // unused
}

void GraphTerminal::processReceivedMessage(Message* _message) {
  // If the receive operation has been registered, this will finish the
  // execution. If not this will save the received message's information.
  u32 source = _message->getSourceId();
  u64* sequence_number = reinterpret_cast<u64*>(_message->getData());
  u32 size = _message->numFlits();
  auto iter = expecting_.find(std::make_tuple(source, *sequence_number));
  if (iter != expecting_.end()) {
    paragraph::Instruction* instruction = std::get<0>(iter->second);
    u32 exp_size = std::get<1>(iter->second);
    assert(exp_size == size);
    assert(*sequence_number == instruction->GetCommunicationTag());
    expecting_.erase(iter);
    finishExecution(instruction);
    schedule();
  } else {
    assert(received_.emplace(std::make_tuple(source, *sequence_number), size)
               .second);
  }
  delete sequence_number;
  delete _message;
}

}  // namespace ParaGraph

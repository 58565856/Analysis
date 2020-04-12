/* Copyright 2017 J.Rieck (based on R. Thomas's work)
 * Copyright 2017 Quarkslab
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <numeric>
#include <iomanip>

#include "LIEF/MachO/hash.hpp"

#include "LIEF/MachO/RPathCommand.hpp"

namespace LIEF {
namespace MachO {

RPathCommand::RPathCommand(void) = default;
RPathCommand& RPathCommand::operator=(const RPathCommand&) = default;
RPathCommand::RPathCommand(const RPathCommand&) = default;
RPathCommand::~RPathCommand(void) = default;

RPathCommand::RPathCommand(const rpath_command *rpathCmd) :
  LoadCommand::LoadCommand{static_cast<LOAD_COMMAND_TYPES>(rpathCmd->cmd), rpathCmd->cmdsize}
{}

RPathCommand* RPathCommand::clone(void) const {
  return new RPathCommand(*this);
}

const std::string& RPathCommand::path(void) const {
  return this->path_;
}

void RPathCommand::path(const std::string& path) {
  this->path_ = path;
}


void RPathCommand::accept(Visitor& visitor) const {
  visitor.visit(*this);
}


bool RPathCommand::operator==(const RPathCommand& rhs) const {
  size_t hash_lhs = Hash::hash(*this);
  size_t hash_rhs = Hash::hash(rhs);
  return hash_lhs == hash_rhs;
}

bool RPathCommand::operator!=(const RPathCommand& rhs) const {
  return not (*this == rhs);
}


std::ostream& RPathCommand::print(std::ostream& os) const {
  LoadCommand::print(os);
  os << std::left
     << std::setw(10) << "Path: " << this->path();
  return os;
}


}
}

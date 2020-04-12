/* Copyright 2017 R. Thomas
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
#include <sstream>

#include "LIEF/PE/hash.hpp"

#include "LIEF/PE/EnumToString.hpp"
#include "LIEF/PE/CodeIntegrity.hpp"

namespace LIEF {
namespace PE {

CodeIntegrity::~CodeIntegrity(void) = default;
CodeIntegrity& CodeIntegrity::operator=(const CodeIntegrity&) = default;
CodeIntegrity::CodeIntegrity(const CodeIntegrity&) = default;

CodeIntegrity::CodeIntegrity(void) :
  flags_{0},
  catalog_{0},
  catalog_offset_{0},
  reserved_{0}
{}


CodeIntegrity::CodeIntegrity(const pe_code_integrity *header) :
  flags_{header->Flags},
  catalog_{header->Catalog},
  catalog_offset_{header->CatalogOffset},
  reserved_{header->Reserved}
{}


uint16_t CodeIntegrity::flags(void) const {
  return this->flags_;
}
uint16_t CodeIntegrity::catalog(void) const {
  return this->catalog_;
}

uint32_t CodeIntegrity::catalog_offset(void) const {
  return this->catalog_offset_;
}

uint32_t CodeIntegrity::reserved(void) const {
  return this->reserved_;
}


void CodeIntegrity::flags(uint16_t flags) {
  this->flags_ = flags;
}

void CodeIntegrity::catalog(uint16_t catalog) {
  this->catalog_ = catalog;
}

void CodeIntegrity::catalog_offset(uint32_t catalog_offset) {
  this->catalog_offset_ = catalog_offset;
}

void CodeIntegrity::reserved(uint32_t reserved) {
  this->reserved_ = reserved;
}

void CodeIntegrity::accept(LIEF::Visitor& visitor) const {
  visitor.visit(*this);
}

bool CodeIntegrity::operator==(const CodeIntegrity& rhs) const {
  size_t hash_lhs = Hash::hash(*this);
  size_t hash_rhs = Hash::hash(rhs);
  return hash_lhs == hash_rhs;
}

bool CodeIntegrity::operator!=(const CodeIntegrity& rhs) const {
  return not (*this == rhs);
}

std::ostream& operator<<(std::ostream& os, const CodeIntegrity& entry) {
  os << std::hex << std::left << std::showbase;
  os << std::setw(CodeIntegrity::PRINT_WIDTH) << std::setfill(' ') << "Flags:"          << std::hex << entry.flags()          << std::endl;
  os << std::setw(CodeIntegrity::PRINT_WIDTH) << std::setfill(' ') << "Catalog:"        << std::hex << entry.catalog()        << std::endl;
  os << std::setw(CodeIntegrity::PRINT_WIDTH) << std::setfill(' ') << "Catalog offset:" << std::hex << entry.catalog_offset() << std::endl;
  os << std::setw(CodeIntegrity::PRINT_WIDTH) << std::setfill(' ') << "Reserved:"       << std::hex << entry.reserved()       << std::endl;
  return os;

}

}
}

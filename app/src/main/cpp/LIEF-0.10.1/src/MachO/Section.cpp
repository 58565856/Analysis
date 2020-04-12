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
#include <algorithm>
#include <numeric>
#include <iomanip>
#include <iterator>

#include "LIEF/logging++.hpp"
#include "LIEF/exception.hpp"
#include "LIEF/MachO/hash.hpp"

#include "LIEF/MachO/Section.hpp"
#include "LIEF/MachO/SegmentCommand.hpp"
#include "LIEF/MachO/EnumToString.hpp"

namespace LIEF {
namespace MachO {

Section& Section::operator=(Section other) {
  this->swap(other);
  return *this;
}

Section::Section(const Section& other) :
  LIEF::Section{other},
  segment_name_{other.segment_name_},
  original_size_{other.original_size_},
  align_{other.align_},
  relocations_offset_{other.relocations_offset_},
  nbof_relocations_{other.nbof_relocations_},
  flags_{other.flags_},
  reserved1_{other.reserved1_},
  reserved2_{other.reserved2_},
  reserved3_{other.reserved3_},
  content_{other.content_},
  segment_{nullptr},
  relocations_{}
{}


Section::~Section(void) {
  for (Relocation* reloc : this->relocations_) {
    delete reloc;
  }
}

Section::Section(void) :
  LIEF::Section{},
  segment_name_{""},
  original_size_{0},
  align_{0},
  relocations_offset_{0},
  nbof_relocations_{0},
  flags_{0},
  reserved1_{0},
  reserved2_{0},
  reserved3_{0},
  content_{},
  segment_{nullptr},
  relocations_{}
{
  this->size_   = 0;
  this->offset_ = 0;
}

Section::Section(const section_32 *sectionCmd) :
  segment_name_{sectionCmd->segname, sizeof(sectionCmd->sectname)},
  original_size_{sectionCmd->size},
  align_{sectionCmd->align},
  relocations_offset_{sectionCmd->reloff},
  nbof_relocations_{sectionCmd->nreloc},
  flags_{sectionCmd->flags},
  reserved1_{sectionCmd->reserved1},
  reserved2_{sectionCmd->reserved2},
  reserved3_{0},
  segment_{nullptr},
  relocations_{}
{
  this->name_            = {sectionCmd->sectname, sizeof(sectionCmd->sectname)};
  this->size_            = sectionCmd->size;
  this->offset_          = sectionCmd->offset;
  this->virtual_address_ = sectionCmd->addr;

  this->name_         = std::string{this->name_.c_str()};
  this->segment_name_ = std::string{this->segment_name_.c_str()};
}

Section::Section(const section_64 *sectionCmd) :
  segment_name_{sectionCmd->segname, sizeof(sectionCmd->segname)},
  original_size_{sectionCmd->size},
  align_{sectionCmd->align},
  relocations_offset_{sectionCmd->reloff},
  nbof_relocations_{sectionCmd->nreloc},
  flags_{sectionCmd->flags},
  reserved1_{sectionCmd->reserved1},
  reserved2_{sectionCmd->reserved2},
  reserved3_{sectionCmd->reserved3},
  segment_{nullptr},
  relocations_{}
{
  this->name_            = {sectionCmd->sectname, sizeof(sectionCmd->sectname)};
  this->size_            = sectionCmd->size;
  this->offset_          = sectionCmd->offset;
  this->virtual_address_ = sectionCmd->addr;

  this->name_         = std::string{this->name_.c_str()};
  this->segment_name_ = std::string{this->segment_name_.c_str()};
}


void Section::swap(Section& other) {
  std::swap(this->name_,            other.name_);
  std::swap(this->virtual_address_, other.virtual_address_);
  std::swap(this->size_,            other.size_);
  std::swap(this->offset_,          other.offset_);

  std::swap(this->segment_name_,        other.segment_name_);
  std::swap(this->original_size_,       other.original_size_);
  std::swap(this->align_,               other.align_);
  std::swap(this->relocations_offset_,  other.relocations_offset_);
  std::swap(this->nbof_relocations_,    other.nbof_relocations_);
  std::swap(this->flags_,               other.flags_);
  std::swap(this->reserved1_,           other.reserved1_);
  std::swap(this->reserved2_,           other.reserved2_);
  std::swap(this->reserved3_,           other.reserved3_);
  std::swap(this->content_,             other.content_);
  std::swap(this->segment_,             other.segment_);
  std::swap(this->relocations_,         other.relocations_);

}


Section::Section(const std::string& name) :
  Section{}
{
  this->name(name);
}

Section::Section(const std::string& name, const Section::content_t& content) :
  Section{}
{
  this->name(name);
  this->content(content);
}

Section::content_t Section::content(void) const {
  if (this->segment_ == nullptr) {
    return this->content_;
  }

  if (this->size_ == 0 or this->offset_ == 0) { // bss section for instance
    return {};
  }

  uint64_t relative_offset = this->offset_ - this->segment_->file_offset();
  const std::vector<uint8_t>& content = this->segment_->content();
  if ((relative_offset + this->size_) > content.size()) {
    throw LIEF::corrupted("Section's size is bigger than segment's size");
  }
  std::vector<uint8_t> section_content = {
    content.data() + relative_offset,
    content.data() + relative_offset + this->size_};
  return section_content;
}

void Section::content(const Section::content_t& data) {
  if (this->segment_ == nullptr) {
    this->content_ = data;
    return;
  }

  if (this->size_ == 0 or this->offset_ == 0) { // bss section for instance
    LOG(ERROR) << "Offset or size is null";
    return;
  }


  uint64_t relative_offset = this->offset_ - this->segment_->file_offset();

  std::vector<uint8_t> content = this->segment_->content();

  if (data.size() > content.size()) {
    LOG(ERROR) << "New data are bigger than the original one";
    return;
  }

  std::move(std::begin(data), std::end(data), std::begin(content) + relative_offset);
  this->segment_->content(content);
}

const std::string& Section::segment_name(void) const {
  if (this->segment_ != nullptr) {
    return this->segment_->name();
  } else {
    return this->segment_name_;
  }
}

uint64_t Section::address(void) const {
  return this->virtual_address();
}

uint32_t Section::alignment(void) const {
  return this->align_;
}

uint32_t Section::relocation_offset(void) const {
  return relocations_offset_;
}

uint32_t Section::numberof_relocations(void) const {
  return nbof_relocations_;
}

uint32_t Section::flags(void) const {
  static constexpr size_t SECTION_FLAGS_MASK = 0xffffff00u;
  return (this->flags_ & SECTION_FLAGS_MASK);
}

uint32_t Section::reserved1(void) const {
  return this->reserved1_;
}

uint32_t Section::reserved2(void) const {
  return this->reserved2_;
}

uint32_t Section::reserved3(void) const {
  return this->reserved3_;
}


uint32_t Section::raw_flags(void) const {
  return this->flags_;
}

it_relocations Section::relocations(void) {
  return this->relocations_;
}

it_const_relocations Section::relocations(void) const {
  return this->relocations_;
}

MACHO_SECTION_TYPES Section::type(void) const {
  static constexpr size_t SECTION_TYPE_MASK = 0xFF;
  return static_cast<MACHO_SECTION_TYPES>(this->flags_ & SECTION_TYPE_MASK);
}

Section::flag_list_t Section::flags_list(void) const {

  Section::flag_list_t flags;

  std::copy_if(
      std::begin(section_flags_array),
      std::end(section_flags_array),
      std::inserter(flags, std::begin(flags)),
      std::bind(static_cast<bool (Section::*)(MACHO_SECTION_FLAGS) const>(&Section::has), this, std::placeholders::_1));

  return flags;
}

void Section::segment_name(const std::string& name) {
  this->segment_name_ = name;
  if (this->segment_ != nullptr) {
    return this->segment_->name(name);
  }
}

void Section::address(uint64_t address) {
  this->virtual_address(address);
}

void Section::alignment(uint32_t align) {
  this->align_ = align;
}

void Section::relocation_offset(uint32_t relocOffset) {
  this->relocations_offset_ = relocOffset;
}

void Section::numberof_relocations(uint32_t nbReloc) {
  this->nbof_relocations_ = nbReloc;
}

void Section::flags(uint32_t flags) {
  this->flags_ = this->flags_ | flags;
}

void Section::reserved1(uint32_t reserved1) {
  this->reserved1_ = reserved1;
}

void Section::reserved2(uint32_t reserved2) {
  this->reserved2_ = reserved2;
}

void Section::reserved3(uint32_t reserved3) {
  this->reserved3_ = reserved3;
}

void Section::type(MACHO_SECTION_TYPES type) {
  static constexpr size_t SECTION_FLAGS_MASK = 0xffffff00u;
  this->flags_ = (this->flags_ & SECTION_FLAGS_MASK) | static_cast<uint8_t>(type);
}


bool Section::has(MACHO_SECTION_FLAGS flag) const {
  return (static_cast<uint32_t>(flag) & this->flags()) > 0;
}

void Section::add(MACHO_SECTION_FLAGS flag) {
  this->flags(this->raw_flags() | static_cast<uint32_t>(flag));
}

void Section::remove(MACHO_SECTION_FLAGS flag) {
  this->flags_= this->raw_flags() & (~ static_cast<uint32_t>(flag));
}

Section& Section::operator+=(MACHO_SECTION_FLAGS flag) {
  this->add(flag);
  return *this;
}

Section& Section::operator-=(MACHO_SECTION_FLAGS flag) {
  this->remove(flag);
  return *this;
}


void Section::clear(uint8_t v) {
  Section::content_t clear(this->size(), v);
  this->content(std::move(clear));
}


bool Section::has_segment(void) const {
  return this->segment_ != nullptr;
}

SegmentCommand& Section::segment(void) {
  return const_cast<SegmentCommand&>(static_cast<const Section*>(this)->segment());
}

const SegmentCommand& Section::segment(void) const {
  if (not this->has_segment()) {
    throw not_found("No segment associated with this section");
  }
  return *this->segment_;
}


void Section::accept(Visitor& visitor) const {
  visitor.visit(*this);
}

bool Section::operator==(const Section& rhs) const {
  size_t hash_lhs = Hash::hash(*this);
  size_t hash_rhs = Hash::hash(rhs);
  return hash_lhs == hash_rhs;
}

bool Section::operator!=(const Section& rhs) const {
  return not (*this == rhs);
}


std::ostream& operator<<(std::ostream& os, const Section& section) {
  const auto& flags = section.flags_list();

   std::string flags_str = std::accumulate(
     std::begin(flags),
     std::end(flags), std::string{},
     [] (const std::string& a, MACHO_SECTION_FLAGS b) {
         return a.empty() ? to_string(b) : a + " " + to_string(b);
     });

  os << std::hex;
  os << std::left
     << std::setw(17) << section.name()
     << std::setw(17) << section.segment_name()
     << std::setw(10) << section.address()
     << std::setw(10) << section.size()
     << std::setw(10) << section.offset()
     << std::setw(10) << section.alignment()
     << std::setw(30) << to_string(section.type())
     << std::setw(20) << section.relocation_offset()
     << std::setw(20) << section.numberof_relocations()
     << std::setw(10) << section.reserved1()
     << std::setw(10) << section.reserved2()
     << std::setw(10) << section.reserved3()
     << std::setw(10) << flags_str;

  if (section.segment_ != nullptr) {
    //os << std::setw(10) << section.segment_->name();
  }

  if (section.relocations().size() > 0)  {
    os << std::endl;
    os << "Relocations associated with the section :" << std::endl;
    for (const Relocation& relocation : section.relocations()) {
      os << "    " << relocation << std::endl;
    }
  }


  return os;
}

} // namespace MachO
} // namespace LIEF

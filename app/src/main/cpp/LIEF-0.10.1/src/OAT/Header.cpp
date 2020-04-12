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

#include "LIEF/OAT/Header.hpp"
#include "LIEF/OAT/EnumToString.hpp"
#include "LIEF/OAT/hash.hpp"

#include <map>
#include <iomanip>
#include <sstream>

namespace LIEF {
namespace OAT {

Header::Header(const Header&) = default;
Header& Header::operator=(const Header&) = default;

Header::Header(void) :
  magic_{{'o', 'a', 't', '\n'}},
  version_{0}
{}


std::string Header::key_to_string(HEADER_KEYS key) {
  const static std::map<HEADER_KEYS, const char*> keys2str = {
    { HEADER_KEYS::KEY_IMAGE_LOCATION,     "image-location"      },
    { HEADER_KEYS::KEY_DEX2OAT_CMD_LINE,   "dex2oat-cmdline"     },
    { HEADER_KEYS::KEY_DEX2OAT_HOST,       "dex2oat-host"        },
    { HEADER_KEYS::KEY_PIC,                "pic"                 },
    { HEADER_KEYS::KEY_HAS_PATCH_INFO,     "has-patch-info"      },
    { HEADER_KEYS::KEY_DEBUGGABLE,         "debuggable"          },
    { HEADER_KEYS::KEY_NATIVE_DEBUGGABLE,  "native-debuggable"   },
    { HEADER_KEYS::KEY_COMPILER_FILTER,    "compiler-filter"     },
    { HEADER_KEYS::KEY_CLASS_PATH,         "classpath"           },
    { HEADER_KEYS::KEY_BOOT_CLASS_PATH,    "bootclasspath"       },
    { HEADER_KEYS::KEY_CONCURRENT_COPYING, "concurrent-copying"  },
    { HEADER_KEYS::KE_COMPILATION_REASON,   "compilation-reason" },
  };

  auto   it  = keys2str.find(key);
  return it == keys2str.end() ? "UNKNOWN" : it->second;
}


Header::magic_t Header::magic(void) const {
  return this->magic_;
}

oat_version_t Header::version(void) const {
  return this->version_;
}


INSTRUCTION_SETS Header::instruction_set(void) const {
  return this->instruction_set_;
}


uint32_t Header::checksum(void) const {
  return this->checksum_;
}

uint32_t Header::nb_dex_files(void) const {
  return this->dex_file_count_;
}


uint32_t Header::executable_offset(void) const {
  return this->executable_offset_;
}

uint32_t Header::i2i_bridge_offset(void) const {
  return this->i2i_bridge_offset_;
}

uint32_t Header::i2c_code_bridge_offset(void) const {
  return this->i2c_code_bridge_offset_;
}

uint32_t Header::jni_dlsym_lookup_offset(void) const {
  return this->jni_dlsym_lookup_offset_;
}

uint32_t Header::quick_generic_jni_trampoline_offset(void) const {
  return this->quick_generic_jni_trampoline_offset_;
}

uint32_t Header::quick_imt_conflict_trampoline_offset(void) const {
  return this->quick_generic_jni_trampoline_offset_;
}

uint32_t Header::quick_resolution_trampoline_offset(void) const {
  return this->quick_imt_conflict_trampoline_offset_;
}

uint32_t Header::quick_to_interpreter_bridge_offset(void) const {
  return this->quick_to_interpreter_bridge_offset_;
}

int32_t Header::image_patch_delta(void) const {
  return this->image_patch_delta_;
}

uint32_t Header::image_file_location_oat_checksum(void) const {
  return this->image_file_location_oat_checksum_;
}
uint32_t Header::image_file_location_oat_data_begin(void) const {
  return this->image_file_location_oat_data_begin_;
}

uint32_t Header::key_value_size(void) const {
  return this->key_value_store_size_;
}

uint32_t Header::oat_dex_files_offset(void) const {
  return this->oat_dex_files_offset_;
}

Header::it_key_values_t Header::key_values(void) {
  it_key_values_t::container_type key_values_list;
  key_values_list.reserve(this->dex2oat_context_.size());

  for (auto&& p : this->dex2oat_context_) {
    HEADER_KEYS key = p.first;
    std::string& value = this->dex2oat_context_.at(key);
    key_values_list.emplace_back(key, std::ref(value));
  }
  return key_values_list;
}

Header::it_const_key_values_t Header::key_values(void) const {
  std::remove_const<it_const_key_values_t::container_type>::type key_values_list;
  for (auto&& p : this->dex2oat_context_) {
    HEADER_KEYS key = p.first;
    std::string value = this->dex2oat_context_.at(key);
    key_values_list.emplace_back(key, value);
  }
  return key_values_list;
}


Header::keys_t Header::keys(void) const {
  Header::keys_t keys_list;
  keys_list.reserve(this->dex2oat_context_.size());
  for (auto p : this->dex2oat_context_) {
    keys_list.push_back(p.first);
  }
  return keys_list;
}

Header::values_t Header::values(void) const {
  Header::values_t values_list;
  values_list.reserve(this->dex2oat_context_.size());
  for (auto p : this->dex2oat_context_) {
    values_list.push_back(p.second);
  }
  return values_list;
}

const std::string& Header::get(HEADER_KEYS key) const {
  auto&& it = this->dex2oat_context_.find(key);
  if (it == std::end(this->dex2oat_context_)) {
    throw not_found("Unable to find the key " + Header::key_to_string(key));
  }
  return it->second;
}

std::string& Header::get(HEADER_KEYS key) {
  return const_cast<std::string&>(static_cast<const Header*>(this)->get(key));
}


Header& Header::set(HEADER_KEYS key, const std::string& value) {
  auto&& it = this->dex2oat_context_.find(key);
  if (it == std::end(this->dex2oat_context_)) {
    throw not_found(std::string{"Can't find key: '"} + to_string(key) + "'");
  }

  it->second = value;
  return *this;
}

const std::string& Header::operator[](HEADER_KEYS key) const {
  return this->get(key);
}

std::string& Header::operator[](HEADER_KEYS key) {
  return this->get(key);
}

void Header::accept(Visitor& visitor) const {
  visitor.visit(*this);
}

bool Header::operator==(const Header& rhs) const {
  size_t hash_lhs = Hash::hash(*this);
  size_t hash_rhs = Hash::hash(rhs);
  return hash_lhs == hash_rhs;
}

bool Header::operator!=(const Header& rhs) const {
  return not (*this == rhs);
}



std::ostream& operator<<(std::ostream& os, const Header& hdr) {
  static constexpr size_t WIDTH = 45;
  os << std::hex << std::left << std::showbase;
  //os << std::setw(33) << std::setfill(' ') << "Version:"   << ident_magic << std::endl;
  //os << std::setw(33) << std::setfill(' ') << "Location:"  << ident_magic << std::endl;
  os << std::setw(WIDTH) << std::setfill(' ') << "Checksum:"  << std::hex << hdr.checksum() << std::endl;
  os << std::setw(WIDTH) << std::setfill(' ') << "Instruction set:"  << to_string(hdr.instruction_set()) << std::endl;
  //os << std::setw(33) << std::setfill(' ') << "Instruction set features:"  << ident_magic << std::endl;
  os << std::setw(WIDTH) << std::setfill(' ') << "Dex file count:"                              << std::dec << hdr.nb_dex_files()      << std::endl;
  os << std::setw(WIDTH) << std::setfill(' ') << "Executable offset:"                           << std::hex << hdr.executable_offset() << std::endl;

  os << std::endl;

  os << std::setw(WIDTH) << std::setfill(' ') << "Interpreter to Interpreter Bridge Offset:"   << std::hex << hdr.i2i_bridge_offset()      << std::endl;
  os << std::setw(WIDTH) << std::setfill(' ') << "Interpreter to Compiled Code Bridge Offset:" << std::hex << hdr.i2c_code_bridge_offset() << std::endl;

  os << std::endl;

  os << std::setw(WIDTH) << std::setfill(' ') << "JNI dlsym lookup offset:" << std::hex << hdr.jni_dlsym_lookup_offset() << std::endl;

  os << std::endl;

  os << std::setw(WIDTH) << std::setfill(' ') << "Quick Generic JNI Trampoline Offset:"  << std::hex << hdr.quick_generic_jni_trampoline_offset()  << std::endl;
  os << std::setw(WIDTH) << std::setfill(' ') << "Quick IMT Conflict Trampoline Offset:" << std::hex << hdr.quick_imt_conflict_trampoline_offset() << std::endl;
  os << std::setw(WIDTH) << std::setfill(' ') << "Quick Resolution Trampoline Offset:"   << std::hex << hdr.quick_resolution_trampoline_offset()   << std::endl;
  os << std::setw(WIDTH) << std::setfill(' ') << "Quick to Interpreter Bridge Offset:"   << std::hex << hdr.quick_to_interpreter_bridge_offset()   << std::endl;

  os << std::endl;

  os << std::setw(WIDTH) << std::setfill(' ') << "Image Patch Delta:" << std::dec << hdr.image_patch_delta() << std::endl;

  os << std::endl;

  os << std::setw(WIDTH) << std::setfill(' ') << "Image File Location OAT Checksum:" << std::hex << hdr.image_file_location_oat_checksum()   << std::endl;
  os << std::setw(WIDTH) << std::setfill(' ') << "Image File Location OAT Begin:"    << std::hex << hdr.image_file_location_oat_data_begin() << std::endl;

  os << std::endl;

  for (auto&& p : hdr.key_values()) {
    os << std::setw(WIDTH) << std::setfill(' ') << Header::key_to_string(p.first) + ":" << p.second << std::endl;
  }

  return os;
}




}
}

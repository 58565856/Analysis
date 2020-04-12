#include "TrieNode.hpp"
#include "LIEF/MachO/Symbol.hpp"
#include "LIEF/iostream.hpp"
#include "LIEF/logging++.hpp"
#include "LIEF/MachO/enums.hpp"

namespace LIEF {
namespace MachO {

TrieEdge::TrieEdge(const std::string& str, TrieNode* node) :
  substr{str},
  child{node}
{}




TrieEdge* TrieEdge::create(const std::string& str, TrieNode* node) {
  return new TrieEdge{str, node};
}

TrieEdge::~TrieEdge(void) = default;

TrieNode::TrieNode(const std::string& str) :
  cummulative_string_{str},
  address_{0},
  flags_{0},
  other_{0},
  trie_offset_{0},
  has_export_info_{false},
  ordered_{false}
{}


TrieNode* TrieNode::create(const std::string& str) {
  return new TrieNode{str};
}

TrieNode::~TrieNode(void) {
  for (TrieEdge* edge : this->children_) {
    delete edge;
  }
}


// Mainly inspired from LLVM: lld/lib/ReaderWriter/MachO/MachONormalizedFileBinaryWriter.cpp
TrieNode& TrieNode::add_symbol(const ExportInfo& info, std::vector<TrieNode*>& nodes) {
  std::string partial_str = info.symbol().name().substr(this->cummulative_string_.size());

  for (TrieEdge* edge : this->children_) {

    std::string edge_string = edge->substr;

    if (partial_str.find(edge_string) == 0) {
      edge->child->add_symbol(info, nodes);
      return *this;
    }

    for (int n = edge_string.size() - 1; n > 0; --n) {
      if (partial_str.substr(0, n) == edge_string.substr(0, n)) {

        std::string b_node_str = edge->child->cummulative_string_;
        b_node_str = b_node_str.substr(0, b_node_str.size() + n - edge_string.size()); // drop front

        TrieNode* b_new_node = TrieNode::create(b_node_str);
        nodes.push_back(b_new_node);

        TrieNode* c_node = edge->child;

        std::string ab_edge_str = edge_string.substr(0, n);
        std::string bc_edge_str = edge_string.substr(n);

        TrieEdge& ab_edge = *edge;

        ab_edge.substr = ab_edge_str;
        ab_edge.child = b_new_node;

        TrieEdge* bc_edge = TrieEdge::create(bc_edge_str, c_node);
        b_new_node->children_.push_back(bc_edge);
        b_new_node->add_symbol(info, nodes);
        return *this;
      }
    }
  }

  if (info.has(EXPORT_SYMBOL_FLAGS::EXPORT_SYMBOL_FLAGS_REEXPORT)) {
    CHECK_EQ(info.other(), 0);
  }

  if (info.has(EXPORT_SYMBOL_FLAGS::EXPORT_SYMBOL_FLAGS_STUB_AND_RESOLVER)) {
    CHECK_NE(info.other(), 0);
  }

  TrieNode* new_node = TrieNode::create(info.symbol().name());
  TrieEdge* new_edge = TrieEdge::create(partial_str, new_node);

  this->children_.push_back(new_edge);

  new_node->address_ = info.address();
  new_node->flags_   = info.flags();
  new_node->other_   = info.other();

  if (info.has(EXPORT_SYMBOL_FLAGS::EXPORT_SYMBOL_FLAGS_REEXPORT)) {
    new_node->imported_name_ = "";
    if (info.alias() and info.alias()->name() != info.symbol().name())
    new_node->imported_name_ = info.alias()->name();
  }

  new_node->has_export_info_ = true;
  nodes.push_back(new_node);
  return *this;
}


// Mainly inspired from LLVM: lld/lib/ReaderWriter/MachO/MachONormalizedFileBinaryWriter.cpp - addOrderedNodes
// Add info in nodes making sure every parents node is inserted before
TrieNode& TrieNode::add_ordered_nodes(const ExportInfo& info, std::vector<TrieNode*>& nodes) {
  if (not this->ordered_) {
    nodes.push_back(this);
    this->ordered_ = true;
  }

  std::string partial_str = info.symbol().name().substr(this->cummulative_string_.size());
  for (TrieEdge* edge : this->children_) {
    std::string edge_string = edge->substr;

    if (partial_str.find(edge_string) == 0) {
      edge->child->add_ordered_nodes(info, nodes);
      return *this;
    }
  }
  return *this;
}


// Mainly inspired from LLVM: lld/lib/ReaderWriter/MachO/MachONormalizedFileBinaryWriter.cpp - updateOffset
bool TrieNode::update_offset(uint32_t& offset) {
  uint32_t node_size = 1;
  if (this->has_export_info_) {
   if (this->flags_ & static_cast<uint64_t>(EXPORT_SYMBOL_FLAGS::EXPORT_SYMBOL_FLAGS_REEXPORT)) {
      node_size = vector_iostream::uleb128_size(this->flags_);
      node_size += vector_iostream::uleb128_size(this->other_);
      node_size += this->imported_name_.size() + 1;
    } else {
      node_size = vector_iostream::uleb128_size(this->flags_);
      node_size += vector_iostream::uleb128_size(this->address_);
      if (this->flags_ & static_cast<uint64_t>(EXPORT_SYMBOL_FLAGS::EXPORT_SYMBOL_FLAGS_STUB_AND_RESOLVER)) {
        node_size += vector_iostream::uleb128_size(this->other_);
      }
    }
    node_size += vector_iostream::uleb128_size(node_size);
  }

  ++node_size;

  for (TrieEdge* edge : this->children_) {
    node_size += edge->substr.size() + 1;
    node_size += vector_iostream::uleb128_size(edge->child->trie_offset_);
  }

  bool result = (this->trie_offset_ != offset);
  this->trie_offset_ = offset;
  offset += node_size;

  return result;
}

TrieNode& TrieNode::write(vector_iostream& buffer) {
  if (this->has_export_info_) {
    if (this->flags_ & static_cast<uint64_t>(EXPORT_SYMBOL_FLAGS::EXPORT_SYMBOL_FLAGS_REEXPORT)) {
      if (not this->imported_name_.empty()) {
        uint32_t node_size = 0;
        node_size += vector_iostream::uleb128_size(this->flags_);
        node_size += vector_iostream::uleb128_size(this->other_);
        node_size += this->imported_name_.size() + 1;

        buffer
          .write<uint8_t>(node_size)
          .write_uleb128(this->flags_)
          .write_uleb128(this->other_)
          .write(this->imported_name_);

      } else {
        uint32_t node_size = 0;
        node_size += vector_iostream::uleb128_size(this->flags_);
        node_size += vector_iostream::uleb128_size(this->other_);
        node_size += 1;
        buffer
          .write<uint8_t>(node_size)
          .write_uleb128(this->flags_)
          .write_uleb128(this->other_)
          .write<uint8_t>('\0');
      }
    }
    else if (this->flags_ & static_cast<uint64_t>(EXPORT_SYMBOL_FLAGS::EXPORT_SYMBOL_FLAGS_STUB_AND_RESOLVER)) {
      uint32_t node_size = 0;
      node_size += vector_iostream::uleb128_size(this->flags_);
      node_size += vector_iostream::uleb128_size(this->address_);
      node_size += vector_iostream::uleb128_size(this->other_);

      buffer
        .write<uint8_t>(node_size)
        .write_uleb128(this->flags_)
        .write_uleb128(this->address_)
        .write_uleb128(this->other_);
    }
    else {
      uint32_t node_size = 0;
      node_size += vector_iostream::uleb128_size(this->flags_);
      node_size += vector_iostream::uleb128_size(this->address_);

      buffer
        .write<uint8_t>(node_size)
        .write_uleb128(this->flags_)
        .write_uleb128(this->address_);
    }

  } else { // not this->has_export_info_
    buffer.write<uint8_t>(0);
  }

  // Number of childs
  CHECK(this->children_.size() < 256);
  buffer.write<uint8_t>(this->children_.size());
  for (TrieEdge* edge : this->children_) {
    buffer.write(edge->substr)
          .write_uleb128(edge->child->trie_offset_);
  }
  return *this;
}



}
}

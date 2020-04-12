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
#include "pyPE.hpp"

#include "LIEF/PE/hash.hpp"
#include "LIEF/Abstract/Section.hpp"
#include "LIEF/PE/Section.hpp"

#include <string>
#include <sstream>

namespace LIEF {
namespace PE {

template<class T>
using getter_t = T (Section::*)(void) const;

template<class T>
using setter_t = void (Section::*)(T);


template<>
void create<Section>(py::module& m) {
  py::class_<Section, LIEF::Section>(m, "Section")
    .def(py::init<>())
    .def(py::init<const std::vector<uint8_t>&, const std::string&, uint32_t>(),
        "Constructor from "
        ":attr:`~lief.PE.Section.content`, "
        ":attr:`~lief.PE.Section.name` and "
        ":attr:`~lief.PE.Section.characteristics`",
        "content"_a, py::arg("name") = "", py::arg("characteristics") = 0)

    .def(py::init<const std::string&>(),
        "Constructor from "
        ":attr:`~lief.PE.Section.name`",
        "name"_a)

    .def_property("virtual_size",
        static_cast<getter_t<uint32_t>>(&Section::virtual_size),
        static_cast<setter_t<uint32_t>>(&Section::virtual_size),
        "The total size of the section when loaded into "
        "memory. If this value is greater than "
        ":attr:`~lief.PE.Section.sizeof_raw_data`, the section is zero-padded. ")

    .def_property("sizeof_raw_data",
        static_cast<getter_t<uint32_t>>(&Section::sizeof_raw_data),
        static_cast<setter_t<uint32_t>>(&Section::sizeof_raw_data),
        "Alias of :attr:`~lief.PE.Section.size`")

    .def_property("pointerto_raw_data",
        static_cast<getter_t<uint32_t>>(&Section::pointerto_raw_data),
        static_cast<setter_t<uint32_t>>(&Section::pointerto_raw_data),
        "Alias of :attr:`~lief.PE.Section.offset`")

    .def_property("pointerto_relocation",
        static_cast<getter_t<uint32_t>>(&Section::pointerto_relocation),
        static_cast<setter_t<uint32_t>>(&Section::pointerto_relocation),
        "The file pointer to the beginning of relocation "
        "entries for the section. This is set to zero for "
        "executable images or if there are no "
        "relocations.")

    .def_property("pointerto_line_numbers",
        static_cast<getter_t<uint32_t>>(&Section::pointerto_line_numbers),
        static_cast<setter_t<uint32_t>>(&Section::pointerto_line_numbers),
        "The file pointer to the beginning of line-number "
        "entries for the section. This is set to zero if "
        "there are no COFF line numbers. This value "
        "should be zero for an image because COFF "
        "debugging information is deprecated.")

    .def_property("numberof_relocations",
        static_cast<getter_t<uint16_t>>(&Section::numberof_relocations),
        static_cast<setter_t<uint16_t>>(&Section::numberof_relocations),
        "The number of relocation entries for the section. "
        "This is set to zero for executable images.")

    .def_property("numberof_line_numbers",
        static_cast<getter_t<uint16_t>>(&Section::numberof_line_numbers),
        static_cast<setter_t<uint16_t>>(&Section::numberof_line_numbers),
        "The number of line-number entries for the "
        "section. This value should be zero for an image "
        "because COFF debugging information is "
        "deprecated.")

    .def_property("characteristics",
        static_cast<getter_t<uint32_t>>(&Section::characteristics),
        static_cast<setter_t<uint32_t>>(&Section::characteristics),
        "The " RST_CLASS_REF(lief.PE.SECTION_CHARACTERISTICS) "  that describe the characteristics of the section")

    .def_property_readonly("characteristics_lists",
        &Section::characteristics_list,
        ":attr:`~lief.PE.Section.characteristics` as a ``list``")

    .def("has_characteristic",
        &Section::has_characteristic,
        "``True`` if the a section has the given " RST_CLASS_REF(lief.PE.SECTION_CHARACTERISTICS) "",
        "characteristic"_a)

    .def("__eq__", &Section::operator==)
    .def("__ne__", &Section::operator!=)
    .def("__hash__",
        [] (const Section& section) {
          return Hash::hash(section);
        })

    .def("__str__",
        [] (const Section& section) {
          std::ostringstream stream;
          stream << section;
          std::string str =  stream.str();
          return str;
        });
}

}
}

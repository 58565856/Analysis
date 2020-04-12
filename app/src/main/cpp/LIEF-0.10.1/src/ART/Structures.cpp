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
#include "LIEF/ART/Structures.hpp"

namespace LIEF {
namespace ART {

constexpr art_version_t ART17::art_version;
constexpr art_version_t ART29::art_version;
constexpr art_version_t ART30::art_version;
constexpr art_version_t ART44::art_version;
constexpr art_version_t ART46::art_version;
constexpr art_version_t ART56::art_version;

constexpr uint32_t ART17::nb_image_roots;
constexpr uint32_t ART29::nb_image_roots;
constexpr uint32_t ART30::nb_image_roots;
constexpr uint32_t ART44::nb_image_roots;
constexpr uint32_t ART46::nb_image_roots;
constexpr uint32_t ART56::nb_image_roots;

} // Namespace ART
} // Namespace LIEF

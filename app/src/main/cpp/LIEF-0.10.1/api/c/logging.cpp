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
#include "LIEF/logging.hpp"
#include "LIEF/logging.h"

using namespace LIEF;

void lief_logging_disable(void) {
  Logger::disable();
}

void lief_logging_enable(void) {
  Logger::enable();
}

void lief_logging_set_level(uint32_t level) {
  Logger::set_level(static_cast<LOGGING_LEVEL>(level));
}

void lief_logging_set_verbose_level(uint32_t level) {
  Logger::set_verbose_level(level);
}

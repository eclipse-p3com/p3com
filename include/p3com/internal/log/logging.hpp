// Copyright (c) 2020 by Robert Bosch GmbH. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

/* Modifications by NXP 2023 */

#ifndef P3COM_LOGGING_HPP
#define P3COM_LOGGING_HPP

#include "iceoryx_hoofs/log/logging_free_function_building_block.hpp"

namespace iox
{
namespace p3com
{
/**
 * @brief Log component
 * 
 */
struct P3comLoggingComponent
{
    // Ctx
    static constexpr char Ctx[]{"p3com"};
    // Description
    static constexpr char Description[]{"Log context of the p3com module."};
};

static constexpr auto LogFatal = iox::log::ffbb::LogFatal<P3comLoggingComponent>;
static constexpr auto LogError = iox::log::ffbb::LogError<P3comLoggingComponent>;
static constexpr auto LogWarn = iox::log::ffbb::LogWarn<P3comLoggingComponent>;
static constexpr auto LogInfo = iox::log::ffbb::LogInfo<P3comLoggingComponent>;
static constexpr auto LogDebug = iox::log::ffbb::LogDebug<P3comLoggingComponent>;
static constexpr auto LogVerbose = iox::log::ffbb::LogVerbose<P3comLoggingComponent>;

} // namespace p3com
} // namespace iox

#endif // P3COM_LOGGING_HPP

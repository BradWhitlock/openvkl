// ======================================================================== //
// Copyright 2018 Intel Corporation                                         //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#pragma once

#include "ospcommon/os/library.h"
#include <map>
#include "VKLCommon.h"
#include "logging.h"
#include "openvkl/VKLDataType.h"

namespace openvkl {

  template <typename T, VKLDataType VKL_TYPE = VKL_UNKNOWN>
  inline T *objectFactory(const std::string &type)
  {
    // Function pointer type for creating a concrete instance of a subtype of
    // this class.
    using creationFunctionPointer = T *(*)();

    // Function pointers corresponding to each subtype.
    static std::map<std::string, creationFunctionPointer> symbolRegistry;
    const auto type_string = stringForType(VKL_TYPE);

    // Find the creation function for the subtype if not already known.
    if (symbolRegistry.count(type) == 0) {
      postLogMessage(VKL_LOG_DEBUG)
          << "trying to look up " << type_string << " type '" << type
          << "' for the first time";

      // Construct the name of the creation function to look for.
      std::string creationFunctionName =
          "openvkl_create_" + type_string + "__" + type;

      // Look for the named function.
      symbolRegistry[type] =
          (creationFunctionPointer)ospcommon::getSymbol(creationFunctionName);

      // The named function may not be found if the requested subtype is not
      // known.
      if (!symbolRegistry[type]) {
        postLogMessage(VKL_LOG_WARNING)
            << "WARNING: unrecognized " << type_string << " type '" << type
            << "'.";
      }
    }

    // Create a concrete instance of the requested subtype.
    auto *object = symbolRegistry[type] ? (*symbolRegistry[type])() : nullptr;

    if (object == nullptr) {
      symbolRegistry.erase(type);
      throw std::runtime_error(
          "Could not find " + type_string + " of type: " + type +
          ".  Make sure you have the correct VKL libraries linked.");
    }

    return object;
  }

}  // namespace openvkl

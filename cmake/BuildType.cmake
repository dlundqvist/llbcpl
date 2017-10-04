# Copyright 2017 Daniel Lundqvist. All rights reserved.
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file.

# Adapted from various CMake files floating around.
set(__default_build_type "Debug")

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE "${__default_build_type}" CACHE
      STRING "Chose type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
               "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

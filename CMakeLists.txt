# Copyright 2017 Daniel Lundqvist. All rights reserved.
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file.
cmake_minimum_required(VERSION 3.9 FATAL_ERROR)
project(llbcpl LANGUAGES CXX)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(BuildType)
include(Warnings)

add_executable(ici ici.cc)

target_compile_features(ici PRIVATE cxx_std_14)
set_target_properties(ici PROPERTIES CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF)
ll_target_extra_warnings(ici PRIVATE)

install(TARGETS ici RUNTIME DESTINATION bin)

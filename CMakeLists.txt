# Copyright 2017 Daniel Lundqvist. All rights reserved.
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file.
cmake_minimum_required(VERSION 3.9 FATAL_ERROR)
project(llbcpl LANGUAGES CXX)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(LL)

ll_add_executable(concat concat.cc)

ll_add_executable(ici ici.cc)

install(TARGETS ici RUNTIME DESTINATION bin)

# Copyright 2017 Daniel Lundqvist. All rights reserved.
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file.
cmake_minimum_required(VERSION 3.9 FATAL_ERROR)
project(llbcpl LANGUAGES CXX)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(LL)

ll_add_executable(concat concat.cc)

set(C_IC bcplkit/iclib.ic bcplkit/blib.ic bcplkit/trn.ic bcplkit/syn.ic)
add_custom_command(OUTPUT c.ic
  COMMAND concat ${CMAKE_CURRENT_BINARY_DIR}/c.ic ${C_IC}
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS ${C_IC})
add_custom_target(c-ic ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/c.ic)

set(CG_IC bcplkit/iclib.ic bcplkit/blib.ic bcplkit/cg.ic)
add_custom_command(OUTPUT cg.ic
  COMMAND concat ${CMAKE_CURRENT_BINARY_DIR}/cg.ic ${CG_IC}
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS ${CG_IC})
add_custom_target(cg-ic ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/cg.ic)

ll_add_executable(ici ici.cc)

install(TARGETS ici RUNTIME DESTINATION bin)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/c.ic DESTINATION share/llbcpl)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/cg.ic DESTINATION share/llbcpl)

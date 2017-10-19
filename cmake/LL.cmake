# Copyright 2017 Daniel Lundqvist. All rights reserved.
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file.

include(CheckCXXCompilerFlag)

set(__ll_gnu_like_compilers "AppleClang;Clang;GNU"
    CACHE INTERNAL
    "List of compiler IDs that have GCC like command line options")

include(BuildType)
include(Warnings)
include(Sanitizer)

function(ll_add_cxx_compiler_flag target scope flag)
  set(var ${flag})
  string(REPLACE "-" "_" var ${var})
  string(REPLACE "/" "_" var ${var})
  string(REPLACE "=" "_" var ${var})
  string(TOUPPER ${var} var)
  string(TOUPPER ${PROJECT_NAME} pu)
  set(var "LL_CXX_FLAG${var}")
  check_cxx_compiler_flag(${flag} ${var})
  if(${var})
    if(NOT DEFINED ARGV3)
      target_compile_options(${target} ${scope} ${flag})
    else()
      target_compile_options(${target} ${scope} $<$<CONFIG:${ARGV3}>:${flag}>)
    endif()
  endif()
endfunction()

function(ll_add_executable target)
  add_executable(${target} ${ARGN})
  target_compile_features(${target} PRIVATE cxx_std_14)
  set_target_properties(${target} PROPERTIES
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF)
  ll_target_extra_warnings(${target} PRIVATE)
  ll_target_sanitizer(${target} PRIVATE)
endfunction()

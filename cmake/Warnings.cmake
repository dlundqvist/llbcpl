# Copyright 2017 Daniel Lundqvist. All rights reserved.
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file.

macro(ll_target_extra_warnings target scope)
  if(CMAKE_CXX_COMPILER_ID IN_LIST __ll_gnu_like_compilers)    
    ll_add_cxx_compiler_flag(${target} ${scope} "-pedantic")
    ll_add_cxx_compiler_flag(${target} ${scope} "-Wall")
    ll_add_cxx_compiler_flag(${target} ${scope} "-Wextra")
    ll_add_cxx_compiler_flag(${target} ${scope} "-Wunused")
    ll_add_cxx_compiler_flag(${target} ${scope} "-Wconversion")
    ll_add_cxx_compiler_flag(${target} ${scope} "-Wsign-conversion")
    ll_add_cxx_compiler_flag(${target} ${scope} "-Wuninitialized")
    ll_add_cxx_compiler_flag(${target} ${scope} "-Wshadow")
    ll_add_cxx_compiler_flag(${target} ${scope} "-Werror" "Debug")
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    ll_add_cxx_compiler_flag(${target} ${scope} "/W4")
    ll_add_cxx_compiler_flag(${target} ${scope} "/WX" "Debug")
  endif()
endmacro()

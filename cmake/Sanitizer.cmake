# Copyright 2017 Daniel Lundqvist. All rights reserved.
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file.

function(ll_target_sanitizer target scope)
  if(CMAKE_CXX_COMPILER_ID IN_LIST __ll_gnu_like_compilers)
    set(CMAKE_REQUIRED_FLAGS "-fsanitize=address")
    check_cxx_compiler_flag("-fsanitize=address" LL_SANITIZER_ADDRESS)
    set(CMAKE_REQUIRED_FLAGS "-fsanitize=undefined")
    check_cxx_compiler_flag("-fsanitize=undefined" LL_SANITIZER_UNDEFINED)
    if(LL_SANITIZER_ADDRESS)
      target_compile_options(${target} ${scope} $<$<CONFIG:Debug>:-fsanitize=address>)
      target_link_libraries(${target} ${scope} $<$<CONFIG:Debug>:-fsanitize=address>)
    endif()
    if(LL_SANITIZER_UNDEFINED)
      target_compile_options(${target} ${scope} $<$<CONFIG:Debug>:-fsanitize=undefined>)
      target_link_libraries(${target} ${scope} $<$<CONFIG:Debug>:-fsanitize=undefined>)
    endif()
  endif()
endfunction()

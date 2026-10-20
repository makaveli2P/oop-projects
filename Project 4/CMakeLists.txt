cmake_minimum_required(VERSION 3.25)
project(sal_ali VERSION 2.0.0 DESCRIPTION "SAL assembly interpreter and visualizer" LANGUAGES CXX)

option(SAL_BUILD_TUI "Build the terminal visualizer (FTXUI)" ON)
option(SAL_BUILD_TESTS "Build the Catch2 tests" ON)
option(SAL_SANITIZE "Build with AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
option(SAL_SANITIZE_THREAD "Build with ThreadSanitizer" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(SAL_SANITIZE)
  # Applies to every target, third-party code included, so that the
  # instrumentation is consistent.
  add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all)
  add_link_options(-fsanitize=address,undefined)
endif()
if(SAL_SANITIZE_THREAD)
  if(SAL_SANITIZE)
    message(FATAL_ERROR "SAL_SANITIZE and SAL_SANITIZE_THREAD cannot be ON together")
  endif()
  add_compile_options(-fsanitize=thread -fno-omit-frame-pointer)
  add_link_options(-fsanitize=thread)
endif()

# Strict warnings, as errors, for our own targets only. Third-party targets
# never get these flags.
function(sal_strict_warnings target)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4)
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow)
  endif()
  set_target_properties(${target} PROPERTIES COMPILE_WARNING_AS_ERROR ON)
endfunction()

include(FetchContent)

add_subdirectory(core)
if(SAL_BUILD_TUI)
  add_subdirectory(tui)
endif()
add_subdirectory(app)

if(SAL_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

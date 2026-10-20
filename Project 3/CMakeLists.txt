cmake_minimum_required(VERSION 3.25)
project(strset VERSION 2.0.0 DESCRIPTION "String set calculator and BST visualizer" LANGUAGES CXX)

option(STRSET_BUILD_TUI "Build the terminal visualizer (FTXUI)" ON)
option(STRSET_BUILD_TESTS "Build the Catch2 tests" ON)
option(STRSET_SANITIZE "Build with AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
option(STRSET_SANITIZE_THREAD "Build with ThreadSanitizer" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(STRSET_SANITIZE)
  # Applies to every target, third-party code included, so that the
  # instrumentation is consistent.
  add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all)
  add_link_options(-fsanitize=address,undefined)
endif()
if(STRSET_SANITIZE_THREAD)
  if(STRSET_SANITIZE)
    message(FATAL_ERROR "STRSET_SANITIZE and STRSET_SANITIZE_THREAD cannot be ON together")
  endif()
  add_compile_options(-fsanitize=thread -fno-omit-frame-pointer)
  add_link_options(-fsanitize=thread)
endif()

# Strict warnings, as errors, for our own targets only. Third-party targets
# never get these flags.
function(strset_strict_warnings target)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4)
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow
                                             -Wnon-virtual-dtor -Wold-style-cast -Woverloaded-virtual)
  endif()
  set_target_properties(${target} PROPERTIES COMPILE_WARNING_AS_ERROR ON)
endfunction()

include(FetchContent)

add_subdirectory(core)
if(STRSET_BUILD_TUI)
  add_subdirectory(tui)
endif()
add_subdirectory(app)

if(STRSET_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

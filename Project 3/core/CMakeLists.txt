add_library(strset_core STATIC
  src/aString.cpp
  src/BST.cpp
  src/calculator.cpp
  src/classic.cpp
  src/command.cpp
  src/Node.cpp
  src/trace.cpp
)
add_library(strset::core ALIAS strset_core)
target_include_directories(strset_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(strset_core PUBLIC cxx_std_17)
strset_strict_warnings(strset_core)

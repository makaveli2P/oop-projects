add_library(sal_core STATIC
  src/assembler.cpp
  src/datapath.cpp
  src/instructions.cpp
  src/machine.cpp
  src/micro_op.cpp
  src/program.cpp
)
add_library(sal::core ALIAS sal_core)
target_include_directories(sal_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(sal_core PUBLIC cxx_std_17)
sal_strict_warnings(sal_core)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.16.1
  GIT_SHALLOW TRUE
  SYSTEM
)
FetchContent_MakeAvailable(Catch2)

add_executable(sal_tests
  test_alu.cpp
  test_assembler.cpp
  test_instructions.cpp
  test_machine.cpp
  test_micro_ops.cpp
  test_programs.cpp
)
target_link_libraries(sal_tests PRIVATE Catch2::Catch2WithMain)
target_compile_definitions(sal_tests PRIVATE SAL_PROGRAMS_DIR="${PROJECT_SOURCE_DIR}/programs")
sal_strict_warnings(sal_tests)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)
catch_discover_tests(sal_tests)

# The visualizer tests need no terminal: they drive the controller, examine
# the routes and the ticker, and render frames to an in-memory screen.
if(SAL_BUILD_TUI)
  target_sources(sal_tests PRIVATE
    test_tui_controller.cpp
    test_tui_render.cpp
    test_tui_route.cpp
    test_tui_ticker.cpp
  )
  target_link_libraries(sal_tests PRIVATE sal::tui)  # sal::tui brings sal::core.
else()
  target_link_libraries(sal_tests PRIVATE sal::core)
endif()

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.16.1
  GIT_SHALLOW TRUE
  SYSTEM
)
FetchContent_MakeAvailable(Catch2)

add_executable(strset_tests
  test_bst.cpp
  test_calculator.cpp
  test_classic.cpp
  test_command.cpp
  test_set_ops.cpp
  test_string.cpp
  test_trace.cpp
)
target_link_libraries(strset_tests PRIVATE Catch2::Catch2WithMain)
strset_strict_warnings(strset_tests)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)
catch_discover_tests(strset_tests)

# The visualizer tests need no terminal: they drive the controller, examine
# the tree drawings and the ticker, and render frames to an in-memory screen.
if(STRSET_BUILD_TUI)
  target_sources(strset_tests PRIVATE
    test_tui_controller.cpp
    test_tui_layout.cpp
    test_tui_render.cpp
    test_tui_ticker.cpp
  )
  target_link_libraries(strset_tests PRIVATE strset::tui)  # strset::tui brings strset::core.
else()
  target_link_libraries(strset_tests PRIVATE strset::core)
endif()

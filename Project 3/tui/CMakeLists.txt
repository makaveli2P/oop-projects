# strset_tui: the terminal visualizer. The controller, the tree layout, the
# keys, the text format and the ticker do not use FTXUI. The views, the
# render mode and the app use FTXUI.

FetchContent_Declare(
  ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI.git
  GIT_TAG v7.0.3
  GIT_SHALLOW TRUE
  SYSTEM
)
set(FTXUI_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
set(FTXUI_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(FTXUI_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(FTXUI_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(FTXUI_QUIET ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(ftxui)

find_package(Threads REQUIRED)

add_library(strset_tui STATIC
  src/app.cpp
  src/controller.cpp
  src/format.cpp
  src/keys.cpp
  src/render.cpp
  src/theme.cpp
  src/ticker.cpp
  src/tree_layout.cpp
  src/views.cpp
)
add_library(strset::tui ALIAS strset_tui)
target_include_directories(strset_tui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
                                      PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(strset_tui
  PUBLIC strset::core Threads::Threads
  PRIVATE ftxui::screen ftxui::dom ftxui::component)
strset_strict_warnings(strset_tui)

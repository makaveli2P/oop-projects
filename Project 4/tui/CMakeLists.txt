# sal_tui: the terminal visualizer. The controller, the routes, the number
# format and the ticker do not use FTXUI. The views and the app use FTXUI.

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

add_library(sal_tui STATIC
  src/app.cpp
  src/controller.cpp
  src/diagram.cpp
  src/format.cpp
  src/keys.cpp
  src/load.cpp
  src/render.cpp
  src/route.cpp
  src/theme.cpp
  src/ticker.cpp
  src/views.cpp
)
add_library(sal::tui ALIAS sal_tui)
target_include_directories(sal_tui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
                                   PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(sal_tui
  PUBLIC sal::core Threads::Threads
  PRIVATE ftxui::screen ftxui::dom ftxui::component)
sal_strict_warnings(sal_tui)

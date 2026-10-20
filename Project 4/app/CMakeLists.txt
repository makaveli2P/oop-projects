add_executable(ali
  src/main.cpp
  src/plain_mode.cpp
)

# With SAL_BUILD_TUI, "ali" links the visualizer library sal_tui. Without it,
# a stub prints a message and only --plain works.
if(SAL_BUILD_TUI)
  target_sources(ali PRIVATE src/tui_mode.cpp)
  target_link_libraries(ali PRIVATE sal::tui)  # sal::tui brings sal::core.
else()
  target_sources(ali PRIVATE src/tui_mode_stub.cpp)
  target_link_libraries(ali PRIVATE sal::core)
endif()
sal_strict_warnings(ali)

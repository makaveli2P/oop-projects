add_executable(strset
  src/main.cpp
)

# With STRSET_BUILD_TUI, "strset" links the visualizer library strset_tui.
# Without it, a stub prints a message and only --plain works.
if(STRSET_BUILD_TUI)
  target_sources(strset PRIVATE src/tui_mode.cpp)
  target_link_libraries(strset PRIVATE strset::tui)  # strset::tui brings strset::core.
else()
  target_sources(strset PRIVATE src/tui_mode_stub.cpp)
  target_link_libraries(strset PRIVATE strset::core)
endif()
strset_strict_warnings(strset)

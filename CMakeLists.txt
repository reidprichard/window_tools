cmake_minimum_required(VERSION 3.25.0)
project(window_tools VERSION 0.1.3)
# CMAKE_EXPORT_COMPILE_COMMANDS doesn't work with MSVC; 
# to generate compile_commands.json run with a different
# generator. For example:
# cd project_root/build
# cmake .. -G="MinGW Makefiles"
# cmake --build .
# The build will fail, but compile_commands.json will be
# created in the build directory. Copy it to the root
# directory to make clangd happy.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) 
add_compile_definitions(CMAKE)
configure_file(src/window_tools.h.in window_tools.h)
add_executable(window_manager src/window_manager.c src/utils.c)
add_executable(kanata_helper_daemon src/kanata_helper_daemon.c src/utils.c)
target_include_directories(window_manager PUBLIC ${PROJECT_BINARY_DIR})
target_include_directories(kanata_helper_daemon PUBLIC ${PROJECT_BINARY_DIR})
target_sources(window_manager PRIVATE window_tools.rc)
target_sources(kanata_helper_daemon PRIVATE window_tools.rc)

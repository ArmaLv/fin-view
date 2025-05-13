cmake_minimum_required(VERSION 3.10)
project(TerminalCLIUtils VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

# Find required packages
find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIR})

# DirMon - Directory Monitor
add_executable(dirmon 
    src/dirmon/dirmon.cpp
)
target_link_libraries(dirmon ${CURSES_LIBRARIES})

# FileView - Directory Structure Viewer
add_executable(fileview
    src/fileview/fileview.cpp
)
target_link_libraries(fileview ${CURSES_LIBRARIES})

# FileSearch - Fuzzy File Search
add_executable(filesearch
    src/filesearch/filesearch.cpp
)
target_link_libraries(filesearch ${CURSES_LIBRARIES})

# Install targets
install(TARGETS dirmon fileview filesearch
    RUNTIME DESTINATION bin
)

# Create an alias for filesearch
install(CODE "execute_process(COMMAND ln -sf filesearch ${CMAKE_INSTALL_PREFIX}/bin/ff)")

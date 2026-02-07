cmake_minimum_required(VERSION 3.10)
project(EntityLib VERSION 1.0.0 LANGUAGES CXX)

# Specify C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Gather all source files
file(GLOB_RECURSE SOURCES "*.cpp")

# Define the library
add_library(EntityLib ${SOURCES})

# Include current directory for header resolution
target_include_directories(EntityLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Optional: Add installation rules
include(GNUInstallDirs)
install(TARGETS EntityLib
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/EntityLib
    FILES_MATCHING PATTERN "*.hpp"
)

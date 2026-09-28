cmake_minimum_required(VERSION 3.30)

set(PROJECT_NAME ordning)

project(${PROJECT_NAME} VERSION 0.0.1 LANGUAGES CXX)

if(NOT DEFINED BUILD_VERSION OR BUILD_VERSION STREQUAL "")
    message(FATAL_ERROR "BUILD_VERSION is not set. Pass -DBUILD_VERSION=<version> (handled by justfile).")
endif()
if(NOT DEFINED BUILD_DATE OR BUILD_DATE STREQUAL "")
    message(FATAL_ERROR "BUILD_DATE is not set. Pass -DBUILD_DATE=<date> (handled by justfile).")
endif()
if(NOT DEFINED BUILD_HASH OR BUILD_HASH STREQUAL "")
    message(FATAL_ERROR "BUILD_HASH is not set. Pass -DBUILD_HASH=<hash> (handled by justfile).")
endif()
if(NOT DEFINED REPOSITORY_URL OR REPOSITORY_URL STREQUAL "")
    message(FATAL_ERROR "REPOSITORY_URL is not set. Pass -DREPOSITORY_URL=<url> (handled by justfile).")
endif()
message(STATUS "BUILD_VERSION:  ${BUILD_VERSION}")
message(STATUS "BUILD_DATE:     ${BUILD_DATE}")
message(STATUS "BUILD_HASH:     ${BUILD_HASH}")
message(STATUS "REPOSITORY_URL: ${REPOSITORY_URL}")

# Generate version header from BUILD_VERSION, BUILD_DATE, and REPOSITORY_URL
configure_file(${CMAKE_SOURCE_DIR}/src/version.h.in ${CMAKE_BINARY_DIR}/version.h @ONLY)

# Windows console window configuration
option(SHOW_CONSOLE "Show console window on Windows (useful for debugging)" OFF)
if(WIN32 AND NOT SHOW_CONSOLE)
    message(STATUS "Windows GUI mode enabled - console window will be hidden")
else()
    message(STATUS "Console mode enabled")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Opt-in address/leak sanitizer build, used by `just debug-run` to get a
# symbolised stack trace out of a crash. The container has no gdb or valgrind,
# so this is the way to see where the application actually died.
option(ENABLE_ASAN "Build with the address and leak sanitizers" OFF)
if(ENABLE_ASAN)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer -g)
    add_link_options(-fsanitize=address)
endif()

# Qt Configuration
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
# set(CMAKE_AUTORCC ON)

# Find Qt6 packages
find_package(Qt6 6.8 QUIET REQUIRED COMPONENTS
    Core
    Gui
    PrintSupport
    Pdf
    PdfWidgets
    Sql
    Svg
    SvgWidgets
    Widgets
    Network
)

add_subdirectory(src)

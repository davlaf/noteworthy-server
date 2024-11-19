cmake_minimum_required(VERSION 3.8)

project(api-server)

# Set C++ standard and flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -pg -g3")
set(CXX_STANDARD "C++17")
set(CXX_STANDARD_REQUIRED "C++17")

# Find pkg-config (required for using pkg-config to find libraries)
find_package(PkgConfig REQUIRED)

# Use pkg-config to find Pistache, WebSockets, nlohmann_json, LCMS2, and MuPDF
pkg_check_modules(PISTACHE REQUIRED libpistache)
pkg_check_modules(LIBWEBSOCKETS REQUIRED libwebsockets)
pkg_check_modules(NLOHMANN_JSON REQUIRED nlohmann_json)
pkg_check_modules(LCMS2 REQUIRED lcms2)
pkg_check_modules(MUPDF REQUIRED mupdf)

# Include the header directories for each library found by pkg-config
include_directories(
    ${PISTACHE_INCLUDE_DIRS}
    ${LIBWEBSOCKETS_INCLUDE_DIRS}
    ${NLOHMANN_JSON_INCLUDE_DIRS}
    ${LCMS2_INCLUDE_DIRS}
    ${MUPDF_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/src  # Local source directory
)

# Locate source files
file(GLOB SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

# Add executable
add_executable(${PROJECT_NAME} ${SRCS})

# Link the libraries using the pkg-config found libraries
target_link_libraries(${PROJECT_NAME} 
    ${PISTACHE_LIBRARIES}
    ${LIBWEBSOCKETS_LIBRARIES}
    ${NLOHMANN_JSON_LIBRARIES}
    ${LCMS2_LIBRARIES}
    ${MUPDF_LIBRARIES}
    pthread
)

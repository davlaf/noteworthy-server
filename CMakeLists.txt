cmake_minimum_required(VERSION 3.8)

project(api-server)

# Set C++ standard and flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -pg -g3")
set(CXX_STANDARD "C++17")
set(CXX_STANDARD_REQUIRED "C++17")

# Include ExternalProject module
include(ExternalProject)

set(EXTERNAL_INSTALL_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/external)

# Define the ExternalProject for Pistache
ExternalProject_Add(PISTACHE
    GIT_REPOSITORY https://github.com/pistacheio/pistache.git
    BUILD_IN_SOURCE true
    CMAKE_ARGS -DPISTACHE_BUILD_TESTS=OFF
    INSTALL_COMMAND meson setup build --prefix=${EXTERNAL_INSTALL_LOCATION} --libdir=lib && meson install -C build
)

include_directories(${EXTERNAL_INSTALL_LOCATION}/include)
link_directories(${EXTERNAL_INSTALL_LOCATION}/lib)

include_directories(src)

# Locate source files
file(GLOB SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

# Add executable first (this is the target)
add_executable(${PROJECT_NAME} ${SRCS} )

# Add dependencies after target is created
add_dependencies(${PROJECT_NAME} PISTACHE )

# Link the libraries using the pkg-config found libraries
target_link_libraries(${PROJECT_NAME}
    pistache
    pthread
)

# Find pkg-config (required for using pkg-config to find libraries)
find_package(PkgConfig REQUIRED)

# Use pkg-config to find Pistache, WebSockets, nlohmann_json, LCMS2, and MuPDF
pkg_check_modules(LIBWEBSOCKETS REQUIRED libwebsockets)
pkg_check_modules(NLOHMANN_JSON REQUIRED nlohmann_json)


# Include the PDFium headers
include_directories(${CMAKE_SOURCE_DIR}/pdfium/include)

# Add the PDFium library
find_library(PDFIUM_LIB pdfium PATHS "${CMAKE_SOURCE_DIR}/pdfium/lib" NO_DEFAULT_PATH)

# Verify the library was found
if(NOT PDFIUM_LIB)
    message(FATAL_ERROR "PDFium library not found!")
else()
    message(STATUS "PDFium library found at: ${PDFIUM_LIB}")
endif()

# Include the header directories for each library found by pkg-config
include_directories(
    ${LIBWEBSOCKETS_INCLUDE_DIRS}
    ${NLOHMANN_JSON_INCLUDE_DIRS}
)

# Link libraries found by pkg-config
target_link_libraries(${PROJECT_NAME}
    ${LIBWEBSOCKETS_LIBRARIES}
    ${NLOHMANN_JSON_LIBRARIES}
    ${PDFIUM_LIB}
)

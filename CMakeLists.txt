cmake_minimum_required (VERSION 3.2)

project(api-server)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -pg -g3" )

include(ExternalProject)

set(EXTERNAL_INSTALL_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/external)

ExternalProject_Add(PISTACHE
    GIT_REPOSITORY https://github.com/pistacheio/pistache.git
    BUILD_IN_SOURCE true
    CMAKE_ARGS -DPISTACHE_BUILD_TESTS=OFF
    INSTALL_COMMAND meson setup build --prefix=${EXTERNAL_INSTALL_LOCATION} --libdir=lib && meson install -C build
)

ExternalProject_Add(NLOHMANN
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${EXTERNAL_INSTALL_LOCATION} -DJSON_BuildTests=OFF
)

ExternalProject_Add(LIBWEBSOCKETS
    GIT_REPOSITORY https://github.com/warmcat/libwebsockets.git
    GIT_TAG v4.3-stable

    CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX=${EXTERNAL_INSTALL_LOCATION}
        -DLWS_WITH_SHARED=OFF
        -DLWS_WTIH_STATIC=ON
        -DLWS_WITHOUT_TESTAPPS=ON
        -DLWS_WITH_SSL=OFF
)

include_directories(${EXTERNAL_INSTALL_LOCATION}/include)
link_directories(${EXTERNAL_INSTALL_LOCATION}/lib)

include_directories(openapi/model)
include_directories(openapi/api)
include_directories(openapi/impl)
include_directories(src)

file(GLOB SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/openapi/api/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/openapi/impl/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/openapi/model/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

add_executable(${PROJECT_NAME} ${SRCS} )
add_dependencies(${PROJECT_NAME} PISTACHE NLOHMANN LIBWEBSOCKETS)
target_link_libraries(${PROJECT_NAME} websockets pistache pthread)

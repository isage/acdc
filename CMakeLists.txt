cmake_minimum_required(VERSION 3.14)
include(CheckCXXCompilerFlag)

project(acdc)

set(acdc_VERSION_MAJOR 1)
set(acdc_VERSION_MINOR 2)
set(acdc_VERSION_PATCH 1)
set(acdc_VERSION_RELEASE 1)


include_directories(${CMAKE_CURRENT_BINARY_DIR})

configure_file(${CMAKE_SOURCE_DIR}/src/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)

file(GLOB_RECURSE APP_SOURCES "src/[a-zA-Z]*.cpp")
file(GLOB_RECURSE UTIL_SOURCES "src/utils/[a-zA-Z]*.c*")
file(GLOB_RECURSE CXML_SOURCES "src/cxml/[a-zA-Z]*.cpp")

set(SOURCES
    ${UTIL_SOURCES}
    ${CXML_SOURCES}
    ${APP_SOURCES}
)

if(MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")
else()
  add_definitions("-Wall")
endif()

if(MINGW)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static")
endif()

add_executable(acdc ${SOURCES})
set_target_properties(acdc PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
)

target_link_libraries(acdc z)


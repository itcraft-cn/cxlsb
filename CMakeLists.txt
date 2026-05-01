cmake_minimum_required(VERSION 3.10)
project(cxlsb VERSION 0.1.0 LANGUAGES C) # managed by updgitver

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-Wall -Wextra -g)
else()
    add_compile_options(-Wall -Wextra -O2)
endif()

find_package(ZLIB REQUIRED)

find_library(LIBZIP_LIBRARY NAMES zip libzip PATHS /usr/lib /usr/local/lib /lib/x86_64-linux-gnu)
find_path(LIBZIP_INCLUDE_DIR NAMES zip.h PATHS /usr/include /usr/local/include)

if(NOT LIBZIP_LIBRARY OR NOT LIBZIP_INCLUDE_DIR)
    message(FATAL_ERROR "libzip not found. Please install libzip-dev:\n"
        "  Ubuntu/Debian: sudo apt-get install libzip-dev\n"
        "  Fedora: sudo dnf install libzip-devel\n"
        "  macOS: brew install libzip")
endif()

set(ZLOG_ROOT "/home/helly/open_source/github/zlog")
set(ZLOG_INCLUDE_DIR "${ZLOG_ROOT}/src")
set(ZLOG_LIBRARY "${ZLOG_ROOT}/libzlog.so")

if(NOT EXISTS "${ZLOG_LIBRARY}")
    message(STATUS "zlog library not found at ${ZLOG_LIBRARY}, building without zlog")
    set(ZLOG_AVAILABLE OFF)
else()
    message(STATUS "Found zlog: ${ZLOG_LIBRARY}")
    set(ZLOG_AVAILABLE ON)
endif()

include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${LIBZIP_INCLUDE_DIR}
)

if(ZLOG_AVAILABLE)
    include_directories(${ZLOG_INCLUDE_DIR})
endif()

file(GLOB_RECURSE CXLSB_SOURCES "src/*.c")

add_library(cxlsb SHARED ${CXLSB_SOURCES})
target_link_libraries(cxlsb ${LIBZIP_LIBRARY} ${ZLIB_LIBRARIES})

if(ZLOG_AVAILABLE)
    target_link_libraries(cxlsb ${ZLOG_LIBRARY})
endif()

set_target_properties(cxlsb PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    OUTPUT_NAME "cxlsb"
)

install(TARGETS cxlsb
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY include/cxlsb
    DESTINATION include
)

enable_testing()

file(GLOB TEST_SOURCES "test/*.c")
foreach(TEST_FILE ${TEST_SOURCES})
    get_filename_component(TEST_NAME ${TEST_FILE} NAME_WE)
    add_executable(${TEST_NAME} ${TEST_FILE})
    target_link_libraries(${TEST_NAME} cxlsb)
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endforeach()

file(GLOB EXAMPLE_SOURCES "examples/*.c")
foreach(EXAMPLE_FILE ${EXAMPLE_SOURCES})
    get_filename_component(EXAMPLE_NAME ${EXAMPLE_FILE} NAME_WE)
    add_executable(${EXAMPLE_NAME} ${EXAMPLE_FILE})
    target_link_libraries(${EXAMPLE_NAME} cxlsb)
endforeach()

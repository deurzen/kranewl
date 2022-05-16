cmake_minimum_required(VERSION 3.10.0)

project(kranewl
    VERSION 0.0.1
    DESCRIPTION "A wlroots-based dynamic Wayland compositor, written in C++"
    HOMEPAGE_URL "%%https://github.com/deurzen/kranewl%%"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(
    -Wall -Wextra -Wshadow -Wnon-virtual-dtor -pedantic
    -Wnon-virtual-dtor
    -Wold-style-cast
    -Wcast-align
    -Wunused
    -Woverloaded-virtual
    -Wpedantic
    -Wconversion
    -Wsign-conversion
    -Wmisleading-indentation
    -Wduplicated-cond
    -Wduplicated-branches
    -Wlogical-op
    -Wnull-dereference
    -Wuseless-cast
    -Wdouble-promotion
    -Wformat=2
    -Weffc++
)

find_program(CCACHE ccache)
if(CCACHE)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
endif(CCACHE)

set(CMAKE_CXX_CPPCHECK cppcheck)
find_program(CPPCHECK NAMES cppcheck)
if (CPPCHECK)
    list(
        APPEND CPPCHECK
            "--enable=style,performance,warning,portability"
            "--enable=warning"
            "--force"
            "--inconclusive"
            "--inline-suppr"
            "--suppress=cppcheckError"
            "--suppress=internalAstError"
            "--suppress=passedByValue"
            "--suppress=syntaxError"
            "--suppress=unmatchedSuppression"
    )
endif(CPPCHECK)

execute_process(COMMAND git rev-parse --short HEAD
    OUTPUT_VARIABLE GIT_REVISION
    ERROR_QUIET
)

if ("${GIT_REVISION}" STREQUAL "")
    set(VERSION "")
else()
    execute_process(
        COMMAND bash -c "git diff --quiet --exit-code || echo +"
        OUTPUT_VARIABLE GIT_DIRTY
    )

    execute_process(
        COMMAND git describe --exact-match --tags
        OUTPUT_VARIABLE GIT_TAG ERROR_QUIET
    )

    execute_process(
        COMMAND git rev-parse --abbrev-ref HEAD
        OUTPUT_VARIABLE GIT_BRANCH
    )

    string(STRIP "${GIT_REVISION}" GIT_REVISION)
    string(STRIP "${GIT_DIRTY}" GIT_DIRTY)
    string(STRIP "${GIT_TAG}" GIT_TAG)
    string(STRIP "${GIT_BRANCH}" GIT_BRANCH)

    if (NOT "${GIT_TAG}" STREQUAL "")
        set(VERSION "${GIT_TAG}-${GIT_REVISION}${GIT_DIRTY}")
    else()
        set(VERSION "${GIT_BRANCH}/${GIT_REVISION}${GIT_DIRTY}")
    endif()
endif()

if(EXISTS ${CMAKE_SOURCE_DIR}/include/version.hh)
    file(READ ${CMAKE_SOURCE_DIR}/include/version.hh CURRENT_VERSION)
else()
    set(CURRENT_VERSION "")
endif()

if (NOT "#define VERSION \"${VERSION}\"" STREQUAL "${CURRENT_VERSION}")
    file(WRITE
        ${CMAKE_SOURCE_DIR}/include/version.hh
        "#define VERSION \"${VERSION}\""
    )
endif()

file(GLOB_RECURSE KRANEWL_SOURCES RELATIVE ${CMAKE_SOURCE_DIR} src/kranewl/*.cc)
file(GLOB_RECURSE KRANEC_SOURCES RELATIVE ${CMAKE_SOURCE_DIR} src/kranec/*.cc)

add_executable(kranewl
    ${KRANEWL_SOURCES}
)

add_executable(kranec
    ${KRANEC_SOURCES}
)

find_package(spdlog REQUIRED)
target_link_libraries(kranewl PRIVATE spdlog::spdlog)

include(FindPkgConfig)
find_package(PkgConfig)

pkg_search_module(CAIRO REQUIRED cairo)
include_directories(${CAIRO_INCLUDE_DIRS})
link_directories(${CAIRO_LIBRARY_DIRS})

pkg_search_module(PANGOCAIRO REQUIRED pangocairo)
include_directories(${PANGOCAIRO_INCLUDE_DIRS})
link_directories(${PANGOCAIRO_LIBRARY_DIRS})

pkg_search_module(WAYLANDSERVER REQUIRED wayland-server)
include_directories(${WAYLANDSERVER_INCLUDE_DIRS})
link_directories(${WAYLANDSERVER_LIBRARY_DIRS})

pkg_search_module(LIBINPUT REQUIRED libinput)
include_directories(${LIBINPUT_INCLUDE_DIRS})
link_directories(${LIBINPUT_LIBRARY_DIRS})

pkg_search_module(LIBUCL REQUIRED libucl)
include_directories(${LIBUCL_INCLUDE_DIRS})
link_directories(${LIBUCL_LIBRARY_DIRS})

pkg_search_module(PIXMAN REQUIRED pixman-1)
include_directories(${PIXMAN_INCLUDE_DIRS})
link_directories(${PIXMAN_LIBRARY_DIRS})

pkg_search_module(WLROOTS REQUIRED wlroots)
include_directories(${WLROOTS_INCLUDE_DIRS})
link_directories(${WLROOTS_LIBRARY_DIRS})

pkg_search_module(XKBCOMMON REQUIRED xkbcommon)
include_directories(${XKBCOMMON_INCLUDE_DIRS})
link_directories(${XKBCOMMON_LIBRARY_DIRS})

target_compile_features(kranewl PRIVATE cxx_std_20)
target_compile_features(kranec PRIVATE cxx_std_20)

target_include_directories(kranewl PRIVATE
    ${CMAKE_SOURCE_DIR}/src/kranewl
    ${CMAKE_SOURCE_DIR}/include
)

target_include_directories(kranec PRIVATE
    ${CMAKE_SOURCE_DIR}/src/kranec
    ${CMAKE_SOURCE_DIR}/include
)

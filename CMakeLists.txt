cmake_minimum_required(VERSION 3.10.0)

project(kranewl
    VERSION 0.0.1
    DESCRIPTION "A wlroots-based dynamic Wayland compositor, written in C++, configured with Lua"
    HOMEPAGE_URL "%%https://github.com/deurzen/kranewl%%"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(
    $<$<CONFIG:DEBUG>:-w>
    $<$<CONFIG:DEBUG>:-DXWAYLAND>
    $<$<CONFIG:DEBUG>:-O0>
    $<$<CONFIG:DEBUG>:-g>
    $<$<CONFIG:RELEASE>:-w>
    $<$<CONFIG:RELEASE>:-DXWAYLAND>
    $<$<CONFIG:RELEASE>:-DTRACING_DISABLED>
    $<$<CONFIG:RELEASE>:-O2>
)

add_compile_definitions(
    $<$<CONFIG:DEBUG>:DEBUG>
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

find_package(PkgConfig)
pkg_get_variable(WAYLAND_PROTOCOLS wayland-protocols pkgdatadir)

add_custom_command(
    OUTPUT ${CMAKE_SOURCE_DIR}/include/protocols/xdg-shell-protocol.h
    COMMAND wayland-scanner server-header ${WAYLAND_PROTOCOLS}/stable/xdg-shell/xdg-shell.xml ${CMAKE_SOURCE_DIR}/include/protocols/xdg-shell-protocol.h
    COMMENT "Generating xdg-shell-protocol.h"
)

add_custom_command(
    OUTPUT ${CMAKE_SOURCE_DIR}/include/protocols/wlr-layer-shell-unstable-v1-protocol.h
    COMMAND wayland-scanner server-header ${CMAKE_SOURCE_DIR}/protocols/wlr-layer-shell-unstable-v1.xml ${CMAKE_SOURCE_DIR}/include/protocols/wlr-layer-shell-unstable-v1-protocol.h
    COMMENT "Generating layer-shell-protocol.h"
)

add_custom_command(
    OUTPUT ${CMAKE_SOURCE_DIR}/include/protocols/pointer-constraints-unstable-v1-protocol.h
    COMMAND wayland-scanner server-header ${WAYLAND_PROTOCOLS}/unstable/pointer-constraints/pointer-constraints-unstable-v1.xml ${CMAKE_SOURCE_DIR}/include/protocols/pointer-constraints-unstable-v1-protocol.h
    COMMENT "Generating pointer-constraints-unstable-v1.h"
)

add_custom_target(run ALL
    DEPENDS ${CMAKE_SOURCE_DIR}/include/protocols/xdg-shell-protocol.h
    DEPENDS ${CMAKE_SOURCE_DIR}/include/protocols/wlr-layer-shell-unstable-v1-protocol.h
    DEPENDS ${CMAKE_SOURCE_DIR}/include/protocols/pointer-constraints-unstable-v1-protocol.h
)

set_source_files_properties(tags PROPERTIES GENERATED true)
add_custom_command (OUTPUT tags
    COMMAND git ls-files | ctags -R --exclude=.git --c++-kinds=+p --links=no --fields=+iaS --extras=+q -L-
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Generating project tags"
)

add_executable(kranewl
    ${KRANEWL_SOURCES}
    tags
)

add_executable(kranec
    ${KRANEC_SOURCES}
)

pkg_check_modules(spdlog REQUIRED IMPORTED_TARGET spdlog)
pkg_check_modules(lua REQUIRED IMPORTED_TARGET lua5.4)
pkg_check_modules(cairo REQUIRED IMPORTED_TARGET cairo)
pkg_check_modules(pangocairo REQUIRED IMPORTED_TARGET pangocairo)
pkg_check_modules(wayland_server REQUIRED IMPORTED_TARGET wayland-server)
pkg_check_modules(wlroots REQUIRED IMPORTED_TARGET wlroots)
pkg_check_modules(pixman_1 REQUIRED IMPORTED_TARGET pixman-1)
pkg_check_modules(libinput REQUIRED IMPORTED_TARGET libinput)
pkg_check_modules(xkbcommon REQUIRED IMPORTED_TARGET xkbcommon)
pkg_check_modules(xcb REQUIRED IMPORTED_TARGET xcb)

target_compile_features(kranewl PRIVATE cxx_std_20)
target_compile_features(kranec PRIVATE cxx_std_20)
target_compile_definitions(kranewl PRIVATE
    # required to use wlroots' unstable interfaces
    WLR_USE_UNSTABLE
)

target_link_libraries(kranewl
    PRIVATE
    PkgConfig::spdlog
    PkgConfig::lua
    PkgConfig::cairo
    PkgConfig::pangocairo
    PkgConfig::wayland_server
    PkgConfig::wlroots
    PkgConfig::pixman_1
    PkgConfig::libinput
    PkgConfig::xkbcommon
    PkgConfig::xcb
)

target_include_directories(kranewl PRIVATE
    ${CMAKE_SOURCE_DIR}/src/kranewl
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/protocols
)

target_include_directories(kranec PRIVATE
    ${CMAKE_SOURCE_DIR}/src/kranec
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/protocols
)

install(TARGETS kranewl)
install(TARGETS kranec)

include(CTest)

add_test(NAME kranewl
    COMMAND
    kranewl
)

set_tests_properties(kranewl
    PROPERTIES
    ENVIRONMENT WL_BACKEND=x11
)

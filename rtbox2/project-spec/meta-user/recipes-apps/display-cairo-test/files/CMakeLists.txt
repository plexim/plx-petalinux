cmake_minimum_required(VERSION 3.5)

project(display-cairo-test LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_BUILD_TYPE, Release)

add_executable(
    display-cairo-test

    main.cpp
    cairo_demo.cpp
)

find_package(PkgConfig REQUIRED)

pkg_check_modules(CAIROMM REQUIRED IMPORTED_TARGET cairomm-1.0)


target_include_directories(
    display-cairo-test

    PRIVATE ../../../../../../../xsdk/rtbox2/rpu/display_cairo_test_r5/src
)

target_link_libraries(
    display-cairo-test

    PkgConfig::CAIROMM
    metal
    rpumsg
)


install (TARGETS display-cairo-test DESTINATION bin)

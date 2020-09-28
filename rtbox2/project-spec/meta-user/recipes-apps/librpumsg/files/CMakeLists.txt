cmake_minimum_required(VERSION 3.5)

project(rpumsg LANGUAGES C)

set(CMAKE_BUILD_TYPE, Release)


add_library (
    rpumsg
    SHARED

    rpumsg.c
)

target_link_libraries (
    rpumsg

    metal
)

set_property(TARGET rpumsg PROPERTY VERSION "0.0.0")
set_property(TARGET rpumsg PROPERTY SOVERSION 0)

set_property(TARGET rpumsg PROPERTY PUBLIC_HEADER "rpumsg.h")

install (
        TARGETS rpumsg
	LIBRARY DESTINATION lib
	PUBLIC_HEADER DESTINATION include
)


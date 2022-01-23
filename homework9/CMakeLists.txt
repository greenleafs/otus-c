cmake_minimum_required(VERSION 3.0)
project(homework9 VERSION 1.0 LANGUAGES C)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GST REQUIRED "gstreamer-base-1.0")

#set(CMAKE_VERBOSE_MAKEFILE on)
#set(CMAKE_EXPORT_COMPILE_COMMANDS on)

add_library(wavesrc SHARED homework.c)
target_compile_options(wavesrc PUBLIC ${GST_CFLAGS_OTHER})
target_include_directories(wavesrc PUBLIC ${GST_INCLUDE_DIRS})
target_link_libraries(wavesrc ${GST_LIBRARIES})
#set_target_properties(wavesrc PROPERTIES VERSION ${PROJECT_VERSION})
#set_target_properties(wavesrc PROPERTIES SOVERSION 1)

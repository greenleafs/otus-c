cmake_minimum_required(VERSION 3.0)
project(homework9 VERSION 1.0 LANGUAGES C)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GST REQUIRED "gstreamer-1.0")

add_executable(homework9 homework.c)
target_link_libraries(homework9 ${GST_LIBRARIES})
target_include_directories(homework9 PUBLIC ${GST_INCLUDE_DIRS})
target_compile_options(homework9 PUBLIC ${GST_CFLAGS_OTHER})

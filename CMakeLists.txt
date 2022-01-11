cmake_minimum_required(VERSION 3.0)
project("OTUS-C" VERSION 1.0 LANGUAGES C)

add_compile_options(
    -Wall -Wextra -Wpedantic -std=c11
)

add_subdirectory(homework3)
add_subdirectory(homework4)
add_subdirectory(homework8)
add_subdirectory(homework9)

add_custom_target(cleanpy rm -rf homework*/__pycache__)
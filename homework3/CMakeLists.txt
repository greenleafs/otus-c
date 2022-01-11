cmake_minimum_required(VERSION 3.0)
project(homework3 VERSION 1.0 LANGUAGES C)

add_executable(homework3 homework.c encode.c)
add_custom_target(test3 python3 -m unittest -v test)
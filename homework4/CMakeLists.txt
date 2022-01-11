cmake_minimum_required(VERSION 3.0)
project(homework4 VERSION 1.0 LANGUAGES C)

add_executable(homework4 homework.c)
add_custom_target(test4 python3 -m unittest -v test)
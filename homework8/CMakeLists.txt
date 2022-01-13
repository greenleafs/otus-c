cmake_minimum_required(VERSION 3.0)
project(homework8 VERSION 1.0 LANGUAGES C)

add_compile_options(-O3)

add_executable(homework8 homework.c htable.c)
add_executable(htable_test htable_test.c)
add_custom_target(test8 python3 -m unittest -v test)

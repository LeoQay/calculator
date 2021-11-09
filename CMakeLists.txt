cmake_minimum_required(VERSION 3.20)
project(calculator C)

set(CMAKE_C_STANDARD 99)

add_executable(calculator main.c calc.c calc_stack.c var_mgr.c)

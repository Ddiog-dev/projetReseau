cmake_minimum_required(VERSION 3.6)
project(projet_c)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror")

set(SOURCE_FILES main.c)
add_executable(projet_c ${SOURCE_FILES})
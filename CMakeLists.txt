cmake_minimum_required(VERSION 3.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -Wno-unused-variable -fno-rtti")

execute_process(COMMAND gcc -print-file-name=plugin OUTPUT_VARIABLE GCC_PLUGIN_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
set(GCC_INCLUDE_DIR ${GCC_PLUGIN_DIR}/include)
include_directories(${GCC_INCLUDE_DIR})

add_library(hellscape SHARED PassManager.cpp Random.h Viz.cpp Viz.h SUB.cpp SUB.h BCF.cpp BCF.h FLA.cpp FLA.h)
set_target_properties(hellscape PROPERTIES PREFIX "")

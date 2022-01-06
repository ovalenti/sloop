cmake_minimum_required(VERSION 3.8)
project(sloop)

add_library(sloop STATIC blob.c defer.c loop.c sb.c)
target_include_directories(sloop PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
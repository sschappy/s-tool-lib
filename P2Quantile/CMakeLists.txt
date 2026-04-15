cmake_minimum_required(VERSION 3.15)
project(SToolLib)

set(LIB_SOURCES
    P2Quantile.cpp
)

add_library(${PROJECT_NAME} STATIC ${LIB_SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
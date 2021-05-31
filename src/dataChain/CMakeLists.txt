# dataChain - github.com/nielsnl68/dataChain
# Copyright Spectronix NL 2021
# MIT License

cmake_minimum_required(VERSION 3.0)

project(dataChain VERSION 0.01.0)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    include(CTest)
endif()

add_subdirectory(src)

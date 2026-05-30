cmake_minimum_required(VERSION 3.28)
project(TicketSystem)

set(CMAKE_CXX_STANDARD 20)

add_executable(TicketSystem main.cpp
        src/vector/utility.h
        src/vector/vector.h
        src/vector/exceptions.h)

target_include_directories(TicketSystem PRIVATE ${CMAKE_SOURCE_DIR}/src)
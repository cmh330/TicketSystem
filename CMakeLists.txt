cmake_minimum_required(VERSION 3.28)
project(TicketSystem)

set(CMAKE_CXX_STANDARD 20)

file(GLOB_RECURSE HEADERS "src/*.h")

add_executable(code main.cpp ${HEADERS}
        src/orderManager.h
        src/storage.h)

target_include_directories(code PRIVATE ${CMAKE_SOURCE_DIR}/src)
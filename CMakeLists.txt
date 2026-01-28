cmake_minimum_required(VERSION 3.28)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
add_definitions(-DUNICODE -D_UNICODE)

project(inheritance_chess_project)

include_directories("include")
file(GLOB SOURCES "src/*.cpp")
add_executable(ichess_runner)
target_sources(ichess_runner PRIVATE ${SOURCES} main.cpp)

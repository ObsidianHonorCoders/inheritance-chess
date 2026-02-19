cmake_minimum_required(VERSION 3.28)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
add_definitions(-DUNICODE -D_UNICODE)

include(FetchContent)
set(FETCHCONTENT_BASE_DIR "${CMAKE_SOURCE_DIR}/tests/build_gtest")
FetchContent_Declare( googletest
                      URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
                      EXCLUDE_FROM_ALL )
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()
file(GLOB TEST_SOURCES "./test_*.cpp")
add_executable("test_${EXE_NAME}" ${TEST_SOURCES})
target_link_libraries("test_${EXE_NAME}" gtest gtest_main gmock )
target_include_directories("test_${EXE_NAME}" PRIVATE "${CMAKE_SOURCE_DIR}/include")
file(GLOB PROJECT_SOURCES "${CMAKE_SOURCE_DIR}/src/*.cpp")
target_sources("test_${EXE_NAME}" PRIVATE ${PROJECT_SOURCES})
add_test(COMMAND "test_${EXE_NAME}" NAME "${EXE_NAME}_Tests")

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

# Legacy test support (leveraging existing test patterns)
file(GLOB LEGACY_TEST_SOURCES "./test_*.cpp")
add_executable("test_${EXE_NAME}" ${LEGACY_TEST_SOURCES})
target_link_libraries("test_${EXE_NAME}" gtest gtest_main gmock )
target_include_directories("test_${EXE_NAME}" PRIVATE "${CMAKE_SOURCE_DIR}/include")
file(GLOB LEGACY_PROJECT_SOURCES "${CMAKE_SOURCE_DIR}/src/*.cpp")
target_sources("test_${EXE_NAME}" PRIVATE ${LEGACY_PROJECT_SOURCES})

# New CCI test support (adapted from existing test structure)
file(GLOB CCI_TEST_SOURCES "../cci/tests/test_*.cpp")
target_sources("test_${EXE_NAME}" PRIVATE ${CCI_TEST_SOURCES})
target_include_directories("test_${EXE_NAME}" PRIVATE "${CMAKE_SOURCE_DIR}/cci/include")
file(GLOB CCI_PROJECT_SOURCES "${CMAKE_SOURCE_DIR}/cci/src/*.cpp")
target_sources("test_${EXE_NAME}" PRIVATE ${CCI_PROJECT_SOURCES})

# Future modular component tests (Phase 2) - structure ready
# file(GLOB CBM_TEST_SOURCES "../cbm/tests/test_*.cpp")
# file(GLOB CPL_TEST_SOURCES "../cpl/tests/test_*.cpp")
# file(GLOB CTU_TEST_SOURCES "../ctu/tests/test_*.cpp")
# file(GLOB CGH_TEST_SOURCES "../cgh/tests/test_*.cpp")

# target_sources("test_${EXE_NAME}" PRIVATE ${CBM_TEST_SOURCES} ${CPL_TEST_SOURCES} ${CTU_TEST_SOURCES} ${CGH_TEST_SOURCES})

# target_include_directories("test_${EXE_NAME}" PRIVATE "${CMAKE_SOURCE_DIR}/cbm/include")
# target_include_directories("test_${EXE_NAME}" PRIVATE "${CMAKE_SOURCE_DIR}/cpl/include")
# target_include_directories("test_${EXE_NAME}" PRIVATE "${CMAKE_SOURCE_DIR}/ctu/include")
# target_include_directories("test_${EXE_NAME}" PRIVATE "${CMAKE_SOURCE_DIR}/cgh/include")

# file(GLOB CBM_PROJECT_SOURCES "${CMAKE_SOURCE_DIR}/cbm/src/*.cpp")
# file(GLOB CPL_PROJECT_SOURCES "${CMAKE_SOURCE_DIR}/cpl/src/*.cpp")
# file(GLOB CTU_PROJECT_SOURCES "${CMAKE_SOURCE_DIR}/ctu/src/*.cpp")
# file(GLOB CGH_PROJECT_SOURCES "${CMAKE_SOURCE_DIR}/cgh/src/*.cpp")

# target_sources("test_${EXE_NAME}" PRIVATE ${CBM_PROJECT_SOURCES} ${CPL_PROJECT_SOURCES} ${CTU_PROJECT_SOURCES} ${CGH_PROJECT_SOURCES})

add_test(COMMAND "test_${EXE_NAME}" NAME "${EXE_NAME}_Tests")

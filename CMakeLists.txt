cmake_minimum_required(VERSION 3.20)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
add_definitions(-DUNICODE -D_UNICODE)

project(inheritance_chess_project VERSION 1.0.0 LANGUAGES CXX)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler-specific options
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Add subdirectories for each component
add_subdirectory(cci)
add_subdirectory(cpl)
add_subdirectory(cbm)
add_subdirectory(ctu)
add_subdirectory(ces)
add_subdirectory(cgh)
add_subdirectory(cei)

# Main executable
add_executable(${EXE_NAME} main.cpp)

# Link main executable with all component libraries
target_link_libraries(${EXE_NAME}
    cci
    cpl
    cbm
    ctu
    ces
    cgh
    cei
)

# Add test options
option(BUILD_ALL_TESTS "Build all component tests" OFF)
option(BUILD_CCI_TESTS "Build CCI tests" OFF)
option(BUILD_CPL_TESTS "Build CPL tests" OFF)
option(BUILD_CGH_TESTS "Build CGH tests" OFF)
option(BEI_TESTS "Build CEI tests" OFF)
option(BUILD_CES_TESTS "Build CES tests" OFF)

# Enable testing if any test option is ON
if(BUILD_ALL_TESTS OR BUILD_CCI_TESTS OR BUILD_CPL_TESTS OR BUILD_CGH_TESTS OR BUILD_CEI_TESTS OR BUILD_CES_TESTS)
    enable_testing()
    
    # Set all test options to ON if BUILD_ALL_TESTS is ON
    if(BUILD_ALL_TESTS)
        set(BUILD_CCI_TESTS ON)
        set(BUILD_CPL_TESTS ON)
        set(BUILD_CGH_TESTS ON)
        set(BUILD_CEI_TESTS ON)
        set(BUILD_CES_TESTS ON)
    endif()
endif()

# Add tests (legacy support)
add_subdirectory(tests)

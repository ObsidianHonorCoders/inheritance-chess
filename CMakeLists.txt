cmake_minimum_required(VERSION 3.20)

project(
    inheritance_chess_project
    VERSION 0.0.0
    LANGUAGES CXX
    DESCRIPTION "Modular C++ chess engine"
    HOMEPAGE_URL "https://github.com/ObsidianHonorCoders/inheritance-chess"
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_definitions(-DUNICODE -D_UNICODE)

if(NOT DEFINED EXE_NAME)
    set(EXE_NAME
        "ichess_runner"
        CACHE STRING "Main interactive executable name" FORCE)
endif()

option(OHC_ENABLE_WERROR "Treat compiler warnings as errors" ON)

# CTest baseline option (BUILD_TESTING)
include(CTest)

# Backward compatibility with older ENABLE_TESTS option.
option(ENABLE_TESTS "(Deprecated) Build centralized test suite target" ${BUILD_TESTING})
set(BUILD_TESTING ${ENABLE_TESTS} CACHE BOOL "Build tests" FORCE)

# Build type default for single-config generators
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Compiler-specific options
if(MSVC)
    add_compile_options(/W4)
    if(OHC_ENABLE_WERROR)
        add_compile_options(/WX)
    endif()
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
    if(OHC_ENABLE_WERROR)
        add_compile_options(-Werror)
    endif()
endif()

# Add subdirectories for each component
add_subdirectory(chess_common_interface)
add_subdirectory(chess_pieces_logic)
add_subdirectory(chess_board_manager)
add_subdirectory(chess_translation_unit)
add_subdirectory(chess_event_system)
add_subdirectory(chess_game_handler)
add_subdirectory(chess_engine_interface)

# Main executable
add_executable(${EXE_NAME} chess_game_handler/src/ichess_runner.cpp)

# Link main executable with all component libraries
target_link_libraries(${EXE_NAME} PRIVATE
    cci
    cpl
    cbm
    ctu
    ces
    cgh
    cei
)

# Two-player console game (always built alongside the main executable)
add_executable(ichess_2p chess_game_handler/src/2p_game.cpp)
target_link_libraries(ichess_2p PRIVATE
    cci
    cpl
    cbm
    ctu
    ces
    cgh
    cei
)

# Add centralized tests
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

# Installation and package export
include(GNUInstallDirs)

install(
    TARGETS
        cci
        cpl
        cbm
        ctu
        ces
        cgh
        cei
        ${EXE_NAME}
        ichess_2p
    EXPORT ${PROJECT_NAME}-targets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
    DIRECTORY chess_common_interface/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/chess_common_interface
    FILES_MATCHING
    PATTERN "*.hpp"
)

install(
    DIRECTORY chess_pieces_logic/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/chess_pieces_logic
    FILES_MATCHING
    PATTERN "*.hpp"
)

install(
    DIRECTORY chess_board_manager/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/chess_board_manager
    FILES_MATCHING
    PATTERN "*.hpp"
)

install(
    DIRECTORY chess_translation_unit/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/chess_translation_unit
    FILES_MATCHING
    PATTERN "*.hpp"
)

install(
    DIRECTORY chess_event_system/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/chess_event_system
    FILES_MATCHING
    PATTERN "*.hpp"
)

install(
    DIRECTORY chess_game_handler/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/chess_game_handler
    FILES_MATCHING
    PATTERN "*.hpp"
)

install(
    DIRECTORY chess_engine_interface/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/chess_engine_interface
    FILES_MATCHING
    PATTERN "*.hpp"
)

install(
    EXPORT ${PROJECT_NAME}-targets
    FILE ${PROJECT_NAME}-targets.cmake
    NAMESPACE ${PROJECT_NAME}::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

include(CMakePackageConfigHelpers)
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/${PROJECT_NAME}-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

include(CPack)

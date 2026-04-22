cmake_minimum_required(VERSION 3.28)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
add_definitions(-DUNICODE -D_UNICODE)

project(inheritance_chess_project)

# Legacy support for existing structure (leveraging existing patterns)
include_directories("include")
file(GLOB LEGACY_SOURCES "src/*.cpp")
add_executable(${EXE_NAME})
target_sources(${EXE_NAME} PRIVATE ${LEGACY_SOURCES} main.cpp)

# New modular CCI component support
include_directories("cci/include")
file(GLOB CCI_SOURCES "cci/src/*.cpp")
target_sources(${EXE_NAME} PRIVATE ${CCI_SOURCES})

# CPL component support (Phase 2) - now active
include_directories("cpl/include")
file(GLOB CPL_SOURCES "cpl/src/*.cpp")
target_sources(${EXE_NAME} PRIVATE ${CPL_SOURCES})

# Future modular components (Phase 2) - structure ready
# include_directories("cbm/include")
# include_directories("ctu/include")
# include_directories("cgh/include")

# file(GLOB CBM_SOURCES "cbm/src/*.cpp")
# file(GLOB CTU_SOURCES "ctu/src/*.cpp")
# file(GLOB CGH_SOURCES "cgh/src/*.cpp")

# target_sources(${EXE_NAME} PRIVATE ${CBM_SOURCES} ${CTU_SOURCES} ${CGH_SOURCES})

# Add tests (leveraging existing test structure)
add_subdirectory(tests)

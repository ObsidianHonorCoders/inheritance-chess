cmake_minimum_required(VERSION 3.28)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
add_definitions(-DUNICODE -D_UNICODE)

project(inheritance_chess_project)

# New modular CCI component support
include_directories("cci/include")
file(GLOB CCI_SOURCES "cci/src/*.cpp")
add_executable(${EXE_NAME})
target_sources(${EXE_NAME} PRIVATE ${CCI_SOURCES} main.cpp)

# CPL component support (Phase 2) - now active
include_directories("cpl/include")
file(GLOB CPL_SOURCES "cpl/src/*.cpp")
target_sources(${EXE_NAME} PRIVATE ${CPL_SOURCES})

# CGH component support (Phase 3) - now active
include_directories("cgh/include")
file(GLOB CGH_SOURCES "cgh/src/*.cpp")
target_sources(${EXE_NAME} PRIVATE ${CGH_SOURCES})

# CBM component support (Phase 3) - now active  
include_directories("cbm/include")
file(GLOB CBM_SOURCES "cbm/src/*.cpp")
target_sources(${EXE_NAME} PRIVATE ${CBM_SOURCES})

# CTU component support (Phase 3) - now active
include_directories("ctu/include")
file(GLOB CTU_SOURCES "ctu/src/*.cpp")
target_sources(${EXE_NAME} PRIVATE ${CTU_SOURCES})

# CEI component support (Phase 4) - now active
include_directories("cei/include")
file(GLOB CEI_SOURCES "cei/src/*.cpp")
target_sources(${EXE_NAME} PRIVATE ${CEI_SOURCES})


# Add tests (leveraging existing test structure)
add_subdirectory(tests)

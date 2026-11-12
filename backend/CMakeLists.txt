cmake_minimum_required(VERSION 3.20)

include(FetchContent)

set(HTTPLIB_INSTALL OFF CACHE BOOL "" FORCE)
set(HTTPLIB_TEST OFF CACHE BOOL "" FORCE)
set(HTTPLIB_USE_OPENSSL_IF_AVAILABLE OFF CACHE BOOL "" FORCE)
set(HTTPLIB_USE_ZLIB_IF_AVAILABLE OFF CACHE BOOL "" FORCE)
FetchContent_Declare(cpp_httplib
    URL https://github.com/yhirose/cpp-httplib/archive/refs/tags/v0.20.1.zip
    URL_HASH SHA256=4015175C311247404064E7D3D78CF21D6B950B66034DE22C726AD788263A8512
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(cpp_httplib)

FetchContent_Declare(nlohmann_json
    URL https://github.com/nlohmann/json/archive/refs/tags/v3.11.3.zip
    URL_HASH SHA256=04022B05D806EB5FF73023C280B68697D12B93E1B7267A0B22A1A39EC7578069
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(nlohmann_json)

FetchContent_Declare(sqlite3
    URL https://www.sqlite.org/2025/sqlite-amalgamation-3500400.zip
    URL_HASH SHA256=1D3049DD0F830A025A53105FC79FD2AB9431AEA99E137809D064D8EE8356B032
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(sqlite3)

add_library(ichess_sqlite STATIC ${sqlite3_SOURCE_DIR}/sqlite3.c)
target_include_directories(ichess_sqlite PUBLIC ${sqlite3_SOURCE_DIR})
target_compile_definitions(ichess_sqlite PRIVATE
    SQLITE_THREADSAFE=1
    SQLITE_DEFAULT_FOREIGN_KEYS=1
    SQLITE_DQS=0
    SQLITE_OMIT_LOAD_EXTENSION
)

add_library(ichess_backend STATIC
    src/http_api.cpp
    src/sqlite_database.cpp
    src/sqlite_game_application.cpp
    src/session_authenticator.cpp
    src/service_runtime.cpp
)

target_include_directories(ichess_backend PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(ichess_backend
    PUBLIC
        cgh
        ctu
        cei
        httplib::httplib
        nlohmann_json::nlohmann_json
        ichess_sqlite
)

set_property(TARGET ichess_backend PROPERTY CXX_STANDARD 20)
set_property(TARGET ichess_backend PROPERTY CXX_STANDARD_REQUIRED ON)

if(WIN32)
    target_link_libraries(ichess_backend PRIVATE bcrypt advapi32)
endif()

add_executable(ichess_service src/server_main.cpp)
target_link_libraries(ichess_service PRIVATE ichess_backend)

if(MSVC)
    target_compile_options(ichess_backend PRIVATE /W4)
else()
    target_compile_options(ichess_backend PRIVATE -Wall -Wextra -Wpedantic)
endif()

cmake_minimum_required(VERSION 3.16)
project(LogicEngine CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# ── Crow (header-only HTTP server) ────────────────────────────────────────────
FetchContent_Declare(Crow
    GIT_REPOSITORY https://github.com/CrowCpp/Crow.git
    GIT_TAG        v1.0+5
)
FetchContent_MakeAvailable(Crow)

find_package(Threads REQUIRED)

add_executable(logic_server
    src/server.cpp
    src/formula.cpp
    src/parser.cpp
    src/inference_engine.cpp
)

target_include_directories(logic_server PRIVATE include)
target_link_libraries(logic_server
    PRIVATE
    Crow::Crow
    Threads::Threads
)

target_compile_options(logic_server PRIVATE
    $<$<CONFIG:Release>:-O2>
)

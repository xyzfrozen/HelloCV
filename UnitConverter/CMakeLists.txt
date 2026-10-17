cmake_minimum_required(VERSION 3.16)
project(UnitConverter VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(unitlib)

add_executable(unit_converter src/main.cpp)
target_link_libraries(unit_converter PRIVATE unitlib)
target_compile_definitions(unit_converter PRIVATE UNIT_CONVERTER_VERSION="${PROJECT_VERSION}")
target_compile_options(unit_converter PRIVATE -Wall -Wextra -Wpedantic)

install(TARGETS unit_converter RUNTIME DESTINATION bin)

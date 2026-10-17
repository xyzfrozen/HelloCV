add_library(unitlib STATIC src/unitlib.cpp)

target_include_directories(unitlib PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
target_compile_options(unitlib PRIVATE -Wall -Wextra -Wpedantic)

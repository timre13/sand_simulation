cmake_minimum_required(VERSION 3.10)

project(sand_sim VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED true)

set(CMAKE_C_FLAGS "-ggdb3 -D_REENTRANT -Wall -Wextra -pedantic-errors")
#set(CMAKE_CXX_FLAGS "-ggdb3 -D_REENTRANT -Wall -Wextra -pedantic-errors -fsanitize=address,leak,undefined")

SET(CMAKE_EXPORT_COMPILE_COMMANDS true)

include_directories(/usr/include/SDL2)
link_libraries(SDL2 SDL2_ttf)

add_executable(sand_sim
    src/main.cpp
)

# TODO what's an appropriate version for this?
cmake_minimum_required(VERSION 3.21)

# c standard to use
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_EXTENSIONS OFF)

# build a compilation database, usable by clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

#project name
project(elite)

add_compile_options(-g -Wall)

# output a static library elite.a
add_library(elite elite.c macrolist.c)

# tests
add_subdirectory(tests)

## WIP: build documentation
#add_custom_target(doc ALL COMMAND doxygen Doxyfile)

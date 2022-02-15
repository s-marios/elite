# using cmake_c_standard 17 requires version 3.21
cmake_minimum_required(VERSION 3.21)

#project name
project(elite)

# TODO: selecting compiler doesn't seem to work?
#set(ENV{CC} "clang")

# c standard to use
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_EXTENSIONS OFF)


# build a compilation database, usable by clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# enable debug info & all warnings
add_compile_options(-g -Wall)

# output a static library elite.a
add_library(elite elite.c macrolist.c elite.h macrolist.h logging.h networking.h)

# tests
add_subdirectory(tests)


## WIP: build documentation
find_package(Doxygen)

if (DOXYGEN_FOUND)
	set(DOXYGEN_OUTPUT_DIRECTORY Documentation)
	doxygen_add_docs(doxygen ALL)
else()
	message(STATUS "doxygen not found. Please install doxygen to generate documentation")
endif()

## WIP: distribution of header files?
#add_custom_target(headers
#	ALL
#	COMMAND cp *.h ${CMAKE_CURRENT_BINARY_DIR}/
#	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
#)

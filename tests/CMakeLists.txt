# TODO what's an appropriate version for this?
cmake_minimum_required(VERSION 3.21)

# build a compilation database, usable by clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# project name
project(elite-tests)


# main test executable, call it with ctest?
add_executable(tests tests.c)
# requires pthread, cmocka and elite
target_link_libraries(tests pthread elite cmocka)

# testing
include(CTest)

add_test(NAME tests COMMAND tests)

# explicitly invoke tests, just to see output when compiling
add_custom_target(run_tests ALL tests)
add_dependencies(run_tests tests)

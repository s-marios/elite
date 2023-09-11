# Check if building as an ESP-IDF component
if(DEFINED ESP_PLATFORM)
  idf_component_register(SRCS "elite.c" "macrolist.c"
    INCLUDE_DIRS "./"
    REQUIRES freertos lwip)
else()

  # Main Project
  # Using cmake_c_standard 17 and PROJECT_IS_TOP_LEVEL
  # requires version 3.21
  cmake_minimum_required(VERSION 3.21)

  # project name
  project(elite)

  # C standard to use
  set(CMAKE_C_STANDARD 17)
  set(CMAKE_C_EXTENSIONS OFF)

  # Build a compilation database, usable by clangd
  set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

  # Enable debug info & all warnings
  add_compile_options(-g -Wall)

  # Output a static library elite.a
  add_library(elite elite.c macrolist.c elite.h macrolist.h logging.h networking.h)

  # Only do tests and documentation if this is the top-level project
  if(PROJECT_IS_TOP_LEVEL)
    add_subdirectory(tests)

    find_package(Doxygen)
    if (DOXYGEN_FOUND)
      set(DOXYGEN_OUTPUT_DIRECTORY Documentation)
      doxygen_add_docs(doxygen ALL)
    else()
      message(STATUS "doxygen not found.
        Please install doxygen to generate documentation")
    endif()
  endif()
endif()

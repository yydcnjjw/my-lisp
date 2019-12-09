cmake_minimum_required(VERSION 3.11)
project(my-lisp C)

SET(EXECUTABLE_OUTPUT_PATH "${PROJECT_BINARY_DIR}/bin")
SET(LIBRARY_OUTPUT_PATH "${PROJECT_BINARY_DIR}/lib")

add_subdirectory(src)
add_subdirectory(test)

CMAKE_MINIMUM_REQUIRED(VERSION 3.14)
SET(PROGRAM_NAME mxc_v4l2_capture)

message("CMAKE_SYSTEM_PROCESSOR ${CMAKE_SYSTEM_PROCESSOR}")
SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_STANDARD_REQUIRED TRUE)
SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG")
SET(CMAKE_CXX_FLAGS "-O2 -save-temps -g")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -D_DEBUG -g -O0")

# SET( CMAKE_SYSTEM_PROCESSOR "aarch64" CACHE STRING "arm64 cross compiler" FORCE)
message("cross compile arm64-linux, ${CMAKE_CURRENT_LIST_LINE}")
SET(CMAKE_C_COMPILER "/usr/bin/aarch64-linux-gnu-gcc")
SET(CMAKE_CXX_COMPILER "/usr/bin/aarch64-linux-gnu-g++")
SET(VCPKG_TARGET_TRIPLET "arm64-linux" CACHE STRING "VCPKG Target Triplet to use")
message("cross compile with ${VCPKG_TARGET_TRIPLET},${CMAKE_CURRENT_LIST_LINE}")
SET( CMAKE_CROSSCOMPILE_PROCESS "aarch64" CACHE STRING "")


message("\n")
message("====================================================")
message("CMAKE_SYSTEM_PROCESSOR ${CMAKE_SYSTEM_PROCESSOR}")
message("CMAKE_CROSSCOMPILE_PROCESS ${CMAKE_CROSSCOMPILE_PROCESS}")
message("cross compile with ${VCPKG_TARGET_TRIPLET}")
message("cross compile C Compiler ${CMAKE_C_COMPILER}")
message("cross compile CXX Compiler ${CMAKE_CXX_COMPILER}")
message("cross compile CXX Compile options ${CMAKE_CXX_FLAGS}")
message("CMAKE_CXX_COMPILER_ID ${CMAKE_CXX_COMPILER_ID}")
message("====================================================")
message("\n")



IF(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    OPTION(BUILD_SHARED_LIBS "Build using shared libraries" OFF)
ELSE()
    SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY "../../../lib")
    SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "../bin/${VCPKG_TARGET_TRIPLET}/${CMAKE_BUILD_TYPE}/")
    OPTION(BUILD_SHARED_LIBS "Build using shared libraries" OFF)
ENDIF()


PROJECT(${PROGRAM_NAME})
ADD_EXECUTABLE(${PROGRAM_NAME} mxc_v4l2_capture.c )



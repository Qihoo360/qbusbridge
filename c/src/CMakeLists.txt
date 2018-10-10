# CMakeLists.txt for LightningServer

CMAKE_MINIMUM_REQUIRED(VERSION 2.4)

PROJECT(QBus_c)

#============================= check =========================================================
IF (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR)
    MESSAGE(FATAL_ERROR "
    CMake generation for ${PROJECT_NAME} not allowed within the #source directory!
    Remove the CMakeCache.txt file and try again from another folder, e.g.:

    rm -rf CMakeFiles CMakeCache.txt
    mkdir cmake-make (or mkdir build)
    cd cmake-make (or cd build)
    cmake -G \"Unix Makefiles\" ../
    ")
ENDIF (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR)

SET(CMAKE_VERBOSE_MAKEFILE TRUE)

SET(CMAKE_USE_RELATIVE_PATHS TRUE)

IF (NOT CMAKE_BUILD_TYPE)

    MESSAGE(STATUS "set default as MINSIZEREL")

    SET(CMAKE_BUILD_TYPE MINSIZEREL)

    SET(CMAKE_CONFIGURARION_TYPES "MINSIZEREL")

ENDIF(NOT CMAKE_BUILD_TYPE)

#============================= sources ========================================================
SET(PROJECT_ALL_FILES
    ./qbus_consumer.cc
    ./qbus_producer.cc
    )

INCLUDE_DIRECTORIES(
    ./
    ../../cxx
    )
#=============================== build =========================================================
IF (CMAKE_BUILD_TYPE MATCHES Debug)
  MESSAGE(STATUS "${PROJECT_NAME} Debug")
  SET(CMAKE_CONFIGURATION_TYPES "Debug")
  SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY "../lib/debug")
ELSE (CMAKE_BUILD_TYPE MATCHES Debug)
  MESSAGE(STATUS "${PROJECT_NAME} Release")
  SET(CMAKE_CONFIGURATION_TYPES "Release")
  SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY "../lib/release")
ENDIF (CMAKE_BUILD_TYPE MATCHES Debug)

    ADD_DEFINITIONS(
        )

    ADD_LIBRARY(${PROJECT_NAME} SHARED ${PROJECT_ALL_FILES} )

    SET(CMAKE_FIND_LIBRARY_SUFFIXES .so)

IF (CMAKE_BUILD_TYPE MATCHES Debug)
    FIND_LIBRARY(LIBQBUS_LIB NAMES QBus PATHS ${CMAKE_BINARY_DIR}/../../cxx/lib/debug NO_DEFAULT_PATH)
ELSE (CMAKE_BUILD_TYPE MATCHES Debug)
    FIND_LIBRARY(LIBQBUS_LIB NAMES QBus PATHS ${CMAKE_BINARY_DIR}/../../cxx/lib/release NO_DEFAULT_PATH)
ENDIF (CMAKE_BUILD_TYPE MATCHES Debug)

    MESSAGE(STATUS "QBus lib path: ${LIBQBUS_LIB}")

    TARGET_LINK_LIBRARIES(${PROJECT_NAME}
        ${LIBQBUS_LIB}
        stdc++
        )

    MESSAGE(STATUS "source dir: ${PROJECT_SOURCE_DIR}")
IF (CMAKE_BUILD_TYPE MATCHES Debug)
    SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "-O0 -g -Wall" OUTPUT_NAME ${PROJECT_NAME})
ELSE (CMAKE_BUILD_TYPE MATCHES Debug)
    SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "-O2 -Wall" OUTPUT_NAME ${PROJECT_NAME})
ENDIF (CMAKE_BUILD_TYPE MATCHES Debug)


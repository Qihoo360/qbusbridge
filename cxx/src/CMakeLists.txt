cmake_minimum_required(VERSION 3.1)

project(QBus CXX)

set(CMAKE_CXX_STANDARD 11)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_INSTALL_PREFIX "${PROJECT_SOURCE_DIR}/..")
string(CONCAT CMAKE_CXX_FLAGS "-D_GLIBCXX_USE_CXX11_ABI=0 " ${CMAKE_CXX_FLAGS})
if (CMAKE_BUILD_TYPE MATCHES Debug)
    string(CONCAT CMAKE_CXX_FLAGS "-g -O0 " ${CMAKE_CXX_FLAGS})
    set(INSTALL_SUBDIR "debug")
else (CMAKE_BUILD_TYPE MATCHES Debug)
    string(CONCAT CMAKE_CXX_FLAGS "-DNDEBUG -O2 " ${CMAKE_CXX_FLAGS})
    set(INSTALL_SUBDIR "release")
endif ()
message(STATUS "CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")

set(CMAKE_PREFIX_PATH
    ${CMAKE_PREFIX_PATH}
    ${PROJECT_SOURCE_DIR}/../thirdparts/local
    )

set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# find qbus::kafka's dependencies
find_path(LIBRDKAFKA_INCLUDE_DIR NAMES "librdkafka/rdkafka.h")
find_library(LIBRDKAFKA_LIB NAMES librdkafka.a)

if (NOT LIBRDKAFKA_INCLUDE_DIR OR NOT LIBRDKAFKA_LIB)
    message(FATAL_ERROR "Can't find librdkafka")
endif ()
message(STATUS "LIBRDKAFKA_INCLUDE_DIR=${LIBRDKAFKA_INCLUDE_DIR}")
message(STATUS "LIBRDKAFKA_LIB=${LIBRDKAFKA_LIB}")

find_path(LOG4CPLUS_INCLUDE_DIR NAMES "log4cplus/logger.h")
find_library(LOG4CPLUS_LIB NAMES liblog4cplus.a)
if (NOT LOG4CPLUS_INCLUDE_DIR OR NOT LOG4CPLUS_LIB)
    MESSAGE(FATAL_ERROR "Can't find log4cplus")
endif ()
add_definitions("-Wno-deprecated-declarations")

message(STATUS "LOG4CPLUS_INCLUDE_DIR=${LOG4CPLUS_INCLUDE_DIR}")
message(STATUS "LOG4CPLUS_LIB=${LOG4CPLUS_LIB}")

set(HEADERS
    qbus_consumer.h
    qbus_producer.h
    )

set(SOURCES
    ${SOURCES}
    kafka/qbus_config.cc
    kafka/qbus_consumer.cc
    kafka/qbus_helper.cc
    kafka/qbus_producer.cc
    kafka/qbus_producer_imp_map.cc
    kafka/qbus_rdkafka.cc
    kafka/qbus_record_msg.cc
    kafka/qbus_thread.cc
    kafka/qbus_topic_partition_set.cc
    kafka/util/logger.cc
    kafka/util/version.cc
    qbus_consumer.cc
    qbus_producer.cc
    )

if (NOT DEFINED LIBNAME)
    set(LIBNAME ${PROJECT_NAME})
endif ()
add_library(${LIBNAME} SHARED ${SOURCES})
include_directories(..
    ${EXTRA_INCLUDE_DIRS}
    ${LIBRDKAFKA_INCLUDE_DIR}
    ${LOG4CPLUS_INCLUDE_DIR}
    )
target_link_libraries(${LIBNAME}
    ${EXTRA_LIBS}
    ${LIBRDKAFKA_LIB}
    ${LOG4CPLUS_LIB}
    ${PROTOBUF_LIBS}
    dl
    pthread
    rt
    z
    -static-libstdc++
    )

install(FILES ${HEADERS} DESTINATION include)
install(TARGETS ${LIBNAME} LIBRARY DESTINATION "lib/${INSTALL_SUBDIR}")

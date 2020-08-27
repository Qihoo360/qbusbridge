#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.
#

file(GLOB PULSAR_SOURCES *.cc *.h lz4/*.c lz4/*.h checksum/*.cc checksum/*.h stats/*.cc stats/*.h c/*.cc c/*.h auth/*.cc auth/*.h auth/athenz/*.cc auth/athenz/*.h)

execute_process(COMMAND python ${CMAKE_SOURCE_DIR}/../src/get-project-version.py OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE PV)
set (CMAKE_CXX_FLAGS " ${CMAKE_CXX_FLAGS} -D_PULSAR_VERSION_=\\\"${PV}\\\"")

if (NOT PROTOC_PATH)
    set(PROTOC_PATH protoc)
endif()

set(LIB_AUTOGEN_DIR ${AUTOGEN_DIR}/lib)
file(MAKE_DIRECTORY ${LIB_AUTOGEN_DIR})
include_directories(${LIB_AUTOGEN_DIR})

# Protobuf generation is only supported natively starting from CMake 3.8
# Using custom command for now
set(PROTO_SOURCES ${LIB_AUTOGEN_DIR}/PulsarApi.pb.cc ${LIB_AUTOGEN_DIR}/PulsarApi.pb.h)
set(PULSAR_SOURCES ${PULSAR_SOURCES} ${PROTO_SOURCES})
ADD_CUSTOM_COMMAND(
         OUTPUT ${PROTO_SOURCES}
         COMMAND ${PROTOC_PATH} -I ../../pulsar-common/src/main/proto ../../pulsar-common/src/main/proto/PulsarApi.proto --cpp_out=${LIB_AUTOGEN_DIR}
         DEPENDS
         ../../pulsar-common/src/main/proto/PulsarApi.proto
         WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

set(LIBRARY_VERSION $ENV{PULSAR_LIBRARY_VERSION})
if (NOT LIBRARY_VERSION)
    set(LIBRARY_VERSION ${PV})
endif(NOT LIBRARY_VERSION)

add_library(pulsarStatic STATIC ${PULSAR_SOURCES})
set_property(TARGET pulsarStatic PROPERTY OUTPUT_NAME pulsar)
set_property(TARGET pulsarStatic PROPERTY VERSION ${LIBRARY_VERSION})
target_compile_definitions(pulsarStatic PRIVATE PULSAR_STATIC)
MESSAGE(STATUS "VERSION:  ${LIBRARY_VERSION}")

# Install regular libpulsar.a
target_link_libraries(pulsarStatic ${COMMON_LIBS})
install(TARGETS pulsarStatic DESTINATION lib)

install(TARGETS pulsarStatic DESTINATION lib)
install(DIRECTORY "../include/pulsar" DESTINATION include)

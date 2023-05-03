cmake_minimum_required(VERSION 3.10.2)
project(document_ut)

set(GAUSE_SOURCE_DIR "${PROJECT_SOURCE_DIR}")

file(GLOB_RECURSE DOCUMENT_SRC ${GAUSE_SOURCE_DIR}/src/*.cpp)
message(STATUS "hanlu ${GAUSE_SOURCE_DIR} ${DOCUMENT_SRC}")
include_directories(
    ${GAUSE_SOURCE_DIR}/include
    ${GAUSE_SOURCE_DIR}/src/storage
    ${GAUSE_SOURCE_DIR}/src/storage/include
    ${GAUSE_SOURCE_DIR}/src/storage/src
    ${GAUSE_SOURCE_DIR}/src/common
    ${GAUSE_SOURCE_DIR}/src/common/include
    ${GAUSE_SOURCE_DIR}/src/executor/include
    ${GAUSE_SOURCE_DIR}/src/executor/base
    ${GAUSE_SOURCE_DIR}/src/executor/document
    ${GAUSE_SOURCE_DIR}/src/oh_adapter
    ${GAUSE_SOURCE_DIR}/src/oh_adapter/include
    ${GAUSE_SOURCE_DIR}/src/oh_adapter/src
    ${GAUSE_SOURCE_DIR}/src/interface
    ${GAUSE_SOURCE_DIR}/src/interface/include
    ${GAUSE_SOURCE_DIR}/src/interface/src
    ${GAUSE_SOURCE_DIR}/third_party/third_party_cJSON
)
aux_source_directory( ${GAUSE_SOURCE_DIR}/third_party/third_party_cJSON DOCUMENT_SRC)
set(MOCK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../mock")
set(KV_STORE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../kv_store")
include(${MOCK_DIR}/include/CMakeLists.txt OPTIONAL)
include(${KV_STORE_DIR}/interfaces/CMakeLists.txt OPTIONAL)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../utils_native/base/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../utils_native/safwk/native/include)
message(STATUS "hanlu3 ${CMAKE_CURRENT_SOURCE_DIR}../../../../../../utils_native/base/include")
add_library(document_ut SHARED ${DOCUMENT_SRC})
set(links mock kvdb adapter data_share secure)
target_link_libraries(document_ut ${links})



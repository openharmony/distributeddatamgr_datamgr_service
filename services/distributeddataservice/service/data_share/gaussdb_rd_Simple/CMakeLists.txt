cmake_minimum_required(VERSION 3.2)

project(grd_simple VERSION 1.0.0)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -std=c++17 -pthread")

# Address Sanitizer
option(USE_ASAN "Compile with address sanitiser" OFF)
if (USE_ASAN)
    message(STATUS "Compile with address sanitiser ${USE_ASAN}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer -O0")
endif()

function(download_repo repo_name repo_url)
    execute_process(
        COMMAND [ -d ${repo_name} ]
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/third_party
        RESULT_VARIABLE REPO_EXISTS
    )
    message(STATUS "check ${repo_name} exists result: ${REPO_EXISTS}")
    if (NOT (${REPO_EXISTS} EQUAL 0))
        message(STATUS "Downloading ${repo_name} from ${repo_url}")
        execute_process(
            COMMAND git clone ${repo_url}
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/third_party
        )
        message(STATUS "Download ${repo_name} finished")
    endif()
endfunction(download_repo)

function(build_repo repo_name script_path)
    execute_process(
        COMMAND [ -f ${repo_name}/Makefile ]
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/third_party
        RESULT_VARIABLE SCRIPT_EXISTS
    )
    if (NOT (${SCRIPT_EXISTS} EQUAL 0))
        message(STATUS "Copy build script ${script_path}/Makefile to repo ${repo_name}")
        execute_process(
            COMMAND cp third_party/kate/${script_path}/Makefile third_party/${repo_name} -f
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        )
        message(STATUS "Build ${repo_name}")
        execute_process(
            COMMAND make -j9
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/third_party/${repo_name}
        )
        message(STATUS "Build finished")
    endif()
endfunction(build_repo)

function(cmake_repo repo_name)
    execute_process(
        COMMAND [ -d ${repo_name}/build ]
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/third_party
        RESULT_VARIABLE BUILD_FINISHED
    )
    if (NOT (${BUILD_FINISHED} EQUAL 0))
        message(STATUS "compile repo ${repo_name}")
        execute_process(
            COMMAND mkdir -p build
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/third_party/${repo_name}
        )
        execute_process(
            COMMAND cmake ..
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/third_party/${repo_name}/build
        )
        execute_process(
            COMMAND make -j9
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/third_party/${repo_name}/build
        )
    endif()
endfunction(cmake_repo)

function(build_openssl repo_name)
    execute_process(
        COMMAND [ -f Makefile ]
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/third_party/${repo_name}
        RESULT_VARIABLE BUILD_FINISHED
    )
    if (NOT (${BUILD_FINISHED} EQUAL 0))
        message(STATUS "compile repo ${repo_name}")
        execute_process(
            COMMAND chmod +x ./config
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/third_party/${repo_name}
        )
        execute_process(
            COMMAND ./config
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/third_party/${repo_name}
        )
        execute_process(
            COMMAND make -j9
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/third_party/${repo_name}
        )
    endif()
endfunction(build_openssl)

# create third_party directory
execute_process(
    COMMAND mkdir -p ${PROJECT_SOURCE_DIR}/third_party
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

# Download dependencies
download_repo("third_party_cJSON" "https://gitee.com/openharmony/third_party_cJSON.git")
download_repo("third_party_googletest" "https://gitee.com/openharmony/third_party_googletest.git")
download_repo("third_party_openssl" "https://gitee.com/openharmony/third_party_openssl.git")
download_repo("third_party_sqlite" "https://gitee.com/openharmony/third_party_sqlite.git")
download_repo("utils_native" "https://gitee.com/openharmony/utils_native.git")
download_repo("kate" "https://gitee.com/lianhuix/kate.git")

# build openssl Makefile
build_openssl("third_party_openssl")

# build securec
build_repo("utils_native" "securec")

# build sqlite
build_repo("third_party_sqlite" "sqlite")

#build googletest
execute_process(
    COMMAND sed -i /Wshadow/d internal_utils.cmake
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/third_party/third_party_googletest/googletest/cmake
)
execute_process(
    COMMAND sed -i "s%testing::GTEST_FLAG(output)%// testing::GTEST_FLAG(output)%g" googlemock/src/gmock_main.cc
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/third_party/third_party_googletest
)
execute_process(
    COMMAND sed -i "s%testing::GTEST_FLAG(output)%// testing::GTEST_FLAG(output)%g" googletest/src/gtest_main.cc
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/third_party/third_party_googletest
)
cmake_repo("third_party_googletest")

# build cJSON
cmake_repo("third_party_cJSON" "cJSON")

find_package(Threads)
enable_testing()

set(DB_PATH ${PROJECT_SOURCE_DIR}/frameworks/libs/distributeddb)

file(GLOB_RECURSE DOCUMENT_SRC "${PROJECT_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE DOCUMENT_TEST "${PROJECT_SOURCE_DIR}/test/*.cpp")

include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src/storage
    ${PROJECT_SOURCE_DIR}/src/storage/include
    ${PROJECT_SOURCE_DIR}/src/storage/src
    ${PROJECT_SOURCE_DIR}/src/common
    ${PROJECT_SOURCE_DIR}/src/common/include
    ${PROJECT_SOURCE_DIR}/src/executor/include
    ${PROJECT_SOURCE_DIR}/src/executor/base
    ${PROJECT_SOURCE_DIR}/src/executor/document
    ${PROJECT_SOURCE_DIR}/src/oh_adapter
    ${PROJECT_SOURCE_DIR}/src/oh_adapter/include
    ${PROJECT_SOURCE_DIR}/src/oh_adapter/src
    ${PROJECT_SOURCE_DIR}/src/interface
    ${PROJECT_SOURCE_DIR}/src/interface/include
    ${PROJECT_SOURCE_DIR}/src/interface/src
)

set(DISTRIBUTEDDB_PATH ${PROJECT_SOURCE_DIR}/third_party/distributeddatamgr_kv_store/frameworks/libs/distributeddb/)
include_directories(
    ${DISTRIBUTEDDB_PATH}/include
    ${DISTRIBUTEDDB_PATH}/interfaces/include
)

include_directories(
    ${PROJECT_SOURCE_DIR}/third_party/third_party_googletest/googletest/include
    ${PROJECT_SOURCE_DIR}/third_party/third_party_googletest/googlemock/include
    ${PROJECT_SOURCE_DIR}/third_party/third_party_cJSON
    ${PROJECT_SOURCE_DIR}/third_party/third_party_sqlite/include
    ${PROJECT_SOURCE_DIR}/third_party/utils_native/base/include
    ${PROJECT_SOURCE_DIR}/third_party/utils_native/base/src/securec
    ${PROJECT_SOURCE_DIR}/third_party/third_party_openssl/include
    ${PROJECT_SOURCE_DIR}/third_party/kate/log/include
)

link_directories(
    ${PROJECT_SOURCE_DIR}/third_party/third_party_googletest/build/lib
    ${PROJECT_SOURCE_DIR}/third_party/third_party_cJSON/build
    ${PROJECT_SOURCE_DIR}/third_party/third_party_openssl
    ${PROJECT_SOURCE_DIR}/third_party/third_party_sqlite
    ${PROJECT_SOURCE_DIR}/third_party/utils_native
)

add_executable(document_ut ${DOCUMENT_SRC} ${DOCUMENT_TEST})

target_link_libraries(document_ut
    cjson

    sqlite3
    securec
    ssl
    crypto

    gtest
    gtest_main
    gmock
    gmock_main
)


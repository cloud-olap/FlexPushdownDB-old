cmake_minimum_required(VERSION 3.15)
project("fpdb" VERSION "${PROJECT_VERSION}" LANGUAGES C CXX)

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_LIST_DIR}/cmake")

include(ProcessorCount)
processorcount(NPROC)

include(CMakeUtils)

setDefaults()

set(FPDB_LLVM_VERSIONS "13.0" "12.0" "11.1" "11.0" "10" CACHE STRING "")
include(Configure_LLVM)


#-----------------------------------------------------------------------------------------------------------------------
# Set openssl path for mac os
#-----------------------------------------------------------------------------------------------------------------------

if (${APPLE})
  set(OPENSSL_ROOT_DIR /usr/local/opt/openssl)
  # This is a bug in CMake that causes it to prefer the system version over
  # the one in the specified ROOT folder.
  set(OPENSSL_CRYPTO_LIBRARY ${OPENSSL_ROOT_DIR}/lib/libcrypto.dylib CACHE FILEPATH "" FORCE)
  set(OPENSSL_SSL_LIBRARY ${OPENSSL_ROOT_DIR}/lib/libssl.dylib CACHE FILEPATH "" FORCE)
endif()


#-----------------------------------------------------------------------------------------------------------------------
# Dependencies
#-----------------------------------------------------------------------------------------------------------------------

set(DEPS_PREFIX "_deps")

include(External_spdlog)
include(External_doctest)
include(External_nanobench)
include(External_caf)
include(External_arrow)
include(External_AWSCPPSDK)
include(External_ANTLR)
include(External_graphviz)
include(External_expected)
include(External_fmt)
include(External_ssb_dbgen)
include(External_primesieve)
include(Dependency_SQLite3)
include(External_Backward)
include(External_CallableTraits)
include(External_zlib_ng)
include(External_boost)
include(External_thrift)
include(External_nlohmannjson)

#-----------------------------------------------------------------------------------------------------------------------
# Modules
#-----------------------------------------------------------------------------------------------------------------------

add_subdirectory(fpdb-util)
add_subdirectory(fpdb-caf)
add_subdirectory(fpdb-tuple)
add_subdirectory(fpdb-aws)
add_subdirectory(fpdb-catalogue)
add_subdirectory(fpdb-expression)
add_subdirectory(fpdb-expression-gandiva)
add_subdirectory(fpdb-calcite/cpp)
add_subdirectory(fpdb-plan)
add_subdirectory(fpdb-cache)
add_subdirectory(fpdb-executor)
add_subdirectory(fpdb-main)
add_subdirectory(fpdb-store-client)
add_subdirectory(fpdb-store-server)


#-----------------------------------------------------------------------------------------------------------------------
# Resources
#-----------------------------------------------------------------------------------------------------------------------

add_custom_target(resources-config
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/resources/config
        ${CMAKE_CURRENT_BINARY_DIR}/resources/config)

add_custom_target(resources-metadata
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/resources/metadata
        ${CMAKE_CURRENT_BINARY_DIR}/resources/metadata)

add_custom_target(resources-query
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/resources/query
        ${CMAKE_CURRENT_BINARY_DIR}/resources/query)

add_custom_target(resources DEPENDS resources-config resources-metadata resources-query)

project(fpdb-main VERSION "${PROJECT_VERSION}" LANGUAGES C CXX)


#-----------------------------------------------------------------------------------------------------------------------
# Targets
#-----------------------------------------------------------------------------------------------------------------------

# fpdb-main
add_library(fpdb-main
        src/ExecConfig.cpp include/fpdb/main/ExecConfig.h
        include/fpdb/main/ActorSystemConfig.h
        src/Client.cpp include/fpdb/main/Client.h
        src/Server.cpp include/fpdb/main/Server.h)

target_include_directories(fpdb-main PUBLIC include)

target_link_libraries(fpdb-main PUBLIC fpdb-executor)
target_link_libraries(fpdb-main PUBLIC fpdb-util)


# fpdb-main-client
add_executable(fpdb-main-client
        src/ClientMain.cpp)

target_link_libraries(fpdb-main-client PRIVATE fpdb-main)


# fpdb-main-server
add_executable(fpdb-main-server
        src/ServerMain.cpp)

target_link_libraries(fpdb-main-server PRIVATE fpdb-main)

add_dependencies(fpdb-main-server resources)


#-----------------------------------------------------------------------------------------------------------------------
# Test
#-----------------------------------------------------------------------------------------------------------------------

add_executable(fpdb-main-test
        test-base/Globals.h
        test-base/MainTest.cpp
        test-base/TestUtil.cpp test-base/TestUtil.h
        test-base/BitmapPushdownTestUtil.cpp test-base/BitmapPushdownTestUtil.h
        test-base/AdaptPushdownTestUtil.cpp test-base/AdaptPushdownTestUtil.h
        test/SSBTest.cpp
        test/SSBFPDBStoreSameNodeTest.cpp
        test/TPCHTest.cpp
        test/TPCHDistTest.cpp
        test/TPCHFPDBStoreSameNodeTest.cpp
        test/CachingOnlyTest.cpp
        test/HybridTest.cpp
        test/BitmapPushdownTest.cpp)

target_include_directories(fpdb-main-test PRIVATE test-base)

target_link_libraries(fpdb-main-test PRIVATE fpdb-main)
target_link_libraries(fpdb-main-test PRIVATE fpdb-store-server-static)
target_link_libraries(fpdb-main-test PRIVATE doctest::doctest)

add_dependencies(fpdb-main-test resources)

add_custom_command(
        TARGET fpdb-main-test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/test-resources
        ${CMAKE_CURRENT_BINARY_DIR}/test-resources)


#-----------------------------------------------------------------------------------------------------------------------
# Bench
#-----------------------------------------------------------------------------------------------------------------------

add_executable(fpdb-main-bench
        test-base/Globals.h
        test-base/MainTest.cpp
        test-base/TestUtil.cpp test-base/TestUtil.h
        test-base/BitmapPushdownTestUtil.cpp test-base/BitmapPushdownTestUtil.h
        test-base/AdaptPushdownTestUtil.cpp test-base/AdaptPushdownTestUtil.h
        bench/SSBDistFPDBStoreBench.cpp
        bench/TPCHBench.cpp
        bench/TPCHDistBench.cpp
        bench/TPCHFPDBStoreDiffNodeBench.cpp
        bench/TPCHDistFPDBStoreBench.cpp
        bench/BytesReadBench.cpp
        bench/BitmapPushdownBench.cpp)

target_include_directories(fpdb-main-bench PRIVATE test-base)

target_link_libraries(fpdb-main-bench PRIVATE fpdb-main)
target_link_libraries(fpdb-main-bench PRIVATE fpdb-store-server-static)
target_link_libraries(fpdb-main-bench PRIVATE doctest::doctest)

add_dependencies(fpdb-main-bench resources)

#-----------------------------------------------------------------------------------------------------------------------
# Hybrid Parquet Tests
#-----------------------------------------------------------------------------------------------------------------------

add_executable(hybrid_parquet_tests
        test-base/TestUtil.cpp test-base/TestUtil.h
        HybridParquetTests.cpp)

target_include_directories(hybrid_parquet_tests PRIVATE test-base)

target_link_libraries(hybrid_parquet_tests PRIVATE fpdb-main)
target_link_libraries(hybrid_parquet_tests PRIVATE fpdb-store-server-static)

add_dependencies(hybrid_parquet_tests resources)

#-----------------------------------------------------------------------------------------------------------------------
# File service for fpdb-server-store
#-----------------------------------------------------------------------------------------------------------------------

set(PROTO_DIR ${CMAKE_CURRENT_SOURCE_DIR}/proto)
set(FILE_SERVICE_PROTO ${PROTO_DIR}/FileService.proto)
set(FILE_SERVICE_GEN_CPP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/gen-cpp)

add_custom_command(
        DEPENDS ${FILE_SERVICE_PROTO}
        OUTPUT
        ${FILE_SERVICE_GEN_CPP_DIR}/FileService.pb.cc
        ${FILE_SERVICE_GEN_CPP_DIR}/FileService.pb.h
        ${FILE_SERVICE_GEN_CPP_DIR}/FileService.grpc.pb.cc
        ${FILE_SERVICE_GEN_CPP_DIR}/FileService.grpc.pb.h
        COMMAND mkdir -p ${FILE_SERVICE_GEN_CPP_DIR}
        COMMAND ${ARROW_PROTOC}
        -I ${PROTO_DIR}
        --cpp_out=${FILE_SERVICE_GEN_CPP_DIR}
        --grpc_out=${FILE_SERVICE_GEN_CPP_DIR}
        --plugin=protoc-gen-grpc=${ARROW_GRPC_CPP_PLUGIN}
        ${FILE_SERVICE_PROTO})

add_library(fpdb-store-server-file_gen STATIC
        ${FILE_SERVICE_GEN_CPP_DIR}/FileService.pb.cc
        ${FILE_SERVICE_GEN_CPP_DIR}/FileService.pb.h
        ${FILE_SERVICE_GEN_CPP_DIR}/FileService.grpc.pb.cc
        ${FILE_SERVICE_GEN_CPP_DIR}/FileService.grpc.pb.h)

target_link_libraries(fpdb-store-server-file_gen PUBLIC grpc_static)


add_library(fpdb-store-server-file STATIC
        src/file/FileServiceHandler.cpp include/fpdb/store/server/file/FileServiceHandler.hpp
        src/file/ArrowRemoteFileInputStream.cpp include/fpdb/store/server/file/ArrowRemoteFileInputStream.h
        src/file/RemoteFileReader.cpp include/fpdb/store/server/file/RemoteFileReader.h
        src/file/RemoteFileReaderBuilder.cpp include/fpdb/store/server/file/RemoteFileReaderBuilder.h
        src/file/RemoteCSVReader.cpp include/fpdb/store/server/file/RemoteCSVReader.h
        src/file/RemoteParquetReader.cpp include/fpdb/store/server/file/RemoteParquetReader.h)

target_include_directories(fpdb-store-server-file PUBLIC include)
target_include_directories(fpdb-store-server-file PUBLIC gen-cpp)

target_link_libraries(fpdb-store-server-file fpdb-store-server-file_gen)
target_link_libraries(fpdb-store-server-file fpdb-tuple)
target_link_libraries(fpdb-store-server-file fmt::fmt)


#-----------------------------------------------------------------------------------------------------------------------
# fpdb-store-server Library
#-----------------------------------------------------------------------------------------------------------------------

add_library(fpdb-store-server-flight-api STATIC
        src/flight/Util.cpp include/fpdb/store/server/flight/Util.hpp
        src/flight/CmdType.cpp include/fpdb/store/server/flight/CmdType.hpp
        src/flight/CmdObject.cpp include/fpdb/store/server/flight/CmdObject.hpp
        src/flight/SelectObjectContentCmd.cpp include/fpdb/store/server/flight/SelectObjectContentCmd.hpp
        include/fpdb/store/server/flight/BitmapType.h
        src/flight/PutBitmapCmd.cpp include/fpdb/store/server/flight/PutBitmapCmd.hpp
        src/flight/ClearBitmapCmd.cpp include/fpdb/store/server/flight/ClearBitmapCmd.hpp
        src/flight/ClearTableCmd.cpp include/fpdb/store/server/flight/ClearTableCmd.hpp
        src/flight/TicketType.cpp include/fpdb/store/server/flight/TicketType.hpp
        src/flight/TicketObject.cpp include/fpdb/store/server/flight/TicketObject.hpp
        src/flight/GetObjectTicket.cpp include/fpdb/store/server/flight/GetObjectTicket.hpp
        src/flight/SelectObjectContentTicket.cpp include/fpdb/store/server/flight/SelectObjectContentTicket.hpp
        src/flight/GetBitmapTicket.cpp include/fpdb/store/server/flight/GetBitmapTicket.hpp
        src/flight/GetTableTicket.cpp include/fpdb/store/server/flight/GetTableTicket.hpp
        src/flight/GetBatchLoadInfoTicket.cpp include/fpdb/store/server/flight/GetBatchLoadInfoTicket.hpp
        src/flight/adaptive/PutAdaptPushdownMetricsCmd.cpp include/fpdb/store/server/flight/adaptive/PutAdaptPushdownMetricsCmd.hpp
        src/flight/adaptive/ClearAdaptPushdownMetricsCmd.cpp include/fpdb/store/server/flight/adaptive/ClearAdaptPushdownMetricsCmd.hpp
        src/flight/adaptive/SetAdaptPushdownCmd.cpp include/fpdb/store/server/flight/adaptive/SetAdaptPushdownCmd.hpp
        src/flight/adaptive/PushbackCompleteCmd.cpp include/fpdb/store/server/flight/adaptive/PushbackCompleteCmd.hpp
        src/flight/adaptive/PushbackDoubleExecCmd.cpp include/fpdb/store/server/flight/adaptive/PushbackDoubleExecCmd.hpp
        src/flight/adaptive/SetNumReqToTailCmd.cpp include/fpdb/store/server/flight/adaptive/SetNumReqToTailCmd.hpp
        include/fpdb/store/server/flight/adaptive/ReqExtraInfo.hpp)

target_include_directories(fpdb-store-server-flight-api PUBLIC include)

target_link_libraries(fpdb-store-server-flight-api PUBLIC arrow_flight_static)
target_link_libraries(fpdb-store-server-flight-api PRIVATE expected)
target_link_libraries(fpdb-store-server-flight-api PRIVATE nlohmann_json)
target_link_libraries(fpdb-store-server-flight-api PRIVATE fmt::fmt)

add_library(fpdb-store-server-objlib OBJECT
        include/fpdb/store/server/Global.hpp
        src/flight/Util.cpp include/fpdb/store/server/flight/Util.hpp
        src/Module.cpp include/fpdb/store/server/Module.hpp
        src/Server.cpp include/fpdb/store/server/Server.hpp
        src/ServerConfig.cpp include/fpdb/store/server/ServerConfig.hpp
        src/FPDBStoreServerConfig.cpp include/fpdb/store/server/FPDBStoreServerConfig.hpp
        src/SignalHandler.cpp include/fpdb/store/server/SignalHandler.hpp
        src/caf/ActorManager.cpp include/fpdb/store/server/caf/ActorManager.hpp
        src/caf/ServerMeta.cpp include/fpdb/store/server/caf/ServerMeta.hpp
        src/caf/AbstractActor.cpp include/fpdb/store/server/caf/AbstractActor.hpp
        src/cluster/ClusterActor.cpp include/fpdb/store/server/cluster/ClusterActor.hpp
        src/cluster/NodeActor.cpp include/fpdb/store/server/cluster/NodeActor.hpp
        src/flight/FlightHandler.cpp include/fpdb/store/server/flight/FlightHandler.hpp
        src/flight/HeaderMiddleware.cpp include/fpdb/store/server/flight/HeaderMiddleware.hpp
        src/flight/HeaderMiddlewareFactory.cpp include/fpdb/store/server/flight/HeaderMiddlewareFactory.hpp
        src/flight/BitmapCache.cpp include/fpdb/store/server/flight/BitmapCache.hpp
        src/flight/BloomFilterCache.cpp include/fpdb/store/server/flight/BloomFilterCache.hpp
        include/fpdb/store/server/flight/adaptive/AdaptPushdownManagerVersion.hpp
        src/flight/adaptive/AdaptPushdownManager.cpp include/fpdb/store/server/flight/adaptive/AdaptPushdownManager.hpp
        src/flight/adaptive/AdaptPushdownManager2.cpp include/fpdb/store/server/flight/adaptive/AdaptPushdownManager2.hpp
        src/flight/adaptive/AdaptPushdownManager2Pa.cpp include/fpdb/store/server/flight/adaptive/AdaptPushdownManager2Pa.hpp
        src/flight/adaptive/AdaptPushdownManager3.cpp include/fpdb/store/server/flight/adaptive/AdaptPushdownManager3.hpp
        src/flight/adaptive/AdaptPushdownManager3Pa.cpp include/fpdb/store/server/flight/adaptive/AdaptPushdownManager3Pa.hpp)

set_target_properties(fpdb-store-server-objlib PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(fpdb-store-server-objlib PROPERTIES OUTPUT_NAME fpdb-store-server)

list(APPEND _PUBLIC_LINK_LIBRARIES expected)
list(APPEND _PUBLIC_LINK_LIBRARIES fpdb-store-server-file)
list(APPEND _PUBLIC_LINK_LIBRARIES fpdb-store-server-flight-api)
list(APPEND _PRIVATE_LINK_LIBRARIES fpdb-executor)
list(APPEND _PUBLIC_LINK_LIBRARIES nlohmann_json)
list(APPEND _PUBLIC_LINK_LIBRARIES caf::libcaf_core_shared)
list(APPEND _PUBLIC_LINK_LIBRARIES caf::libcaf_io_shared)
list(APPEND _PUBLIC_LINK_LIBRARIES caf::libcaf_openssl_shared)
list(APPEND _PRIVATE_LINK_LIBRARIES fmt::fmt)

list(APPEND _PUBLIC_SHARED_LINK_LIBRARIES arrow_flight_shared)
list(APPEND _PUBLIC_SHARED_LINK_LIBRARIES fpdb-store-client-shared)

list(APPEND _PUBLIC_STATIC_LINK_LIBRARIES arrow_flight_static)
list(APPEND _PUBLIC_STATIC_LINK_LIBRARIES fpdb-store-client-static)

target_include_directories(fpdb-store-server-objlib PUBLIC include)

target_link_libraries(fpdb-store-server-objlib ${_PUBLIC_LINK_LIBRARIES})
target_link_libraries(fpdb-store-server-objlib ${_PUBLIC_SHARED_LINK_LIBRARIES})
target_link_libraries(fpdb-store-server-objlib ${_PRIVATE_LINK_LIBRARIES})

add_library(fpdb-store-server-static STATIC $<TARGET_OBJECTS:fpdb-store-server-objlib>)
set_target_properties(fpdb-store-server-static PROPERTIES OUTPUT_NAME $<TARGET_PROPERTY:fpdb-store-server-objlib,OUTPUT_NAME>)
target_include_directories(fpdb-store-server-static PUBLIC $<TARGET_PROPERTY:fpdb-store-server-objlib,INCLUDE_DIRECTORIES>)
target_link_libraries(fpdb-store-server-static PUBLIC ${_PUBLIC_LINK_LIBRARIES})
target_link_libraries(fpdb-store-server-static PRIVATE ${_PUBLIC_STATIC_LINK_LIBRARIES})
target_link_libraries(fpdb-store-server-static PRIVATE ${_PRIVATE_LINK_LIBRARIES})

add_library(fpdb-store-server-shared SHARED $<TARGET_OBJECTS:fpdb-store-server-objlib>)
set_target_properties(fpdb-store-server-shared PROPERTIES OUTPUT_NAME $<TARGET_PROPERTY:fpdb-store-server-objlib,OUTPUT_NAME>)
target_include_directories(fpdb-store-server-shared PUBLIC $<TARGET_PROPERTY:fpdb-store-server-objlib,INCLUDE_DIRECTORIES>)
target_link_libraries(fpdb-store-server-shared PUBLIC ${_PUBLIC_LINK_LIBRARIES})
target_link_libraries(fpdb-store-server-shared PRIVATE ${_PUBLIC_SHARED_LINK_LIBRARIES})
target_link_libraries(fpdb-store-server-shared PRIVATE ${_PRIVATE_LINK_LIBRARIES})


#-----------------------------------------------------------------------------------------------------------------------
# Executable
#-----------------------------------------------------------------------------------------------------------------------

add_executable(fpdb-store-server-executable
        executable/Main.cpp executable/Global.hpp)

target_link_libraries(fpdb-store-server-executable Backward::Backward)
target_link_libraries(fpdb-store-server-executable fpdb-store-server-static)

add_dependencies(fpdb-store-server-executable resources)


#-----------------------------------------------------------------------------------------------------------------------
# Tests
#-----------------------------------------------------------------------------------------------------------------------

add_executable(fpdb-store-server-test
        test/Global.cpp test/Global.hpp
        test/Main.cpp
        test/FileServiceTest.cpp
        test/FlightBasicTest.cpp
        test/FlightSelectTest.cpp
        test/DistributedTest.cpp
        )

find_package(LibBFD REQUIRED)

target_link_libraries(fpdb-store-server-test fmt::fmt)
target_link_libraries(fpdb-store-server-test spdlog::spdlog)
target_link_libraries(fpdb-store-server-test doctest::doctest)
target_link_libraries(fpdb-store-server-test LibBFD::LibBFD)
target_link_libraries(fpdb-store-server-test Backward::Backward)
target_link_libraries(fpdb-store-server-test arrow_flight_static)
target_link_libraries(fpdb-store-server-test fpdb-store-server-static)
target_link_libraries(fpdb-store-server-test fpdb-file-read-test-util)

add_custom_command(
        TARGET fpdb-store-server-test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/test-resources
        ${CMAKE_CURRENT_BINARY_DIR}/test-resources)

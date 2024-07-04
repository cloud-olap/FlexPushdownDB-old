# AWS SDK
set(AWS_CPP_SDK_VERSION "1.8.133")
set(AWS_CPP_SDK_GIT_URL "https://github.com/aws/aws-sdk-cpp.git")


include(ExternalProject)
find_package(Git REQUIRED)


set(AWSCPPSDK_BASE aws-cpp-sdk_ep)
set(AWSCPPSDK_PREFIX ${DEPS_PREFIX}/${AWSCPPSDK_BASE})
set(AWSCPPSDK_BASE_DIR ${CMAKE_CURRENT_BINARY_DIR}/${AWSCPPSDK_PREFIX})
set(AWSCPPSDK_INSTALL_DIR ${AWSCPPSDK_BASE_DIR}/install)
set(AWSCPPSDK_INCLUDE_DIR ${AWSCPPSDK_INSTALL_DIR}/include)
set(AWSCPPSDK_LIB_DIR ${AWSCPPSDK_INSTALL_DIR}/lib)
set(AWSCPPSDK_CORE_SHARED_LIBS ${AWSCPPSDK_LIB_DIR}/${CMAKE_SHARED_LIBRARY_PREFIX}aws-cpp-sdk-core${CMAKE_SHARED_LIBRARY_SUFFIX})
set(AWSCPPSDK_CORE_STATIC_LIBS ${AWSCPPSDK_LIB_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}aws-cpp-sdk-core${CMAKE_STATIC_LIBRARY_SUFFIX})
set(AWSCPPSDK_S3_SHARED_LIBS ${AWSCPPSDK_LIB_DIR}/${CMAKE_SHARED_LIBRARY_PREFIX}aws-cpp-sdk-s3${CMAKE_SHARED_LIBRARY_SUFFIX})
set(AWSCPPSDK_S3_STATIC_LIBS ${AWSCPPSDK_LIB_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}aws-cpp-sdk-s3${CMAKE_STATIC_LIBRARY_SUFFIX})


ExternalProject_Add(${AWSCPPSDK_BASE}
        PREFIX ${AWSCPPSDK_BASE_DIR}
        GIT_REPOSITORY ${AWS_CPP_SDK_GIT_URL}
        GIT_TAG ${AWS_CPP_SDK_VERSION}
        GIT_PROGRESS ON
        GIT_SHALLOW ON
        UPDATE_DISCONNECTED TRUE
        INSTALL_DIR ${AWSCPPSDK_INSTALL_DIR}
        BUILD_BYPRODUCTS ${AWSCPPSDK_CORE_SHARED_LIBS} ${AWSCPPSDK_CORE_STATIC_LIBS} ${AWSCPPSDK_S3_SHARED_LIBS} ${AWSCPPSDK_S3_STATIC_LIBS}
        CMAKE_ARGS
        -DBUILD_ONLY=s3
        -DCPP_STANDARD=17
        -DENABLE_TESTING=OFF
        -DENABLE_UNITY_BUILD=OFF # Speeds up incremental builds
        -DCMAKE_INSTALL_MESSAGE=NEVER
        -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
        -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
        -DCMAKE_C_FLAGS=${CMAKE_C_FLAGS}
        -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
        -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
        -DCMAKE_INSTALL_PREFIX=${AWSCPPSDK_INSTALL_DIR}
        )


file(MAKE_DIRECTORY ${AWSCPPSDK_INCLUDE_DIR}) # Include directory needs to exist to run configure step


add_library("aws-cpp-sdk-core" SHARED IMPORTED)
set_target_properties("aws-cpp-sdk-core" PROPERTIES IMPORTED_LOCATION ${AWSCPPSDK_CORE_SHARED_LIBS})
target_include_directories("aws-cpp-sdk-core" INTERFACE ${AWSCPPSDK_INCLUDE_DIR})
add_dependencies("aws-cpp-sdk-core" ${AWSCPPSDK_BASE})

#add_library("aws-cpp-sdk-core-static" STATIC IMPORTED)
#set_target_properties("aws-cpp-sdk-core-static" PROPERTIES IMPORTED_LOCATION ${AWSCPPSDK_CORE_STATIC_LIBS})
#target_include_directories("aws-cpp-sdk-core-static" INTERFACE ${AWSCPPSDK_INCLUDE_DIR})
#add_dependencies("aws-cpp-sdk-core-static" ${AWSCPPSDK_BASE})

add_library("aws-cpp-sdk-s3" SHARED IMPORTED)
set_target_properties("aws-cpp-sdk-s3" PROPERTIES IMPORTED_LOCATION ${AWSCPPSDK_S3_SHARED_LIBS})
target_include_directories("aws-cpp-sdk-s3" INTERFACE ${AWSCPPSDK_INCLUDE_DIR})
add_dependencies("aws-cpp-sdk-s3" ${AWSCPPSDK_BASE})

#add_library("aws-cpp-sdk-s3-static" STATIC IMPORTED)
#set_target_properties("aws-cpp-sdk-s3-static" PROPERTIES IMPORTED_LOCATION ${AWSCPPSDK_S3_STATIC_LIBS})
#target_include_directories("aws-cpp-sdk-s3-static" INTERFACE ${AWSCPPSDK_INCLUDE_DIR})
#add_dependencies("aws-cpp-sdk-s3-static" ${AWSCPPSDK_BASE})


#showTargetProps("aws-cpp-sdk-core")
#showTargetProps("aws-cpp-sdk-s3")

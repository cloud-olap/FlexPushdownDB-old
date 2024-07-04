# std::expected

set(EXPECTED_VERSION "v1.0.0")
set(EXPECTED_GIT_URL "https://github.com/TartanLlama/expected.git")

include(ExternalProject)
find_package(Git REQUIRED)


set(EXPECTED_BASE expected_ep)
set(EXPECTED_PREFIX ${DEPS_PREFIX}/${EXPECTED_BASE})
set(EXPECTED_BASE_DIR ${CMAKE_CURRENT_BINARY_DIR}/${EXPECTED_PREFIX})
set(EXPECTED_INSTALL_DIR ${EXPECTED_BASE_DIR}/install)
set(EXPECTED_INCLUDE_DIR ${EXPECTED_INSTALL_DIR}/include)


ExternalProject_Add(${EXPECTED_BASE}
        PREFIX ${EXPECTED_BASE_DIR}
        INSTALL_DIR ${EXPECTED_INSTALL_DIR}
        GIT_REPOSITORY ${EXPECTED_GIT_URL}
        GIT_TAG ${EXPECTED_VERSION}
        GIT_PROGRESS ON
        GIT_SHALLOW ON
        UPDATE_DISCONNECTED TRUE
        CMAKE_ARGS
        -DCMAKE_INSTALL_MESSAGE=NEVER
        -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
        -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
        -DCMAKE_INSTALL_PREFIX=${EXPECTED_INSTALL_DIR}
        )


file(MAKE_DIRECTORY ${EXPECTED_INCLUDE_DIR}) # Include directory needs to exist to run configure step


add_library(expected INTERFACE IMPORTED)
target_include_directories(expected INTERFACE ${EXPECTED_INCLUDE_DIR})
add_dependencies(expected ${EXPECTED_BASE})


#showTargetProps(expected)

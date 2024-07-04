# Doctest

set(DOCTEST_VERSION "1c8da00") # 1c8da00 = 2.4.0
set(DOCTEST_GIT_URL "https://github.com/onqtam/doctest.git")


include(ExternalProject)
find_package(Git REQUIRED)


set(DOCTEST_BASE doctest_ep)
set(DOCTEST_PREFIX ${DEPS_PREFIX}/${DOCTEST_BASE})
set(DOCTEST_BASE_DIR ${CMAKE_CURRENT_BINARY_DIR}/${DOCTEST_PREFIX})
set(DOCTEST_SOURCE_DIR ${DOCTEST_BASE_DIR}/src/${DOCTEST_BASE})
set(DOCTEST_INCLUDE_DIR ${DOCTEST_SOURCE_DIR})


ExternalProject_Add(${DOCTEST_BASE}
        PREFIX ${DOCTEST_PREFIX}
        GIT_REPOSITORY ${DOCTEST_GIT_URL}
        GIT_TAG ${DOCTEST_VERSION}
        GIT_PROGRESS ON
        GIT_SHALLOW ON
        UPDATE_DISCONNECTED TRUE
        CMAKE_ARGS
        -DCMAKE_C_COMPILER:STRING=${CMAKE_C_COMPILER}
        -DCMAKE_CXX_COMPILER:STRING=${CMAKE_CXX_COMPILER}
        -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND ""
        )


add_library(doctest::doctest INTERFACE IMPORTED)
target_include_directories(doctest::doctest INTERFACE ${DOCTEST_INCLUDE_DIR})
add_dependencies(doctest::doctest ${DOCTEST_BASE})

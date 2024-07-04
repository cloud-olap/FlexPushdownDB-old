#-----------------------------------------------------------------------------------------------------------------------
# Util
#-----------------------------------------------------------------------------------------------------------------------

add_library(fpdb-util
        include/fpdb/util/Traits.h
        include/fpdb/util/Color.h
        src/Util.cpp include/fpdb/util/Util.h
        src/CPUMonitor.cpp include/fpdb/util/CPUMonitor.h)

target_include_directories(fpdb-util PUBLIC include)

set_target_properties(fpdb-util PROPERTIES POSITION_INDEPENDENT_CODE ON)

#target_link_libraries(fpdb-util PUBLIC stdc++fs)
target_link_libraries(fpdb-util PUBLIC fmt::fmt)
target_link_libraries(fpdb-util PUBLIC spdlog::spdlog)
target_link_libraries(fpdb-util PUBLIC expected)
target_link_libraries(fpdb-util PUBLIC boost_callable_traits)


#-----------------------------------------------------------------------------------------------------------------------
# Test
#-----------------------------------------------------------------------------------------------------------------------

add_executable(fpdb-util-test
        test/Globals.h
        test/MainTest.cpp
        test/TraitsTest.cpp
        test/UtilTest.cpp)

target_link_libraries(fpdb-util-test fpdb-util)
target_link_libraries(fpdb-util-test fmt::fmt)
target_link_libraries(fpdb-util-test spdlog::spdlog)
target_link_libraries(fpdb-util-test doctest::doctest)


#-----------------------------------------------------------------------------------------------------------------------
# Diagnostics
#-----------------------------------------------------------------------------------------------------------------------

#showTargetProps(fpdb-util)

//
// Created by Yifei Yang on 11/30/21.
//

#ifndef FPDB_FPDB_MAIN_TEST_GLOBALS_H
#define FPDB_FPDB_MAIN_TEST_GLOBALS_H

#define LOG_LEVEL SPDLOG_LEVEL_DEBUG

#define SPDLOG_ACTIVE_LEVEL LOG_LEVEL
#include "spdlog/spdlog.h"
#include "thread"

namespace fpdb::main::test {

inline constexpr int PARALLEL_DIST_SF0_01 = 2;
inline constexpr int PARALLEL_DIST_SF10 = 17;    // set to 2^n may cause skewness
inline constexpr int PARALLEL_DIST_SF100 = 17;
inline constexpr int PARALLEL_FPDB_STORE_SAME_NODE = 3;
inline constexpr int PARALLEL_FPDB_STORE_DIFF_NODE = 17;

}

#endif //FPDB_FPDB_MAIN_TEST_GLOBALS_H

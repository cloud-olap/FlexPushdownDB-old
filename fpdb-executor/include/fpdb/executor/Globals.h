//
// Created by Yifei Yang on 11/17/22.
//

#ifndef FPDB_FPDB_EXECUTOR_INCLUDE_FPDB_EXECUTOR_GLOBALS_H
#define FPDB_FPDB_EXECUTOR_INCLUDE_FPDB_EXECUTOR_GLOBALS_H

#include <mutex>

namespace fpdb::executor {

// used for output of concurrent runs
inline std::mutex ConcurrentOutputMutex;

/**
 * Clear global states
 */
void clearGlobal();

}

#endif //FPDB_FPDB_EXECUTOR_INCLUDE_FPDB_EXECUTOR_GLOBALS_H

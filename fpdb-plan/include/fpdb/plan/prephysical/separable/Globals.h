//
// Created by Yifei Yang on 11/7/22.
//

#ifndef FPDB_FPDB_PLAN_INCLUDE_FPDB_PLAN_GLOBALS_H
#define FPDB_FPDB_PLAN_INCLUDE_FPDB_PLAN_GLOBALS_H

namespace fpdb::plan::prephysical::separable {

/**
 * Pushdown parameters used by FPDB store (others are set in fpdb-executor)
 * Set by "pushdown.conf"
 */
inline bool ENABLE_CO_LOCATED_JOIN_PUSHDOWN;

}

#endif //FPDB_FPDB_PLAN_INCLUDE_FPDB_PLAN_GLOBALS_H

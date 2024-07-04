//
// Created by Yifei Yang on 11/7/21.
//

#ifndef FPDB_FPDB_PLAN_INCLUDE_FPDB_PLAN_PREPHYSICAL_JOINTYPE_H
#define FPDB_FPDB_PLAN_INCLUDE_FPDB_PLAN_PREPHYSICAL_JOINTYPE_H

#include <tl/expected.hpp>
#include <fmt/format.h>

namespace fpdb::plan::prephysical {

enum JoinType {
  INNER,
  LEFT,
  RIGHT,
  FULL,
  SEMI
};

inline tl::expected<std::string, std::string> joinTypeToStr(JoinType joinType) {
  switch (joinType) {
    case INNER: return "Inner";
    case LEFT: return "Left";
    case RIGHT: return "Right";
    case FULL: return "Full";
    case SEMI: return "Semi";
    default: return tl::make_unexpected(fmt::format("Unknown join type: {}", joinType));
  }
}

inline tl::expected<JoinType, std::string> strToJoinType(const std::string &joinTypeStr) {
  if (joinTypeStr == "Inner") {
    return JoinType::INNER;
  } else if (joinTypeStr == "Left") {
    return JoinType::LEFT;
  } else if (joinTypeStr == "Right") {
    return JoinType::RIGHT;
  } else if (joinTypeStr == "Full") {
    return JoinType::FULL;
  } else if (joinTypeStr == "Semi") {
    return JoinType::SEMI;
  } else {
    return tl::make_unexpected(fmt::format("Unknown join type string: {}", joinTypeStr));
  }
}

}

#endif //FPDB_FPDB_PLAN_INCLUDE_FPDB_PLAN_PREPHYSICAL_JOINTYPE_H

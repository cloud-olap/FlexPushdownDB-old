//
// Created by Yifei Yang on 12/10/21.
//

#ifndef FPDB_FPDB_EXPRESSION_GANDIVA_INCLUDE_FPDB_EXPRESSION_GANDIVA_DATEINTERVALTYPE_H
#define FPDB_FPDB_EXPRESSION_GANDIVA_INCLUDE_FPDB_EXPRESSION_GANDIVA_DATEINTERVALTYPE_H

#include <tl/expected.hpp>

namespace fpdb::expression::gandiva {

enum DateIntervalType {
  DAY,
  MONTH,
  YEAR
};

std::string intervalTypeToString(DateIntervalType intervalType);
tl::expected<DateIntervalType, std::string> stringToIntervalType(const std::string &str);

}

#endif //FPDB_FPDB_EXPRESSION_GANDIVA_INCLUDE_FPDB_EXPRESSION_GANDIVA_DATEINTERVALTYPE_H

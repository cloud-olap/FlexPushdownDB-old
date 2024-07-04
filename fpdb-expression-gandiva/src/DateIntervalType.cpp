//
// Created by Yifei Yang on 2/22/22.
//

#include <fpdb/expression/gandiva/DateIntervalType.h>
#include <fmt/format.h>

namespace fpdb::expression::gandiva {

std::string intervalTypeToString(DateIntervalType intervalType) {
  switch (intervalType) {
    case DAY: return "Day";
    case MONTH: return "Month";
    case YEAR: return "Year";
    default:
      throw std::runtime_error(fmt::format("Unsupported date interval type: {}", intervalType));
  }
}

tl::expected<DateIntervalType, std::string> stringToIntervalType(const std::string &str) {
  if (str == "Day") {
    return DateIntervalType::DAY;
  } else if (str == "Month") {
    return DateIntervalType::MONTH;
  } else if (str == "Year") {
    return DateIntervalType::YEAR;
  } else {
    return tl::make_unexpected(fmt::format("Unsupported date interval type: '{}'", str));
  }
}

}

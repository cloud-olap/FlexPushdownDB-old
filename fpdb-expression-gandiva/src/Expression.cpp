//
// Created by matt on 27/4/20.
//

#include <fpdb/expression/gandiva/Expression.h>
#include <fpdb/expression/gandiva/Add.h>
#include <fpdb/expression/gandiva/And.h>
#include <fpdb/expression/gandiva/Cast.h>
#include <fpdb/expression/gandiva/Column.h>
#include <fpdb/expression/gandiva/DateAdd.h>
#include <fpdb/expression/gandiva/DateExtract.h>
#include <fpdb/expression/gandiva/Divide.h>
#include <fpdb/expression/gandiva/EqualTo.h>
#include <fpdb/expression/gandiva/GreaterThan.h>
#include <fpdb/expression/gandiva/GreaterThanOrEqualTo.h>
#include <fpdb/expression/gandiva/If.h>
#include <fpdb/expression/gandiva/In.h>
#include <fpdb/expression/gandiva/IsNull.h>
#include <fpdb/expression/gandiva/LessThan.h>
#include <fpdb/expression/gandiva/LessThanOrEqualTo.h>
#include <fpdb/expression/gandiva/Like.h>
#include <fpdb/expression/gandiva/Multiply.h>
#include <fpdb/expression/gandiva/Not.h>
#include <fpdb/expression/gandiva/NotEqualTo.h>
#include <fpdb/expression/gandiva/NumericLiteral.h>
#include <fpdb/expression/gandiva/Or.h>
#include <fpdb/expression/gandiva/StringLiteral.h>
#include <fpdb/expression/gandiva/Substr.h>
#include <fpdb/expression/gandiva/Subtract.h>
#include <fpdb/tuple/serialization/ArrowSerializer.h>
#include <fpdb/util/Util.h>
#include <fmt/format.h>

using namespace fpdb::expression::gandiva;
using namespace fpdb::util;

Expression::Expression(ExpressionType type) :
  type_(type) {}

ExpressionType Expression::getType() const {
  return type_;
}

const gandiva::NodePtr &Expression::getGandivaExpression() const {
  return gandivaExpression_;
}

std::string Expression::showString() {
  return gandivaExpression_->ToString();
}

tl::expected<std::shared_ptr<fpdb::expression::gandiva::Expression>, std::string>
Expression::fromJson(const nlohmann::json &jObj) {
  if (!jObj.contains("type")) {
    return tl::make_unexpected(fmt::format("Type not specified in file format JSON '{}'", to_string(jObj)));
  }
  auto type = jObj["type"].get<std::string>();

  if (type == "Add") {
    return Add::fromJson(jObj);
  } else if (type == "And") {
    return And::fromJson(jObj);
  } else if (type == "Cast") {
    return Cast::fromJson(jObj);
  } else if (type == "Column") {
    return Column::fromJson(jObj);
  } else if (type == "DateAdd") {
    return DateAdd::fromJson(jObj);
  } else if (type == "DateExtract") {
    return DateExtract::fromJson(jObj);
  } else if (type == "Divide") {
    return Divide::fromJson(jObj);
  } else if (type == "EqualTo") {
    return EqualTo::fromJson(jObj);
  } else if (type == "GreaterThan") {
    return GreaterThan::fromJson(jObj);
  } else if (type == "GreaterThanOrEqualTo") {
    return GreaterThanOrEqualTo::fromJson(jObj);
  } else if (type == "If") {
    return If::fromJson(jObj);
  } else if (type == "IsNull") {
    return IsNull::fromJson(jObj);
  } else if (type == "LessThan") {
    return LessThan::fromJson(jObj);
  } else if (type == "LessThanOrEqualTo") {
    return LessThanOrEqualTo::fromJson(jObj);
  } else if (type == "Like") {
    return Like::fromJson(jObj);
  } else if (type == "Multiply") {
    return Multiply::fromJson(jObj);
  } else if (type == "Not") {
    return Not::fromJson(jObj);
  } else if (type == "NotEqualTo") {
    return NotEqualTo::fromJson(jObj);
  } else if (type == "Or") {
    return Or::fromJson(jObj);
  } else if (type == "StringLiteral") {
    return StringLiteral::fromJson(jObj);
  } else if (type == "Substr") {
    return Substr::fromJson(jObj);
  } else if (type == "Subtract") {
    return Subtract::fromJson(jObj);
  }

  else if (type == "In") {
    if (!jObj.contains("dataType")) {
      return tl::make_unexpected(fmt::format("Data type not specified in In expression JSON '{}'", to_string(jObj)));
    }
    auto dataType = fpdb::tuple::ArrowSerializer::bytes_to_dataType(jObj["dataType"].get<std::vector<uint8_t>>());
    if (dataType->id() == ::arrow::int32()->id()) {
      return In<::arrow::Int32Type, int32_t>::fromJson(jObj);
    } else if (dataType->id() == ::arrow::int64()->id()) {
      return In<::arrow::Int64Type, int64_t>::fromJson(jObj);
    } else if (dataType->id() == ::arrow::float64()->id()) {
      return In<::arrow::DoubleType, double>::fromJson(jObj);
    } else if (dataType->id() == ::arrow::utf8()->id()) {
      return In<::arrow::StringType, std::string>::fromJson(jObj);
    } else if (dataType->id() == ::arrow::date64()->id()) {
      return In<::arrow::Date64Type, int64_t>::fromJson(jObj);
    } else {
      return tl::make_unexpected(fmt::format("Unsupported data type in In expression '{}'", dataType->name()));
    }
  }

  else if (type == "NumericLiteral") {
    if (!jObj.contains("dataType")) {
      return tl::make_unexpected(fmt::format("Data type not specified in NumericLiteral expression JSON '{}'", to_string(jObj)));
    }
    auto dataType = fpdb::tuple::ArrowSerializer::bytes_to_dataType(jObj["dataType"].get<std::vector<uint8_t>>());
    if (dataType->id() == ::arrow::int32()->id()) {
      return NumericLiteral<::arrow::Int32Type>::fromJson(jObj);
    } else if (dataType->id() == ::arrow::int64()->id()) {
      return NumericLiteral<::arrow::Int64Type>::fromJson(jObj);
    } else if (dataType->id() == ::arrow::float64()->id()) {
      return NumericLiteral<::arrow::DoubleType>::fromJson(jObj);
    } else if (dataType->id() == ::arrow::boolean()->id()) {
      return NumericLiteral<::arrow::BooleanType>::fromJson(jObj);
    } else if (dataType->id() == ::arrow::date64()->id()) {
      return NumericLiteral<::arrow::Date64Type>::fromJson(jObj);
    } else {
      return tl::make_unexpected(fmt::format("Unsupported data type in NumericLiteral expression '{}'", dataType->name()));
    }
  }

  else {
    return tl::make_unexpected(fmt::format("Unsupported expression type: '{}'", type));
  }
}

std::shared_ptr<std::string> fpdb::expression::gandiva::removePrefixInt(const std::string& str) {
  if (str.substr(0, prefixInt_.size()) == prefixInt_) {
    return std::make_shared<std::string>(str.substr(prefixInt_.size(), str.size() - prefixInt_.size()));
  } else {
    return nullptr;
  }
}

std::shared_ptr<std::string> fpdb::expression::gandiva::removePrefixFloat(const std::string& str) {
  if (str.substr(0, prefixFloat_.size()) == prefixFloat_) {
    return std::make_shared<std::string>(str.substr(prefixFloat_.size(), str.size() - prefixFloat_.size()));
  } else {
    return nullptr;
  }
}

std::shared_ptr<arrow::DataType> getNumericType(const std::shared_ptr<fpdb::expression::gandiva::Expression>& expr) {
  if (expr->getTypeString() == "NumericLiteral<Int32>") {
    return arrow::int32();
  } else if (expr->getTypeString() == "NumericLiteral<Int64>") {
    return arrow::int64();
  } else if (expr->getTypeString() == "NumericLiteral<Double>") {
    return arrow::float64();
  } else {
    return nullptr;
  }
}

std::shared_ptr<fpdb::expression::gandiva::Expression> 
fpdb::expression::gandiva::cascadeCast(std::shared_ptr<fpdb::expression::gandiva::Expression> expr) {
  if (expr->getType() == AND) {
    auto andExpr = std::static_pointer_cast<fpdb::expression::gandiva::And>(expr);
    vector<std::shared_ptr<Expression>> castChildExprs;
    for (const auto &childExpr: andExpr->getExprs()) {
      castChildExprs.emplace_back(cascadeCast(childExpr));
    }
    return and_(castChildExprs);
  }

  else if (expr->getType() == OR) {
    auto orExpr = std::static_pointer_cast<fpdb::expression::gandiva::Or>(expr);
    vector<std::shared_ptr<Expression>> castChildExprs;
    for (const auto &childExpr: orExpr->getExprs()) {
      castChildExprs.emplace_back(cascadeCast(childExpr));
    }
    return or_(castChildExprs);
  }

  else if (expr->getType() == EQUAL_TO) {
    auto eqExpr = std::static_pointer_cast<fpdb::expression::gandiva::EqualTo>(expr);
    auto leftExpr = eqExpr->getLeft();
    auto rightExpr = eqExpr->getRight();
    auto leftType = getNumericType(leftExpr);
    if (leftType) {
      return eq(leftExpr, fpdb::expression::gandiva::cast(rightExpr, leftType));
    } else {
      auto rightType = getNumericType(rightExpr);
      if (rightType) {
        return eq(fpdb::expression::gandiva::cast(leftExpr, rightType), rightExpr);
      }
    }
    return expr;
  }

  else if (expr->getType() == GREATER_THAN) {
    auto gtExpr = std::static_pointer_cast<fpdb::expression::gandiva::GreaterThan>(expr);
    auto leftExpr = gtExpr->getLeft();
    auto rightExpr = gtExpr->getRight();
    auto leftType = getNumericType(leftExpr);
    if (leftType) {
      return gt(leftExpr, fpdb::expression::gandiva::cast(rightExpr, leftType));
    } else {
      auto rightType = getNumericType(rightExpr);
      if (rightType) {
        return gt(fpdb::expression::gandiva::cast(leftExpr, rightType), rightExpr);
      }
    }
    return expr;
  }

  else if (expr->getType() == GREATER_THAN_OR_EQUAL_TO) {
    auto geExpr = std::static_pointer_cast<fpdb::expression::gandiva::GreaterThanOrEqualTo>(expr);
    auto leftExpr = geExpr->getLeft();
    auto rightExpr = geExpr->getRight();
    auto leftType = getNumericType(leftExpr);
    if (leftType) {
      return gte(leftExpr, fpdb::expression::gandiva::cast(rightExpr, leftType));
    } else {
      auto rightType = getNumericType(rightExpr);
      if (rightType) {
        return gte(fpdb::expression::gandiva::cast(leftExpr, rightType), rightExpr);
      }
    }
    return expr;
  }

  else if (expr->getType() == LESS_THAN) {
    auto ltExpr = std::static_pointer_cast<fpdb::expression::gandiva::LessThan>(expr);
    auto leftExpr = ltExpr->getLeft();
    auto rightExpr = ltExpr->getRight();
    auto leftType = getNumericType(leftExpr);
    if (leftType) {
      return lt(leftExpr, fpdb::expression::gandiva::cast(rightExpr, leftType));
    } else {
      auto rightType = getNumericType(rightExpr);
      if (rightType) {
        return lt(fpdb::expression::gandiva::cast(leftExpr, rightType), rightExpr);
      }
    }
    return expr;
  }

  else if (expr->getType() == LESS_THAN_OR_EQUAL_TO) {
    auto leExpr = std::static_pointer_cast<fpdb::expression::gandiva::LessThanOrEqualTo>(expr);
    auto leftExpr = leExpr->getLeft();
    auto rightExpr = leExpr->getRight();
    auto leftType = getNumericType(leftExpr);
    if (leftType) {
      return lte(leftExpr, fpdb::expression::gandiva::cast(rightExpr, leftType));
    } else {
      auto rightType = getNumericType(rightExpr);
      if (rightType) {
        return lte(fpdb::expression::gandiva::cast(leftExpr, rightType), rightExpr);
      }
    }
    return expr;
  }

  return expr;
}

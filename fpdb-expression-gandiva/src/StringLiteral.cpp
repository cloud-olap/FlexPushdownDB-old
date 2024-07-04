//
// Created by Yifei Yang on 7/15/20.
//

#include "fpdb/expression/gandiva/StringLiteral.h"
#include <gandiva/tree_expr_builder.h>
#include <fmt/format.h>
#include <utility>

using namespace fpdb::expression::gandiva;

StringLiteral::StringLiteral(std::optional<std::string> value) :
  Expression(STRING_LITERAL),
  value_(std::move(value)) {}

void StringLiteral::compile(const std::shared_ptr<arrow::Schema> &){
  ::gandiva::NodePtr literal;
  if (value_.has_value()) {
    literal = ::gandiva::TreeExprBuilder::MakeStringLiteral(*value_);
  } else {
    literal = ::gandiva::TreeExprBuilder::MakeNull(arrow::utf8());
  }

  gandivaExpression_ = literal;
  returnType_ = ::arrow::TypeTraits<::arrow::StringType>::type_singleton();
}

std::string StringLiteral::alias(){
  if (value_.has_value()) {
    return "\'" + *value_ + "\'";
  } else {
    return "";
  }
}

std::set<std::string> StringLiteral::involvedColumnNames() {
  return {};
}

const std::optional<std::string> &StringLiteral::value() const {
  return value_;
}

std::string StringLiteral::getTypeString() const {
  return "StringLiteral";
}

::nlohmann::json StringLiteral::toJson() const {
  ::nlohmann::json jObj;
  jObj.emplace("type", getTypeString());
  if (value_.has_value()) {
    jObj.emplace("value", *value_);
  }
  return jObj;
}

tl::expected<std::shared_ptr<StringLiteral>, std::string> StringLiteral::fromJson(const nlohmann::json &jObj) {
  std::optional<std::string> value = std::nullopt;
  if (jObj.contains("value")) {
    value = jObj["value"].get<std::string>();
  }
  return std::make_shared<StringLiteral>(value);
}

std::shared_ptr<Expression> fpdb::expression::gandiva::str_lit(const std::optional<std::string> &value){
  return std::make_shared<StringLiteral>(value);
}
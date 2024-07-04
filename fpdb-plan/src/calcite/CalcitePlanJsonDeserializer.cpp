//
// Created by Yifei Yang on 11/1/21.
//

#include <fpdb/plan/calcite/CalcitePlanJsonDeserializer.h>
#include <fpdb/plan/prephysical/JoinType.h>
#include <fpdb/plan/Util.h>
#include <fpdb/catalogue/obj-store/ObjStoreCatalogueEntry.h>
#include <fpdb/expression/gandiva/Expression.h>
#include <fpdb/expression/gandiva/Column.h>
#include <fpdb/expression/gandiva/And.h>
#include <fpdb/expression/gandiva/Or.h>
#include <fpdb/expression/gandiva/Not.h>
#include <fpdb/expression/gandiva/Add.h>
#include <fpdb/expression/gandiva/DateAdd.h>
#include <fpdb/expression/gandiva/Subtract.h>
#include <fpdb/expression/gandiva/Multiply.h>
#include <fpdb/expression/gandiva/Divide.h>
#include <fpdb/expression/gandiva/EqualTo.h>
#include <fpdb/expression/gandiva/NotEqualTo.h>
#include <fpdb/expression/gandiva/LessThan.h>
#include <fpdb/expression/gandiva/GreaterThan.h>
#include <fpdb/expression/gandiva/LessThanOrEqualTo.h>
#include <fpdb/expression/gandiva/GreaterThanOrEqualTo.h>
#include <fpdb/expression/gandiva/StringLiteral.h>
#include <fpdb/expression/gandiva/NumericLiteral.h>
#include <fpdb/expression/gandiva/In.h>
#include <fpdb/expression/gandiva/If.h>
#include <fpdb/expression/gandiva/Like.h>
#include <fpdb/expression/gandiva/DateExtract.h>
#include <fpdb/expression/gandiva/IsNull.h>
#include <fpdb/expression/gandiva/Substr.h>
#include <fpdb/expression/gandiva/Cast.h>
#include <fpdb/tuple/ColumnName.h>

#include <fmt/format.h>
#include <set>
#include <utility>

using namespace fpdb::plan;
using namespace fpdb::catalogue;
using namespace fpdb::tuple;

namespace fpdb::plan::calcite {

CalcitePlanJsonDeserializer::CalcitePlanJsonDeserializer(string planJsonString,
                                                         const shared_ptr<CatalogueEntry> &catalogueEntry):
  planJsonString_(std::move(planJsonString)),
  catalogueEntry_(catalogueEntry),
  pOpIdGenerator_(0) {}

shared_ptr<PrePhysicalPlan> CalcitePlanJsonDeserializer::deserialize(string planJsonString,
                                                                     const shared_ptr<CatalogueEntry> &catalogueEntry) {
  CalcitePlanJsonDeserializer deserializer(planJsonString, catalogueEntry);
  return deserializer.deserialize();
}

shared_ptr<PrePhysicalPlan> CalcitePlanJsonDeserializer::deserialize() {
  auto jObj = json::parse(planJsonString_);
  auto rootPrePOp = deserializeDfs(jObj["plan"]);
  vector<string> outputColumnNames;
  for (const auto &columnName: jObj["outputFields"].get<vector<string>>()) {
    outputColumnNames.emplace_back(ColumnName::canonicalize(columnName));
  }
  return make_shared<PrePhysicalPlan>(rootPrePOp, outputColumnNames);
}

shared_ptr<PrePhysicalOp> CalcitePlanJsonDeserializer::deserializeDfs(json &jObj) {
  string opName = jObj["operator"].get<string>();
  if (opName == "EnumerableSort") {
    return deserializeSort(jObj);
  } else if (opName == "EnumerableLimitSort") {
    return deserializeLimitSort(jObj);
  } else if (opName == "EnumerableAggregate" || opName == "EnumerableSortedAggregate") {
    return deserializeAggregateOrGroup(jObj);
  } else if (opName == "EnumerableProject") {
    return deserializeProject(jObj);
  } else if (opName == "EnumerableHashJoin") {
    return deserializeHashJoin(jObj);
  } else if (opName == "EnumerableNestedLoopJoin") {
    return deserializeNestedLoopJoin(jObj);
  } else if (opName == "EnumerableFilter") {
    return deserializeFilterOrFilterableScan(jObj);
  } else if (opName == "EnumerableTableScan") {
    return deserializeTableScan(jObj);
  } else {
    throw runtime_error(fmt::format("Unsupported PrePhysicalOp type, {}", opName));
  }
}

vector<shared_ptr<PrePhysicalOp>> CalcitePlanJsonDeserializer::deserializeProducers(const json &jObj) {
  vector<shared_ptr<PrePhysicalOp>> producers;
  auto producersJArr = jObj["inputs"].get<vector<json>>();
  for (auto &producerJObj: producersJArr) {
    const shared_ptr<PrePhysicalOp> &producer = deserializeDfs(producerJObj);
    producers.emplace_back(producer);
  }
  return producers;
}

shared_ptr<fpdb::expression::gandiva::Expression> CalcitePlanJsonDeserializer::deserializeInputRef(const json &jObj) {
  const auto &columnName = ColumnName::canonicalize(jObj["inputRef"].get<string>());
  return fpdb::expression::gandiva::col(columnName);
}

shared_ptr<fpdb::expression::gandiva::Expression> CalcitePlanJsonDeserializer::deserializeLiteral(const json &jObj) {
  const auto &literalJObj = jObj["literal"];
  const auto &type = literalJObj["type"].get<string>();
  const auto &valueJObj = literalJObj["value"];

  // check if the value is null
  bool isNull = valueJObj.is_null();

  if (type == "CHAR" || type == "VARCHAR") {
    optional<string> value = isNull ? nullopt : optional<string>(valueJObj.get<string>());
    return fpdb::expression::gandiva::str_lit(value);
  } else if (type == "INTEGER") {
    optional<int> value = isNull ? nullopt : optional<int>(valueJObj.get<int>());
    // gandiva only supports binary expressions with same left, right and return type,
    // so to avoid casting we make all integer fields as int64, as well as scalars.
    return fpdb::expression::gandiva::num_lit<arrow::Int64Type>(value);
  } else if (type == "BIGINT") {
    optional<long> value = isNull ? nullopt : optional<long>(valueJObj.get<long>());
    return fpdb::expression::gandiva::num_lit<arrow::Int64Type>(value);
  } else if (type == "DOUBLE" || type == "DECIMAL") {
    optional<double> value = isNull ? nullopt : optional<double>(valueJObj.get<double>());
    return fpdb::expression::gandiva::num_lit<arrow::DoubleType>(value);
  } else if (type == "BOOLEAN") {
    optional<bool> value = isNull ? nullopt : optional<bool>(valueJObj.get<bool>());
    return fpdb::expression::gandiva::num_lit<arrow::BooleanType>(value);
  } else if (type == "DATE_MS") {
    optional<long> value = isNull ? nullopt : optional<long>(valueJObj.get<long>());
    return fpdb::expression::gandiva::num_lit<arrow::Date64Type>(value);
  } else if (type == "INTERVAL_DAY") {
    optional<int> value = isNull ? nullopt : optional<int>(valueJObj.get<int>());
    return fpdb::expression::gandiva::num_lit<arrow::Int32Type>(value,
                                                                  fpdb::expression::gandiva::DateIntervalType::DAY);
  } else if (type == "INTERVAL_MONTH") {
    optional<int> value = isNull ? nullopt : optional<int>(valueJObj.get<int>());
    return fpdb::expression::gandiva::num_lit<arrow::Int32Type>(value,
                                                                  fpdb::expression::gandiva::DateIntervalType::MONTH);
  }
  else {
    throw runtime_error(fmt::format("Unsupported literal type, {}, from: {}", type, to_string(literalJObj)));
  }
}

shared_ptr<fpdb::expression::gandiva::Expression> CalcitePlanJsonDeserializer::deserializeAndOrNotOperation(const string &opName, const json &jObj) {
  if (opName == "AND" || opName == "OR") {
    const auto &exprsJArr = jObj["operands"].get<vector<json>>();
    vector<shared_ptr<fpdb::expression::gandiva::Expression>> operands;
    operands.reserve(exprsJArr.size());
    for (const auto &exprJObj: exprsJArr) {
      operands.emplace_back(deserializeExpression(exprJObj));
    }
    if (opName == "AND") {
      return and_(operands);
    } else {
      return or_(operands);
    }
  } else {
    const auto &operands = jObj["operands"].get<vector<json>>();
    const auto expr = deserializeExpression(operands[0]);
    return not_(expr);
  }
}

shared_ptr<fpdb::expression::gandiva::Expression> CalcitePlanJsonDeserializer::deserializeBinaryOperation(const string &opName, const json &jObj) {
  const auto &leftExpr = deserializeExpression(jObj["operands"].get<vector<json>>()[0]);
  const auto &rightExpr = deserializeExpression(jObj["operands"].get<vector<json>>()[1]);

  // plus, minus
  if (opName == "PLUS" || opName == "MINUS") {
    // handle date plus, minus
    if (leftExpr->getTypeString() == "NumericLiteral<Date64>" || rightExpr->getTypeString() == "NumericLiteral<Date64>") {
      // check interval type
      optional<fpdb::expression::gandiva::DateIntervalType> intervalType;
      if (leftExpr->getTypeString() == "NumericLiteral<Int32>") {
        intervalType = static_pointer_cast<fpdb::expression::gandiva::NumericLiteral<arrow::Int32Type>>(leftExpr)->getIntervalType();
      } else {
        intervalType = static_pointer_cast<fpdb::expression::gandiva::NumericLiteral<arrow::Int32Type>>(rightExpr)->getIntervalType();
      }
      if (!intervalType.has_value()) {
        throw runtime_error("Invalid date plus/minus operation, interval not found");
      }

      // make expression, currently gandiva does not support date minus,
      // so we need to make subtrahend opposite and use plus
      if (opName == "MINUS") {
        if (rightExpr->getTypeString() != "NumericLiteral<Int32>") {
          throw runtime_error(fmt::format("Invalid date minus operation, subtrahend is not an interval, but: {}",
                                          rightExpr->getTypeString()));
        }
        static_pointer_cast<fpdb::expression::gandiva::NumericLiteral<arrow::Int32Type>>(rightExpr)->makeOpposite();
      }
      return datePlus(leftExpr, rightExpr, intervalType.value());
    }

    // regular plus, minus
    else {
      if (opName == "PLUS") {
        return fpdb::expression::gandiva::plus(leftExpr, rightExpr);
      } else {
        return fpdb::expression::gandiva::minus(leftExpr, rightExpr);
      }
    }
  }

  // other binary operations
  else if (opName == "TIMES") {
    return times(leftExpr, rightExpr);
  } else if (opName == "DIVIDE") {
    return divide(leftExpr, rightExpr);
  } else if (opName == "EQUALS") {
    return eq(leftExpr, rightExpr);
  } else if (opName == "NOT_EQUALS") {
    return neq(leftExpr, rightExpr);
  } else if (opName == "LESS_THAN") {
    return lt(leftExpr, rightExpr);
  } else if (opName == "GREATER_THAN") {
    return gt(leftExpr, rightExpr);
  } else if (opName == "LESS_THAN_OR_EQUAL") {
    return lte(leftExpr, rightExpr);
  } else if (opName == "GREATER_THAN_OR_EQUAL") {
    return gte(leftExpr, rightExpr);
  } else if (opName == "LIKE")  {
    return like(leftExpr, rightExpr);
  }

  // invalid
  throw runtime_error(fmt::format("Unsupported binary expression type, {}, from: {}", opName, to_string(jObj)));
}

shared_ptr<fpdb::expression::gandiva::Expression> CalcitePlanJsonDeserializer::deserializeInOperation(const json &jObj) {
  const auto &operandsJArr = jObj["operands"].get<vector<json>>();
  const auto &leftExpr = deserializeExpression(operandsJArr[0]);
  const auto &literalsJObj = operandsJArr[1]["literals"];
  const auto &type = literalsJObj["type"].get<string>();

  if (type == "CHAR" || type == "VARCHAR") {
    const unordered_set<string> &values = literalsJObj["values"].get<unordered_set<string>>();
    return fpdb::expression::gandiva::in<arrow::StringType, string>(leftExpr, values);
  }
  else if (type == "INTEGER" || type == "BIGINT") {
    // gandiva only supports binary expressions with same left, right and return type,
    // so to avoid casting we make all integer fields as int64, as well as scalars.
    const unordered_set<int64_t> &values = literalsJObj["values"].get<unordered_set<int64_t>>();
    return fpdb::expression::gandiva::in<arrow::Int64Type, int64_t>(leftExpr, values);
  }
  else if (type == "DECIMAL") {
    const unordered_set<double> &values = literalsJObj["values"].get<unordered_set<double>>();
    return fpdb::expression::gandiva::in<arrow::DoubleType, double>(leftExpr, values);
  }
  else if (type == "DATE_MS") {
    const unordered_set<int64_t> &values = literalsJObj["values"].get<unordered_set<int64_t>>();
    return fpdb::expression::gandiva::in<arrow::Date64Type, int64_t>(leftExpr, values);
  }
  else {
    throw runtime_error(fmt::format("Unsupported literal type, {}, from: {}", type, to_string(literalsJObj)));
  }
}

shared_ptr<fpdb::expression::gandiva::Expression> CalcitePlanJsonDeserializer::deserializeCaseOperation(const json &jObj) {
  const auto &condJObj = jObj["condition"];
  const auto &ifExpr = deserializeExpression(condJObj["if"]);
  const auto &thenExpr = deserializeExpression(condJObj["then"]);
  const auto &elseExpr = deserializeExpression(condJObj["else"]);
  return if_(ifExpr, thenExpr, elseExpr);
}

shared_ptr<fpdb::expression::gandiva::Expression> CalcitePlanJsonDeserializer::deserializeExtractOperation(const json &jObj) {
  const auto &operands = jObj["operands"].get<vector<json>>();
  // date expression
  const auto &dateExpr = deserializeExpression(operands[1]);

  // extract symbol
  if (!operands[0].contains("literal")) {
    throw runtime_error("Unsupported extract, no literal to extract");
  }
  const auto &literalObj = operands[0]["literal"];
  if (!literalObj.contains("type") || literalObj["type"].get<string>() != "SYMBOL") {
    throw runtime_error("Unsupported extract, extract literal type is not symbol");
  }
  const auto &symbol = literalObj["value"].get<string>();
  fpdb::expression::gandiva::DateIntervalType intervalType;
  if (symbol == "DAY") {
    intervalType = fpdb::expression::gandiva::DateIntervalType::DAY;
  } else if (symbol == "MONTH") {
    intervalType = fpdb::expression::gandiva::DateIntervalType::MONTH;
  } else if (symbol == "YEAR") {
    intervalType = fpdb::expression::gandiva::DateIntervalType::YEAR;
  } else {
    throw runtime_error(fmt::format("Unsupported extract literal symbol: {}", symbol));
  }

  return dateExtract(dateExpr, intervalType);
}

shared_ptr<fpdb::expression::gandiva::Expression> CalcitePlanJsonDeserializer::deserializeNullOperation(const string &opName, const json &jObj) {
  if (opName == "IS_NULL") {
    const auto &expr = deserializeExpression(jObj["operands"].get<vector<json>>()[0]);
    return isNull(expr);
  } else {
    throw runtime_error(fmt::format("Unsupported null expression type, {}, from: {}", opName, to_string(jObj)));
  }
}

shared_ptr<fpdb::expression::gandiva::Expression> CalcitePlanJsonDeserializer::deserializeSubstrOperation(const json &jObj) {
  const auto &operandsJArr = jObj["operands"].get<vector<json>>();
  const auto &expr = deserializeExpression(operandsJArr[0]);
  const auto &fromLit = deserializeExpression(operandsJArr[1]);
  const auto &forLit = deserializeExpression(operandsJArr[2]);
  return substr(expr, fromLit, forLit);
}

shared_ptr<fpdb::expression::gandiva::Expression> CalcitePlanJsonDeserializer::deserializeCastOperation(const json &jObj) {
  const auto &operand = jObj["operand"].get<json>();
  const auto &expr = deserializeExpression(operand);
  const auto &type = deserializeDataType(jObj);

  return cast(expr, type);
}

shared_ptr<::arrow::DataType> CalcitePlanJsonDeserializer::deserializeDataType(const json &jObj) {
  const auto &type = jObj["type"].get<std::string>();

  if(type == "INTEGER"){
    return ::arrow::int64();
  }
  // TODO: Add more types
//  else if(type == "INTEGER"){
//    return ::arrow::int64();
//  }
  else{
    throw runtime_error(fmt::format("Unrecognized data type, {}, from: {}", type, to_string(jObj)));
  }
}

shared_ptr<fpdb::expression::gandiva::Expression> CalcitePlanJsonDeserializer::deserializeOperation(const json &jObj) {
  const string &opName = jObj["op"].get<string>();
  // and, or
  if (opName == "AND" || opName == "OR" || opName == "NOT") {
    return deserializeAndOrNotOperation(opName, jObj);
  }
  // binary operation
  else if (opName == "PLUS" || opName == "MINUS" || opName == "TIMES" || opName == "DIVIDE" || opName == "EQUALS"
    || opName == "NOT_EQUALS" || opName == "LESS_THAN" || opName == "GREATER_THAN" || opName == "LESS_THAN_OR_EQUAL"
    || opName == "GREATER_THAN_OR_EQUAL" || opName == "LIKE") {
    return deserializeBinaryOperation(opName, jObj);
  }
  // in
  else if (opName == "IN") {
    return deserializeInOperation(jObj);
  }
  // case
  else if (opName == "CASE") {
    return deserializeCaseOperation(jObj);
  }
  // extract
  else if (opName == "EXTRACT") {
    return deserializeExtractOperation(jObj);
  }
  // is null
  else if (opName == "IS_NULL") {
    return deserializeNullOperation(opName, jObj);
  }
  else if (opName == "SUBSTRING") {
    return deserializeSubstrOperation(jObj);
  }
  else if (opName == "CAST") {
    return deserializeCastOperation(jObj);
  }
  // invalid
  else {
    throw runtime_error(fmt::format("Unsupported expression type, {}, from: {}", opName, to_string(jObj)));
  }
}

shared_ptr<fpdb::expression::gandiva::Expression> CalcitePlanJsonDeserializer::deserializeExpression(const json &jObj) {
  // single column
  if (jObj.contains("inputRef")) {
    return deserializeInputRef(jObj);
  }
  // literal
  else if (jObj.contains("literal")) {
    return deserializeLiteral(jObj);
  }
  // operation
  else if (jObj.contains("op")) {
    return deserializeOperation(jObj);
  }
  // invalid
  else {
    throw runtime_error(fmt::format("Unsupported expression type, only column, literal and operation are "
                                    "supported, from: {}", to_string(jObj)));
  }
}

unordered_map<string, string> CalcitePlanJsonDeserializer::deserializeColumnRenames(const vector<json> &jArr) {
  unordered_map<string, string> renames;
  for (const auto &renameJObj: jArr) {
    const auto &oldName = ColumnName::canonicalize(renameJObj["old"].get<string>());
    const auto &newName = ColumnName::canonicalize(renameJObj["new"].get<string>());
    renames.emplace(oldName, newName);
  }
  return renames;
}

pair<vector<string>, vector<string>> CalcitePlanJsonDeserializer::deserializeHashJoinCondition(const json &jObj) {
  const string &opName = jObj["op"].get<string>();
  if (opName == "AND") {
    vector<string> leftColumnNames, rightColumnNames;
    auto childJObj1 = jObj["operands"].get<vector<json>>()[0];
    auto childJObj2 = jObj["operands"].get<vector<json>>()[1];
    const auto &childJoinColumns1 = deserializeHashJoinCondition(childJObj1);
    const auto &childJoinColumns2 = deserializeHashJoinCondition(childJObj2);
    leftColumnNames.insert(leftColumnNames.end(), childJoinColumns1.first.begin(), childJoinColumns1.first.end());
    leftColumnNames.insert(leftColumnNames.end(), childJoinColumns2.first.begin(), childJoinColumns2.first.end());
    rightColumnNames.insert(rightColumnNames.end(), childJoinColumns1.second.begin(), childJoinColumns1.second.end());
    rightColumnNames.insert(rightColumnNames.end(), childJoinColumns2.second.begin(), childJoinColumns2.second.end());
    return make_pair(leftColumnNames, rightColumnNames);
  }

  else if (opName == "EQUALS") {
    auto leftJObj = jObj["operands"].get<vector<json>>()[0];
    auto rightJObj = jObj["operands"].get<vector<json>>()[1];
    auto leftColumnName = ColumnName::canonicalize(leftJObj["inputRef"].get<string>());
    auto rightColumnName = ColumnName::canonicalize(rightJObj["inputRef"].get<string>());
    return make_pair(vector<string>{leftColumnName}, vector<string>{rightColumnName});
  }

  else {
    throw runtime_error(fmt::format("Invalid hash join condition operation type, {}, from: {}",
                                    opName, to_string(jObj)));
  }
}

void CalcitePlanJsonDeserializer::addProjectForJoinColumnRenames(shared_ptr<PrePhysicalOp> &op,
                                                                 const vector<shared_ptr<PrePhysicalOp>> &producers,
                                                                 const json &jObj) {
  const auto &leftProducer = producers[0];
  const auto &rightProducer = producers[1];

  // deserialize input column renames and create a project below, if any
  vector<shared_ptr<PrePhysicalOp>> calibratedProducers;

  // left
  if (jObj.contains("leftFieldRenames")) {
    const auto &leftFieldRenames = deserializeColumnRenames(jObj["leftFieldRenames"]);
    const auto &leftFieldNames = jObj["leftFieldNames"].get<vector<string>>();
    vector<pair<string, string>> projectColumnNamePairs;
    set<string> projectColumnNames;
    for (const auto &column: leftFieldNames) {
      const auto &renameIt = leftFieldRenames.find(column);
      const auto &canonColumn = ColumnName::canonicalize(column);
      if (renameIt == leftFieldRenames.end()) {
        projectColumnNamePairs.emplace_back(make_pair(canonColumn, canonColumn));
        projectColumnNames.emplace(canonColumn);
      } else {
        const auto &renameColumn = ColumnName::canonicalize(renameIt->second);
        projectColumnNamePairs.emplace_back(make_pair(canonColumn, renameColumn));
        projectColumnNames.emplace(renameColumn);
      }
    }


    shared_ptr<ProjectPrePOp> projectPrePOp = make_shared<ProjectPrePOp>(pOpIdGenerator_.fetch_add(1),
                                                                         vector<shared_ptr<fpdb::expression::gandiva::Expression>>(),
                                                                         vector<string>(),
                                                                         projectColumnNamePairs);
    projectPrePOp->setProjectColumnNames(projectColumnNames);
    projectPrePOp->setProducers({leftProducer});
    calibratedProducers.emplace_back(projectPrePOp);
  } else {
    calibratedProducers.emplace_back(leftProducer);
  }

  // right
  if (jObj.contains("rightFieldRenames")) {
    const auto &rightFieldRenames = deserializeColumnRenames(jObj["rightFieldRenames"]);
    const auto &rightFieldNames = jObj["rightFieldNames"].get<vector<string>>();
    vector<pair<string, string>> projectColumnNamePairs;
    set<string> projectColumnNames;
    for (const auto &column: rightFieldNames) {
      const auto &renameIt = rightFieldRenames.find(column);
      const auto &canonColumn = ColumnName::canonicalize(column);
      if (renameIt == rightFieldRenames.end()) {
        projectColumnNamePairs.emplace_back(make_pair(canonColumn, canonColumn));
        projectColumnNames.emplace(canonColumn);
      } else {
        const auto &renameColumn = ColumnName::canonicalize(renameIt->second);
        projectColumnNamePairs.emplace_back(make_pair(canonColumn, renameColumn));
        projectColumnNames.emplace(renameColumn);
      }
    }

    shared_ptr<ProjectPrePOp> projectPrePOp = make_shared<ProjectPrePOp>(pOpIdGenerator_.fetch_add(1),
                                                                         vector<shared_ptr<fpdb::expression::gandiva::Expression>>(),
                                                                         vector<string>(),
                                                                         projectColumnNamePairs);
    projectPrePOp->setProjectColumnNames(projectColumnNames);
    projectPrePOp->setProducers({rightProducer});
    calibratedProducers.emplace_back(projectPrePOp);
  } else {
    calibratedProducers.emplace_back(rightProducer);
  }

  op->setProducers(calibratedProducers);
}

vector<SortKey> CalcitePlanJsonDeserializer::deserializeSortKeys(const json &jObj) {
  vector<SortKey> sortKeys;
  for (const auto &sortFieldJObj: jObj) {
    const string &columnName = ColumnName::canonicalize(sortFieldJObj["field"].get<string>());
    const string &directionStr = sortFieldJObj["direction"].get<string>();
    auto direction = (directionStr == "ASCENDING") ? ASCENDING : DESCENDING;
    sortKeys.emplace_back(SortKey(columnName, direction));
  }
  return sortKeys;
}

shared_ptr<SortPrePOp> CalcitePlanJsonDeserializer::deserializeSort(const json &jObj) {
  // deserialize sort fields
  const auto &sortFieldsJArr = jObj["sortFields"].get<vector<json>>();
  const auto &sortKeys = deserializeSortKeys(sortFieldsJArr);
  shared_ptr<SortPrePOp> sortPrePOp = make_shared<SortPrePOp>(pOpIdGenerator_.fetch_add(1), sortKeys);

  // deserialize producers
  sortPrePOp->setProducers(deserializeProducers(jObj));
  return sortPrePOp;
}

shared_ptr<LimitSortPrePOp> CalcitePlanJsonDeserializer::deserializeLimitSort(const json &jObj) {
  // deserialize sort fields
  const auto &sortFieldsJArr = jObj["sortFields"].get<vector<json>>();
  const auto &sortKeys = deserializeSortKeys(sortFieldsJArr);

  // deserialize limit
  const auto &limitJObj = jObj["limit"];
  if (!limitJObj.contains("literal")) {
    throw runtime_error(fmt::format("Invalid sort limit, not an literal, from: {}", to_string(limitJObj)));
  }
  const auto &limitLiteralJObj = limitJObj["literal"];
  if (limitLiteralJObj["type"].get<string>() != "INTEGER") {
    throw runtime_error(fmt::format("Invalid sort limit literal, not an integer, from: {}",
                                    to_string(limitLiteralJObj)));
  }
  int64_t limitVal = limitLiteralJObj["value"].get<int64_t>();
  shared_ptr<LimitSortPrePOp> limitSortPrePOp = make_shared<LimitSortPrePOp>(pOpIdGenerator_.fetch_add(1),
                                                                             limitVal,
                                                                             sortKeys);

  // deserialize producers
  limitSortPrePOp->setProducers(deserializeProducers(jObj));
  return limitSortPrePOp;
}

shared_ptr<PrePhysicalOp> CalcitePlanJsonDeserializer::deserializeAggregateOrGroup(json &jObj) {
  // deserialize group fields
  const auto &groupFieldsJArr = jObj["groupFields"].get<vector<json>>();
  vector<string> groupColumnNames;
  groupColumnNames.reserve(groupFieldsJArr.size());
  for (const auto &groupFieldJObj: groupFieldsJArr) {
    const string &groupColumnName = ColumnName::canonicalize(groupFieldJObj.get<string>());
    groupColumnNames.emplace_back(groupColumnName);
  }

  // deserialize aggregate output columns and functions
  const auto &aggregationsJArr = jObj["aggregations"].get<vector<json>>();
  vector<string> aggOutputColumnNames;
  vector<shared_ptr<AggregatePrePFunction>> aggFunctions;
  aggOutputColumnNames.reserve(aggregationsJArr.size());
  aggFunctions.reserve(aggregationsJArr.size());

  for (const auto &aggregationJObj: aggregationsJArr) {
    // aggregate output column
    const string &aggOutputColumnName = ColumnName::canonicalize(aggregationJObj["aggOutputField"].get<string>());
    aggOutputColumnNames.emplace_back(aggOutputColumnName);

    // aggregate function type
    const string &aggFunctionStr = aggregationJObj["aggFunction"].get<string>();
    AggregatePrePFunctionType aggFunctionType;
    if (aggFunctionStr == "SUM0" || aggFunctionStr == "SUM") {
      aggFunctionType = SUM;
    } else if (aggFunctionStr == "MIN") {
      aggFunctionType = MIN;
    } else if (aggFunctionStr == "MAX") {
      aggFunctionType = MAX;
    } else if (aggFunctionStr == "AVG") {
      aggFunctionType = AVG;
    } else if (aggFunctionStr == "COUNT") {
      aggFunctionType = COUNT;
    } else {
      throw runtime_error(fmt::format("Unsupported aggregation function type, {}, from: {}",
                                      aggFunctionStr, to_string(aggregationJObj)));
    }

    // aggregate expr if any (count may not have)
    shared_ptr<fpdb::expression::gandiva::Expression> aggFieldExpr;
    if (aggregationJObj.contains("aggInputField")) {
      const string &aggInputColumnName = ColumnName::canonicalize(aggregationJObj["aggInputField"].get<string>());
      if (aggInputColumnName.substr(0, 2) == "$f") {
        // it's not just a column, need to get it from the input Project op
        auto inputProjectJObj = jObj["inputs"].get<vector<json>>()[0];
        size_t aggInputProjectFieldId = stoul(aggInputColumnName.substr(2, aggInputColumnName.length() - 2));
        auto aggFieldExprJObj = inputProjectJObj["fields"].get<vector<json>>()[aggInputProjectFieldId]["expr"];
        aggFieldExpr = deserializeExpression(aggFieldExprJObj);

        // need to let the input Project op know the expr has been consumed
        json consumedProjectFieldIdJArr;
        if (inputProjectJObj.contains("consumedFieldsId")) {
          consumedProjectFieldIdJArr = inputProjectJObj["consumedFieldsId"].get<vector<size_t>>();
        }
        consumedProjectFieldIdJArr.emplace_back(aggInputProjectFieldId);
        inputProjectJObj["consumedFieldsId"] = consumedProjectFieldIdJArr;
        jObj["inputs"] = vector<json>{inputProjectJObj};
      } else {
        // it's just a column
        aggFieldExpr = fpdb::expression::gandiva::col(aggInputColumnName);
      }
    }

    // make the aggregate function
    shared_ptr<AggregatePrePFunction> aggFunction = make_shared<AggregatePrePFunction>(aggFunctionType, aggFieldExpr);
    aggFunctions.emplace_back(aggFunction);
  }

  // project column names
  set<string> projectColumnNames;
  projectColumnNames.insert(aggOutputColumnNames.begin(), aggOutputColumnNames.end());

  // decide if it's an Aggregate op or a Group op
  shared_ptr<PrePhysicalOp> prePOp;
  if (groupColumnNames.empty()) {
    prePOp = make_shared<AggregatePrePOp>(pOpIdGenerator_.fetch_add(1),
                                          aggOutputColumnNames,
                                          aggFunctions);
  } else {
    projectColumnNames.insert(groupColumnNames.begin(), groupColumnNames.end());
    prePOp = make_shared<GroupPrePOp>(pOpIdGenerator_.fetch_add(1),
                                      groupColumnNames,
                                      aggOutputColumnNames,
                                      aggFunctions);
  }
  prePOp->setProjectColumnNames(projectColumnNames);

  // deserialize producers
  prePOp->setProducers(deserializeProducers(jObj));
  return prePOp;
}

shared_ptr<prephysical::PrePhysicalOp> CalcitePlanJsonDeserializer::deserializeProject(const json &jObj) {
  // whether we can skip to deserialize the Project
  // we need to keep the Project when there is an expr unconsumed by the consumer or there is a rename
  bool skipSelf = true;

  // consumed field ids
  set<size_t> consumedFieldsIdSet;
  if (jObj.contains("consumedFieldsId")) {
    const vector<size_t> &consumedFieldsIdVec = jObj["consumedFieldsId"].get<vector<size_t>>();
    consumedFieldsIdSet.insert(consumedFieldsIdVec.begin(), consumedFieldsIdVec.end());
  }

  // project columns (may need rename) and exprs
  set<string> usedColumnNames, projectColumnNames;
  vector<pair<string, string>> projectColumnNamePairs;
  vector<shared_ptr<fpdb::expression::gandiva::Expression>> exprs;
  vector<string> exprNames;
  const auto &fieldsJArr = jObj["fields"].get<vector<json>>();

  for (uint fieldId = 0; fieldId < fieldsJArr.size(); ++fieldId) {
    const auto &fieldJObj = fieldsJArr[fieldId];
    // deserialize field
    const auto &outputColumnName = ColumnName::canonicalize(fieldJObj["name"].get<string>());
    const auto &fieldExprJObj = fieldJObj["expr"];

    if (fieldExprJObj.contains("inputRef")) {
      const string &inputColumnName = ColumnName::canonicalize(fieldExprJObj["inputRef"].get<string>());
      usedColumnNames.emplace(inputColumnName);
      projectColumnNames.emplace(outputColumnName);
      projectColumnNamePairs.emplace_back(make_pair(inputColumnName, outputColumnName));
      if (inputColumnName != outputColumnName) {
        // we need to keep the project to do the rename
        skipSelf = false;
      }
    }

    else if (fieldExprJObj.contains("op") || fieldExprJObj.contains("literal")) {
      const auto &expr = deserializeExpression(fieldExprJObj);
      const auto &exprUsedColumnNames = expr->involvedColumnNames();
      usedColumnNames.insert(exprUsedColumnNames.begin(), exprUsedColumnNames.end());

      // check consumed id
      if (consumedFieldsIdSet.find(fieldId) != consumedFieldsIdSet.end()) {
        // this field is consumed by the consumer, here we add its involvedColumnNames instead of itself
        for (const auto &columnName: exprUsedColumnNames) {
          const auto &canonColumnName = ColumnName::canonicalize(columnName);
          projectColumnNames.emplace(canonColumnName);
          projectColumnNamePairs.emplace_back(make_pair(canonColumnName, canonColumnName));
        }
        continue;
      }
      // this field is unconsumed by the consumer, and it's an expr
      exprs.emplace_back(expr);
      exprNames.emplace_back(outputColumnName);
      projectColumnNames.emplace(outputColumnName);
      skipSelf = false;
    }

    else {
      throw runtime_error(fmt::format("Invalid project field, only column or expr are valid, from: {}",
                                      to_string(jObj)));
    }
  }

  if (skipSelf) {
    // skip the Project, continue to deserialize its producer
    const auto &producer = deserializeDfs(jObj["inputs"].get<vector<json>>()[0]);
    // set projectColumnNames for its producer
    producer->setProjectColumnNames(usedColumnNames);
    return producer;
  }

  else {
    // not skip the Project
    shared_ptr<ProjectPrePOp> projectPrePOp = make_shared<ProjectPrePOp>(pOpIdGenerator_.fetch_add(1),
                                                                         exprs,
                                                                         exprNames,
                                                                         projectColumnNamePairs);
    projectPrePOp->setProjectColumnNames(projectColumnNames);

    // deserialize producers
    projectPrePOp->setProducers(deserializeProducers(jObj));
    return projectPrePOp;
  }
}

shared_ptr<prephysical::HashJoinPrePOp> CalcitePlanJsonDeserializer::deserializeHashJoin(const json &jObj) {
  // deserialize join type
  const auto &joinTypeStr = jObj["joinType"].get<string>();
  JoinType joinType;
  if (joinTypeStr == "INNER") {
    joinType = INNER;
  } else if (joinTypeStr == "LEFT") {
    joinType = LEFT;
  } else if (joinTypeStr == "RIGHT") {
    joinType = RIGHT;
  } else if (joinTypeStr == "FULL") {
    joinType = FULL;
  } else if (joinTypeStr == "SEMI") {
    joinType = SEMI;
  } else {
    throw runtime_error(fmt::format("Unsupported hash join type, {}, from: {}", joinTypeStr, to_string(jObj)));
  }

  // deserialize hash join condition
  if (!jObj.contains("condition")) {
    throw runtime_error("Invalid hash join, no join condition");
  }
  const auto &joinCondJObj = jObj["condition"];
  const pair<vector<string>, vector<string>> &joinColumnNames = deserializeHashJoinCondition(joinCondJObj);

  // deserialize pushable
  if (!jObj.contains("pushable")) {
    throw runtime_error("Invalid hash join, no field 'pushable'");
  }
  bool pushable = jObj["pushable"].get<bool>();

  shared_ptr<PrePhysicalOp> hashJoinPrePOp = make_shared<HashJoinPrePOp>(pOpIdGenerator_.fetch_add(1),
                                                                         joinType,
                                                                         joinColumnNames.first,
                                                                         joinColumnNames.second,
                                                                         pushable);

  // deserialize producers
  const auto &producers = deserializeProducers(jObj);
  addProjectForJoinColumnRenames(hashJoinPrePOp, producers, jObj);

  return static_pointer_cast<HashJoinPrePOp>(hashJoinPrePOp);
}

shared_ptr<prephysical::NestedLoopJoinPrePOp> CalcitePlanJsonDeserializer::deserializeNestedLoopJoin(const json &jObj) {
  // deserialize join type
  const auto &joinTypeStr = jObj["joinType"].get<string>();
  JoinType joinType;
  if (joinTypeStr == "INNER") {
    joinType = INNER;
  } else if (joinTypeStr == "LEFT") {
    joinType = LEFT;
  } else if (joinTypeStr == "RIGHT") {
    joinType = RIGHT;
  } else if (joinTypeStr == "FULL") {
    joinType = FULL;
  } else {
    throw runtime_error(fmt::format("Unsupported nested loop join type, {}, from: {}", joinTypeStr, to_string(jObj)));
  }

  // deserialize join condition, if has
  shared_ptr<fpdb::expression::gandiva::Expression> predicate = nullptr;
  if (jObj.contains("condition")) {
    predicate = deserializeExpression(jObj["condition"]);
  }

  shared_ptr<PrePhysicalOp> nestedLoopJoinPrePOp = make_shared<NestedLoopJoinPrePOp>(pOpIdGenerator_.fetch_add(1),
                                                                                     joinType,
                                                                                     predicate);

  // deserialize producers
  const auto &producers = deserializeProducers(jObj);
  addProjectForJoinColumnRenames(nestedLoopJoinPrePOp, producers, jObj);

  return static_pointer_cast<NestedLoopJoinPrePOp>(nestedLoopJoinPrePOp);
}

shared_ptr<prephysical::PrePhysicalOp> CalcitePlanJsonDeserializer::deserializeFilterOrFilterableScan(const json &jObj) {
  // deserialize filter predicate
  const auto &predicate = deserializeExpression(jObj["condition"]);

  // deserialize producers
  const auto &producers = deserializeProducers(jObj);
  if (producers[0]->getType() == FILTERABLE_SCAN) {
    // if the producer is filterable scan, then set its filter predicate
    const auto &filterableScan = static_pointer_cast<FilterableScanPrePOp>(producers[0]);
    filterableScan->setPredicate(predicate);
    return filterableScan;
  }

  else {
    // else do regularly, make a filter op
    shared_ptr<FilterPrePOp> filterPrePOp = make_shared<FilterPrePOp>(pOpIdGenerator_.fetch_add(1),
                                                                      predicate);
    filterPrePOp->setProducers(deserializeProducers(jObj));
    return filterPrePOp;
  }
}

shared_ptr<prephysical::FilterableScanPrePOp> CalcitePlanJsonDeserializer::deserializeTableScan(const json &jObj) {
  const string &tableName = jObj["table"].get<string>();
  shared_ptr<Table> table;

  set<string> columnNameSet;
  // fetch table from catalogue entry
  if (catalogueEntry_->getType() == CatalogueEntryType::OBJ_STORE) {
    const auto objStoreCatalogueEntry = static_pointer_cast<obj_store::ObjStoreCatalogueEntry>(catalogueEntry_);
    table = objStoreCatalogueEntry->getTable(tableName);
    const vector<string> &columnNames = table->getColumnNames();
    columnNameSet.insert(columnNames.begin(), columnNames.end());
  } else {
    throw runtime_error(fmt::format("Unsupported catalogue entry type: {}, from: {}",
                                    catalogueEntry_->getType(), catalogueEntry_->getName()));
  }

  auto filterableScanPrePOp = make_shared<prephysical::FilterableScanPrePOp>(pOpIdGenerator_.fetch_add(1), table);
  filterableScanPrePOp->setProjectColumnNames(columnNameSet);
  return filterableScanPrePOp;
}

}
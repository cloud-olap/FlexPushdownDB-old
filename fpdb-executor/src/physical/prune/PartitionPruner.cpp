//
// Created by Yifei Yang on 11/21/21.
//

#include <fpdb/executor/physical/prune/PartitionPruner.h>
#include <fpdb/expression/gandiva/Canonicalizer.h>
#include <fpdb/expression/gandiva/And.h>
#include <fpdb/expression/gandiva/Or.h>
#include <fpdb/expression/gandiva/BinaryExpression.h>
#include <fpdb/expression/gandiva/Column.h>
#include <fpdb/expression/gandiva/StringLiteral.h>
#include <fpdb/expression/gandiva/NumericLiteral.h>
#include <fpdb/expression/gandiva/Util.h>
#include <fpdb/util/Util.h>
#include <fmt/format.h>

using namespace fpdb::expression::gandiva;
using namespace fpdb::util;

namespace fpdb::executor::physical {

unordered_map<shared_ptr<Partition>, shared_ptr<Expression>, PartitionPointerHash, PartitionPointerPredicate>
PartitionPruner::prune(const vector<shared_ptr<Partition>> &partitions, const shared_ptr<Expression> &predicate) {
  unordered_map<shared_ptr<Partition>, shared_ptr<Expression>, PartitionPointerHash, PartitionPointerPredicate> pruneRes;
  // No predicate, then no partition can be pruned
  if (!predicate) {
    for (const auto &partition: partitions) {
      pruneRes.emplace(partition, nullptr);
    }
    return pruneRes;
  }

  const auto canonicalizedPred = Canonicalizer::canonicalize(predicate);
  for (const auto &partition: partitions) {
    auto prunedPredicate = predicate;
    const auto &zoneMap = partition->getZoneMap();
    for (const auto &it: zoneMap) {
      const auto &columnName = it.first;
      const auto &valuePair = it.second;
      prunedPredicate = prunePredicate(prunedPredicate, columnName, valuePair.first, valuePair.second);
    }

    // if after pruning there is no predicate left, the partition can be pruned
    if (prunedPredicate) {
      pruneRes.emplace(partition, prunedPredicate);
    }
  }

  return pruneRes;
}

shared_ptr<Expression> PartitionPruner::prunePredicate(const shared_ptr<Expression> &predicate, const string &columnName,
                                                       const shared_ptr<Scalar> &statsMin, const shared_ptr<Scalar> &statsMax) {
  auto type = predicate->getType();
  switch (type) {
    case AND: {
      const auto &andExpr = static_pointer_cast<And>(predicate);
      vector<shared_ptr<Expression>> prunedChildExprs;
      for (const auto &childExpr: andExpr->getExprs()) {
        const auto &prunedChildExpr = prunePredicate(childExpr, columnName, statsMin, statsMax);
        if (!prunedChildExpr) {
          return nullptr;
        }
        prunedChildExprs.emplace_back(prunedChildExpr);
      }
      return and_(prunedChildExprs);
    }

    case OR: {
      const auto &orExpr = static_pointer_cast<Or>(predicate);
      vector<shared_ptr<Expression>> prunedChildExprs;
      for (const auto &childExpr: orExpr->getExprs()) {
        const auto &prunedChildExpr = prunePredicate(childExpr, columnName, statsMin, statsMax);
        if (prunedChildExpr) {
          prunedChildExprs.emplace_back(prunedChildExpr);
        }
      }
      if (prunedChildExprs.empty()) {
        return nullptr;
      } else if (prunedChildExprs.size() == 1) {
        return prunedChildExprs[0];
      } else {
        return or_(prunedChildExprs);
      }
    }

    case EQUAL_TO:
    case LESS_THAN:
    case LESS_THAN_OR_EQUAL_TO:
    case GREATER_THAN:
    case GREATER_THAN_OR_EQUAL_TO: {
      const auto &biExpr = static_pointer_cast<BinaryExpression>(predicate);
      const auto &leftExpr = biExpr->getLeft();
      const auto &rightExpr = biExpr->getRight();

      if (leftExpr->getType() == COLUMN && Util::isLiteral(rightExpr)) {
        const auto &colExpr = static_pointer_cast<Column>(leftExpr);
        if (colExpr->getColumnName() != columnName) {
          return predicate;
        }
        const auto &litScalar = litToScalar(rightExpr);

        // check if the predicate gives empty result on the min-max
        bool valid;
        switch (type) {
          case EQUAL_TO: {
            valid = checkValid(litScalar, litScalar, false, false, statsMin, statsMax);
            break;
          }
          case LESS_THAN: {
            valid = checkValid(nullptr, litScalar, false, true, statsMin, statsMax);
            break;
          }
          case LESS_THAN_OR_EQUAL_TO: {
            valid = checkValid(nullptr, litScalar, false, false, statsMin, statsMax);
            break;
          }
          case GREATER_THAN: {
            valid = checkValid(litScalar, nullptr, true, false, statsMin, statsMax);
            break;
          }
          default: {
            valid = checkValid(litScalar, nullptr, false, false, statsMin, statsMax);
            break;
          }
        }
        if (valid) {
          return predicate;
        } else {
          return nullptr;
        }
      }
    }

    default:
      return predicate;
  }
}

shared_ptr<Scalar> PartitionPruner::litToScalar(const shared_ptr<Expression> &literal) {
  switch (literal->getType()) {
    case expression::gandiva::STRING_LITERAL: {
      const auto &strLit = static_pointer_cast<StringLiteral>(literal);
      const auto &value = strLit->value();
      if (value.has_value()) {
        return Scalar::make(arrow::MakeScalar(*value));
      } else {
        return nullptr;
      }
    }
    case expression::gandiva::NUMERIC_LITERAL: {
      if (literal->getTypeString() == "NumericLiteral<Int32>") {
        const auto &numLit = static_pointer_cast<NumericLiteral<arrow::Int32Type>>(literal);
        const auto &value = numLit->value();
        if (value.has_value()) {
          return Scalar::make(arrow::MakeScalar(*value));
        } else {
          return nullptr;
        }
      } else if (literal->getTypeString() == "NumericLiteral<Int64>") {
        const auto &numLit = static_pointer_cast<NumericLiteral<arrow::Int64Type>>(literal);
        const auto &value = numLit->value();
        if (value.has_value()) {
          return Scalar::make(arrow::MakeScalar(*value));
        } else {
          return nullptr;
        }
      } if (literal->getTypeString() == "NumericLiteral<Double>") {
        const auto &numLit = static_pointer_cast<NumericLiteral<arrow::DoubleType>>(literal);
        const auto &value = numLit->value();
        if (value.has_value()) {
          return Scalar::make(arrow::MakeScalar(*value));
        } else {
          return nullptr;
        }
      } else {
        return nullptr;
      }
    }
    default: {
      throw runtime_error(fmt::format("Expr is not a literal, but {}", literal->getTypeString()));
    }
  }
}

bool PartitionPruner::checkValid(const shared_ptr<Scalar> &predMin, const shared_ptr<Scalar> &predMax,
                                 bool predMinOpen, bool predMaxOpen,
                                 const shared_ptr<Scalar> &statsMin, const shared_ptr<Scalar> &statsMax) {
  if (predMin) {
    if (predMin->type()->id() != statsMax->type()->id()) {
      throw runtime_error(fmt::format("Inconsistent type when pruning partition using max stats, predicate: {}, but stats: {}",
                                      predMin->type()->name(), statsMax->type()->name()));
    }
    if (predMinOpen) {
      if (lte(statsMax, predMin)) {
        return false;
      }
    } else {
      if (lt(statsMax, predMin)) {
        return false;
      }
    }
  }
  
  if (predMax) {
    if (predMax->type()->id() != statsMin->type()->id()) {
      throw runtime_error(fmt::format("Inconsistent type when pruning partition using min stats, predicate: {}, but stats: {}",
                                      predMax->type()->name(), statsMin->type()->name()));
    }
    if (predMaxOpen) {
      if (lte(predMax, statsMin)) {
        return false;
      }
    } else {
      if (lt(predMax, statsMin)) {
        return false;
      }
    }
  }
  
  return true;
}

bool PartitionPruner::lt(const shared_ptr<Scalar> &v1, const shared_ptr<Scalar> &v2) {
  if (v1->type()->id() == arrow::int32()->id()) {
    auto expV1 = v1->value<int>();
    auto expV2 = v2->value<int>();
    if (!expV1.has_value() || !expV2.has_value()) {
      throw runtime_error(expV1.has_value() ? expV2.error() : expV1.error());
    }
    return *expV1 < *expV2;
  } else if (v1->type()->id() == arrow::int64()->id()) {
    auto expV1 = v1->value<long>();
    auto expV2 = v2->value<long>();
    if (!expV1.has_value() || !expV2.has_value()) {
      throw runtime_error(expV1.has_value() ? expV2.error() : expV1.error());
    }
    return *expV1 < *expV2;
  } else if (v1->type()->id() == arrow::float64()->id()) {
    auto expV1 = v1->value<double>();
    auto expV2 = v2->value<double>();
    if (!expV1.has_value() || !expV2.has_value()) {
      throw runtime_error(expV1.has_value() ? expV2.error() : expV1.error());
    }
    return *expV1 < *expV2;
  } else if (v1->type()->id() == arrow::utf8()->id()) {
    auto expV1 = v1->value<string>();
    auto expV2 = v2->value<string>();
    if (!expV1.has_value() || !expV2.has_value()) {
      throw runtime_error(expV1.has_value() ? expV2.error() : expV1.error());
    }
    return *expV1 < *expV2;
  } else {
    throw runtime_error(fmt::format("Unsupported data type: {}", v1->type()->name()));
  }
}

bool PartitionPruner::lte(const shared_ptr<Scalar> &v1, const shared_ptr<Scalar> &v2) {
  if (v1->type()->id() == arrow::int32()->id()) {
    auto expV1 = v1->value<int>();
    auto expV2 = v2->value<int>();
    if (!expV1.has_value() || !expV2.has_value()) {
      throw runtime_error(expV1.has_value() ? expV2.error() : expV1.error());
    }
    return *expV1 <= *expV2;
  } else if (v1->type()->id() == arrow::int64()->id()) {
    auto expV1 = v1->value<long>();
    auto expV2 = v2->value<long>();
    if (!expV1.has_value() || !expV2.has_value()) {
      throw runtime_error(expV1.has_value() ? expV2.error() : expV1.error());
    }
    return *expV1 <= *expV2;
  } else if (v1->type()->id() == arrow::float64()->id()) {
    auto expV1 = v1->value<double>();
    auto expV2 = v2->value<double>();
    if (!expV1.has_value() || !expV2.has_value()) {
      throw runtime_error(expV1.has_value() ? expV2.error() : expV1.error());
    }
    return *expV1 <= *expV2;
  } else if (v1->type()->id() == arrow::utf8()->id()) {
    auto expV1 = v1->value<string>();
    auto expV2 = v2->value<string>();
    if (!expV1.has_value() || !expV2.has_value()) {
      throw runtime_error(expV1.has_value() ? expV2.error() : expV1.error());
    }
    return *expV1 <= *expV2;
  } else {
    throw runtime_error(fmt::format("Unsupported data type: {}", v1->type()->name()));
  }
}

}

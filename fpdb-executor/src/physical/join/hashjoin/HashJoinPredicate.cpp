//
// Created by matt on 29/4/20.
//

#include <fpdb/executor/physical/join/hashjoin/HashJoinPredicate.h>
#include <fmt/format.h>
#include <utility>
#include <sstream>
#include <cassert>

using namespace fpdb::executor::physical::join;

HashJoinPredicate::HashJoinPredicate(vector<string> leftColumnNames,
                                     vector<string> rightColumnNames) :
	leftColumnNames_(move(leftColumnNames)),
	rightColumnNames_(move(rightColumnNames)) {
  assert(leftColumnNames_.size() == rightColumnNames_.size());
}

const vector<string> &HashJoinPredicate::getLeftColumnNames() const {
  return leftColumnNames_;
}

const vector<string> &HashJoinPredicate::getRightColumnNames() const {
  return rightColumnNames_;
}

string HashJoinPredicate::toString() const {
  stringstream ss;
  for (uint i = 0; i < leftColumnNames_.size(); ++i) {
    ss << leftColumnNames_[i];
    ss << "-";
    ss << rightColumnNames_[i];
    if (i < leftColumnNames_.size() - 1) {
      ss << "-";
    }
  }
  return ss.str();
}

::nlohmann::json HashJoinPredicate::toJson() const {
  ::nlohmann::json jObj;
  jObj.emplace("leftColumnNames", leftColumnNames_);
  jObj.emplace("rightColumnNames", rightColumnNames_);
  return jObj;
}

tl::expected<HashJoinPredicate, std::string> HashJoinPredicate::fromJson(const nlohmann::json &jObj) {
  if (!jObj.contains("leftColumnNames")) {
    return tl::make_unexpected(fmt::format("LeftColumnNames not specified in HashJoinPredicate JSON '{}'", to_string(jObj)));
  }
  auto leftColumnNames = jObj["leftColumnNames"].get<vector<string>>();

  if (!jObj.contains("rightColumnNames")) {
    return tl::make_unexpected(fmt::format("RightColumnNames not specified in HashJoinPredicate JSON '{}'", to_string(jObj)));
  }
  auto rightColumnNames = jObj["rightColumnNames"].get<vector<string>>();

  return HashJoinPredicate(leftColumnNames, rightColumnNames);
}

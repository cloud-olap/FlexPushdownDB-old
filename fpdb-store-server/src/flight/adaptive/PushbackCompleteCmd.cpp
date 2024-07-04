//
// Created by Yifei Yang on 9/14/23.
//

#include "fpdb/store/server/flight/adaptive/PushbackCompleteCmd.hpp"
#include "fpdb/store/server/flight/Util.hpp"
#include "fmt/format.h"

namespace fpdb::store::server::flight {

PushbackCompleteCmd::PushbackCompleteCmd(long query_id, std::string op):
  CmdObject(CmdType::pushback_complete()),
  query_id_(query_id), op_(op) {}

std::shared_ptr<PushbackCompleteCmd> PushbackCompleteCmd::make(long query_id, std::string op) {
  return std::make_shared<PushbackCompleteCmd>(query_id, op);
}

long PushbackCompleteCmd::query_id() const {
  return query_id_;
}

const std::string &PushbackCompleteCmd::op() const {
  return op_;
}

tl::expected<std::string, std::string> PushbackCompleteCmd::serialize(bool pretty) {
  nlohmann::json value;
  value.emplace(TypeJSONName.data(), type()->name());
  value.emplace(QueryIdJSONName.data(), query_id_);
  value.emplace(OpJSONName.data(), op_);
  return value.dump(pretty ? 2 : -1);
}

tl::expected<std::shared_ptr<PushbackCompleteCmd>, std::string>
PushbackCompleteCmd::from_json(const nlohmann::json& jObj) {
  if (!jObj.contains(QueryIdJSONName.data())) {
    return tl::make_unexpected(fmt::format("query_id_ not specified in PushbackCompleteCmd JSON '{}'", to_string(jObj)));
  }
  auto query_id = jObj[QueryIdJSONName.data()].get<long>();

  if (!jObj.contains(OpJSONName.data())) {
    return tl::make_unexpected(fmt::format("op_ not specified in PushbackCompleteCmd JSON '{}'", to_string(jObj)));
  }
  auto op = jObj[OpJSONName.data()].get<std::string>();

  return make(query_id, op);
}

}

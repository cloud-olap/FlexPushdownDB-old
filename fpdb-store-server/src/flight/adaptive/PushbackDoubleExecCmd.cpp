//
// Created by Yifei Yang on 9/25/23.
//

#include "fpdb/store/server/flight/adaptive/PushbackDoubleExecCmd.hpp"
#include "fpdb/store/server/flight/Util.hpp"
#include "fmt/format.h"

namespace fpdb::store::server::flight {

PushbackDoubleExecCmd::PushbackDoubleExecCmd(long query_id, std::string op):
  CmdObject(CmdType::pushback_double_exec()),
  query_id_(query_id), op_(op) {}

std::shared_ptr<PushbackDoubleExecCmd> PushbackDoubleExecCmd::make(long query_id, std::string op) {
  return std::make_shared<PushbackDoubleExecCmd>(query_id, op);
}

long PushbackDoubleExecCmd::query_id() const {
  return query_id_;
}

const std::string &PushbackDoubleExecCmd::op() const {
  return op_;
}

tl::expected<std::string, std::string> PushbackDoubleExecCmd::serialize(bool pretty) {
  nlohmann::json value;
  value.emplace(TypeJSONName.data(), type()->name());
  value.emplace(QueryIdJSONName.data(), query_id_);
  value.emplace(OpJSONName.data(), op_);
  return value.dump(pretty ? 2 : -1);
}

tl::expected<std::shared_ptr<PushbackDoubleExecCmd>, std::string>
PushbackDoubleExecCmd::from_json(const nlohmann::json& jObj) {
  if (!jObj.contains(QueryIdJSONName.data())) {
    return tl::make_unexpected(fmt::format("query_id_ not specified in PushbackDoubleExecCmd JSON '{}'", to_string(jObj)));
  }
  auto query_id = jObj[QueryIdJSONName.data()].get<long>();

  if (!jObj.contains(OpJSONName.data())) {
    return tl::make_unexpected(fmt::format("op_ not specified in PushbackDoubleExecCmd JSON '{}'", to_string(jObj)));
  }
  auto op = jObj[OpJSONName.data()].get<std::string>();

  return make(query_id, op);
}
  
}

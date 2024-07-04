//
// Created by Yifei Yang on 12/16/22.
//

#include "fpdb/store/server/flight/adaptive/SetAdaptPushdownCmd.hpp"
#include "fpdb/store/server/flight/Util.hpp"
#include "fmt/format.h"

namespace fpdb::store::server::flight {

SetAdaptPushdownCmd::SetAdaptPushdownCmd(bool enableAdaptPushdown, int maxThreads):
  CmdObject(CmdType::set_adapt_pushdown()),
  enableAdaptPushdown_(enableAdaptPushdown),
  maxThreads_(maxThreads) {}

std::shared_ptr<SetAdaptPushdownCmd> SetAdaptPushdownCmd::make(bool enableAdaptPushdown, int maxThreads) {
  return std::make_shared<SetAdaptPushdownCmd>(enableAdaptPushdown, maxThreads);
}

bool SetAdaptPushdownCmd::enableAdaptPushdown() const {
  return enableAdaptPushdown_;
}

int SetAdaptPushdownCmd::maxThreads() const {
  return maxThreads_;
}

tl::expected<std::string, std::string> SetAdaptPushdownCmd::serialize(bool pretty) {
  nlohmann::json value;
  value.emplace(TypeJSONName.data(), type()->name());
  value.emplace(EnableAdaptPushdownJSONName.data(), enableAdaptPushdown_);
  value.emplace(MaxThreadsJSONName.data(), maxThreads_);
  return value.dump(pretty ? 2 : -1);
}

tl::expected<std::shared_ptr<SetAdaptPushdownCmd>, std::string>
SetAdaptPushdownCmd::from_json(const nlohmann::json &jObj) {
  if (!jObj.contains(EnableAdaptPushdownJSONName.data())) {
    return tl::make_unexpected(fmt::format("enableAdaptPushdown not specified in SetAdaptPushdownCmd JSON '{}'", to_string(jObj)));
  }
  auto enableAdaptPushdown = jObj[EnableAdaptPushdownJSONName.data()].get<bool>();

  if (!jObj.contains(MaxThreadsJSONName.data())) {
    return tl::make_unexpected(fmt::format("availCpuPercent not specified in SetAdaptPushdownCmd JSON '{}'", to_string(jObj)));
  }
  auto maxThreads = jObj[MaxThreadsJSONName.data()].get<int>();

  return SetAdaptPushdownCmd::make(enableAdaptPushdown, maxThreads);
}

}

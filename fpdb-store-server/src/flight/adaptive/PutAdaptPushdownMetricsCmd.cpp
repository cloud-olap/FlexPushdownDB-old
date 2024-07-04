//
// Created by Yifei Yang on 10/31/22.
//

#include "fpdb/store/server/flight/adaptive/PutAdaptPushdownMetricsCmd.hpp"
#include "fpdb/store/server/flight/Util.hpp"
#include "fmt/format.h"

namespace fpdb::store::server::flight {

PutAdaptPushdownMetricsCmd::PutAdaptPushdownMetricsCmd(
        const std::unordered_map<std::string, int64_t> &adaptPushdownMetrics):
  CmdObject(CmdType::put_adapt_pushdown_metrics()),
  adaptPushdownMetrics_(adaptPushdownMetrics) {}

std::shared_ptr<PutAdaptPushdownMetricsCmd> PutAdaptPushdownMetricsCmd::make(
        const std::unordered_map<std::string, int64_t> &adaptPushdownMetrics) {
  return std::make_shared<PutAdaptPushdownMetricsCmd>(adaptPushdownMetrics);
}

const std::unordered_map<std::string, int64_t> &PutAdaptPushdownMetricsCmd::getAdaptPushdownMetrics() const {
  return adaptPushdownMetrics_;
}

tl::expected<std::string, std::string> PutAdaptPushdownMetricsCmd::serialize(bool pretty) {
  nlohmann::json value;
  value.emplace(TypeJSONName.data(), type()->name());
  value.emplace(AdaptPushdownMetricsJSONName.data(), adaptPushdownMetrics_);
  return value.dump(pretty ? 2 : -1);
}

tl::expected<std::shared_ptr<PutAdaptPushdownMetricsCmd>, std::string>
PutAdaptPushdownMetricsCmd::from_json(const nlohmann::json& jObj) {
  if (!jObj.contains(AdaptPushdownMetricsJSONName.data())) {
    return tl::make_unexpected(fmt::format("AdaptPushdownMetrics not specified in PutAdaptPushdownMetricsCmd JSON '{}'", to_string(jObj)));
  }
  auto adaptPushdownMetrics = jObj[AdaptPushdownMetricsJSONName.data()].get<std::unordered_map<std::string, int64_t>>();

  return PutAdaptPushdownMetricsCmd::make(adaptPushdownMetrics);
}

}

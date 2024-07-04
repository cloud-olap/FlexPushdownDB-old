//
// Created by Yifei Yang on 12/15/22.
//

#include "fpdb/store/server/flight/adaptive/ClearAdaptPushdownMetricsCmd.hpp"
#include "fpdb/store/server/flight/Util.hpp"
#include "fmt/format.h"

namespace fpdb::store::server::flight {

ClearAdaptPushdownMetricsCmd::ClearAdaptPushdownMetricsCmd():
  CmdObject(CmdType::clear_adapt_pushdown_metrics()) {}

std::shared_ptr<ClearAdaptPushdownMetricsCmd> ClearAdaptPushdownMetricsCmd::make() {
  return std::make_shared<ClearAdaptPushdownMetricsCmd>();
}

tl::expected<std::string, std::string> ClearAdaptPushdownMetricsCmd::serialize(bool pretty) {
  nlohmann::json value;
  value.emplace(TypeJSONName.data(), type()->name());
  return value.dump(pretty ? 2 : -1);
}

tl::expected<std::shared_ptr<ClearAdaptPushdownMetricsCmd>, std::string>
ClearAdaptPushdownMetricsCmd::from_json(const nlohmann::json&) {
  return ClearAdaptPushdownMetricsCmd::make();
}

}

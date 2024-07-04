//
// Created by Yifei Yang on 12/15/22.
//

#ifndef FPDB_FPDB_STORE_SERVER_INCLUDE_FPDB_STORE_SERVER_FLIGHT_ADAPTIVE_CLEARADAPTPUSHDOWNMETRICSCMD_HPP
#define FPDB_FPDB_STORE_SERVER_INCLUDE_FPDB_STORE_SERVER_FLIGHT_ADAPTIVE_CLEARADAPTPUSHDOWNMETRICSCMD_HPP

#include "fpdb/store/server/flight/CmdObject.hpp"

namespace fpdb::store::server::flight {

class ClearAdaptPushdownMetricsCmd : public CmdObject {

public:
  ClearAdaptPushdownMetricsCmd();

  static std::shared_ptr<ClearAdaptPushdownMetricsCmd> make();

  tl::expected<std::string, std::string> serialize(bool pretty) override;
  static tl::expected<std::shared_ptr<ClearAdaptPushdownMetricsCmd>, std::string> from_json(const nlohmann::json& jObj);
};

}


#endif //FPDB_FPDB_STORE_SERVER_INCLUDE_FPDB_STORE_SERVER_FLIGHT_ADAPTIVE_CLEARADAPTPUSHDOWNMETRICSCMD_HPP

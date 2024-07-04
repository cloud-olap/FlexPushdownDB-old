
//
// Created by Yifei Yang on 10/31/22.
//

#ifndef FPDB_FPDB_STORE_SERVER_INCLUDE_FPDB_STORE_SERVER_FLIGHT_ADAPTIVE_PUTADAPTPUSHDOWNMETRICSCMD_HPP
#define FPDB_FPDB_STORE_SERVER_INCLUDE_FPDB_STORE_SERVER_FLIGHT_ADAPTIVE_PUTADAPTPUSHDOWNMETRICSCMD_HPP

#include "fpdb/store/server/flight/CmdObject.hpp"
#include "fpdb/store/server/flight/BitmapType.h"

namespace fpdb::store::server::flight {

class PutAdaptPushdownMetricsCmd : public CmdObject {

public:
  PutAdaptPushdownMetricsCmd(const std::unordered_map<std::string, int64_t> &adaptPushdownMetrics);

  static std::shared_ptr<PutAdaptPushdownMetricsCmd> make(
          const std::unordered_map<std::string, int64_t> &adaptPushdownMetrics);

  const std::unordered_map<std::string, int64_t> &getAdaptPushdownMetrics() const;

  tl::expected<std::string, std::string> serialize(bool pretty) override;
  static tl::expected<std::shared_ptr<PutAdaptPushdownMetricsCmd>, std::string> from_json(const nlohmann::json& jObj);

private:
  std::unordered_map<std::string, int64_t> adaptPushdownMetrics_;
};

}


#endif //FPDB_FPDB_STORE_SERVER_INCLUDE_FPDB_STORE_SERVER_FLIGHT_ADAPTIVE_PUTADAPTPUSHDOWNMETRICSCMD_HPP

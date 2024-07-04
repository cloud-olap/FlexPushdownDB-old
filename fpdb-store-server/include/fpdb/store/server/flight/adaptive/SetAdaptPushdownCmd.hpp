//
// Created by Yifei Yang on 12/16/22.
//

#ifndef FPDB_FPDB_STORE_SERVER_INCLUDE_FPDB_STORE_SERVER_FLIGHT_ADAPTIVE_SETADAPTPUSHDOWNCMD_HPP
#define FPDB_FPDB_STORE_SERVER_INCLUDE_FPDB_STORE_SERVER_FLIGHT_ADAPTIVE_SETADAPTPUSHDOWNCMD_HPP

#include "fpdb/store/server/flight/CmdObject.hpp"

namespace fpdb::store::server::flight {

class SetAdaptPushdownCmd : public CmdObject {

public:
  SetAdaptPushdownCmd(bool enableAdaptPushdown, int maxThreads);

  static std::shared_ptr<SetAdaptPushdownCmd> make(bool enableAdaptPushdown, int maxThreads);

  bool enableAdaptPushdown() const;
  int maxThreads() const;

  tl::expected<std::string, std::string> serialize(bool pretty) override;
  static tl::expected<std::shared_ptr<SetAdaptPushdownCmd>, std::string> from_json(const nlohmann::json& jObj);

private:
  bool enableAdaptPushdown_;
  int maxThreads_;
};

}


#endif //FPDB_FPDB_STORE_SERVER_INCLUDE_FPDB_STORE_SERVER_FLIGHT_ADAPTIVE_SETADAPTPUSHDOWNCMD_HPP

//
// Created by Yifei Yang on 9/14/23.
//

#ifndef FPDB_FPDB_STORE_SERVER_INCLUDE_FPDB_STORE_SERVER_FLIGHT_ADAPTIVE_PUSHBACKCOMPLETECMD_HPP
#define FPDB_FPDB_STORE_SERVER_INCLUDE_FPDB_STORE_SERVER_FLIGHT_ADAPTIVE_PUSHBACKCOMPLETECMD_HPP

#include "fpdb/store/server/flight/CmdObject.hpp"

namespace fpdb::store::server::flight {

/**
 * Used by AdaptPushdownManager2 to signal the complete of a pushback req
 */
class PushbackCompleteCmd : public CmdObject {

public:
  PushbackCompleteCmd(long query_id, std::string op);

  static std::shared_ptr<PushbackCompleteCmd> make(long query_id, std::string op);

  long query_id() const;
  const std::string &op() const;

  tl::expected<std::string, std::string> serialize(bool pretty) override;
  static tl::expected<std::shared_ptr<PushbackCompleteCmd>, std::string> from_json(const nlohmann::json& jObj);

private:
  long query_id_;
  std::string op_;
};

}

#endif //FPDB_FPDB_STORE_SERVER_INCLUDE_FPDB_STORE_SERVER_FLIGHT_ADAPTIVE_PUSHBACKCOMPLETECMD_HPP

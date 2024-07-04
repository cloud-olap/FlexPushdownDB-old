//
// Created by Yifei Yang on 9/25/23.
//

#ifndef FPDB_FPDB_STORE_SERVER_INCLUDE_FPDB_STORE_SERVER_FLIGHT_ADAPTIVE_REQEXTRAINFO_HPP
#define FPDB_FPDB_STORE_SERVER_INCLUDE_FPDB_STORE_SERVER_FLIGHT_ADAPTIVE_REQEXTRAINFO_HPP

#include "fpdb/store/server/flight/Util.hpp"
#include "nlohmann/json.hpp"
#include "fmt/format.h"
#include "string"

namespace fpdb::store::server::flight {

/**
 * More information of arriving pushdown requests, e.g. sender ip and port, whether this is for double-exec, etc
 */
struct ReqExtraInfo {
  ReqExtraInfo(bool isDoubleExec = false,
               const std::string &senderIp = "",
               int senderPort = 0):
    isDoubleExec_(isDoubleExec), senderIp_(senderIp), senderPort_(senderPort) {}

  nlohmann::json to_json() {
    nlohmann::json jObj;
    jObj.emplace(IsDoubleExecJSONName.data(), isDoubleExec_);
    jObj.emplace(SenderIpJSONName.data(), senderIp_);
    jObj.emplace(SenderPortJSONName.data(), senderPort_);
    return jObj;
  }

  static tl::expected<ReqExtraInfo, std::string> from_json(const nlohmann::json &jObj) {
    if (!jObj.contains(IsDoubleExecJSONName.data())) {
      return tl::make_unexpected(fmt::format("isDoubleExec_ not specified in ReqExtraInfo JSON '{}'", to_string(jObj)));
    }
    auto isDoubleExec = jObj[IsDoubleExecJSONName.data()].get<bool>();

    if (!jObj.contains(SenderIpJSONName.data())) {
      return tl::make_unexpected(fmt::format("senderIp_ not specified in ReqExtraInfo JSON '{}'", to_string(jObj)));
    }
    auto senderIp = jObj[SenderIpJSONName.data()].get<std::string>();

    if (!jObj.contains(SenderPortJSONName.data())) {
      return tl::make_unexpected(fmt::format("senderPort_ not specified in ReqExtraInfo JSON '{}'", to_string(jObj)));
    }
    auto senderPort = jObj[SenderPortJSONName.data()].get<int>();

    return ReqExtraInfo(isDoubleExec, senderIp, senderPort);
  }

  // below are parameters set from the compute side
  bool isDoubleExec_;   // means this request is a double-exec fork
  std::string senderIp_;
  int senderPort_;

  // below are parameters set from the storage side
  bool isDoubleExecFrom_ = false;   // means this requests has a double-exec fork
  int64_t pa_ = 0;          // pushdown amenability, used by adapt_pushdown_manager of PA version
};

}

#endif //FPDB_FPDB_STORE_SERVER_INCLUDE_FPDB_STORE_SERVER_FLIGHT_ADAPTIVE_REQEXTRAINFO_HPP

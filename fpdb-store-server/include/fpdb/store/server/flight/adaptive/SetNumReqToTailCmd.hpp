//
// Created by Yifei Yang on 9/28/23.
//

#ifndef FPDB_FPDB_STORE_SERVER_INCLUDE_FPDB_STORE_SERVER_FLIGHT_ADAPTIVE_SETNUMREQTOTAILCMD_HPP
#define FPDB_FPDB_STORE_SERVER_INCLUDE_FPDB_STORE_SERVER_FLIGHT_ADAPTIVE_SETNUMREQTOTAILCMD_HPP

#include "fpdb/store/server/flight/CmdObject.hpp"

namespace fpdb::store::server::flight {

class SetNumReqToTailCmd : public CmdObject {
public:
  SetNumReqToTailCmd(int64_t numReqToTail);

  static std::shared_ptr<SetNumReqToTailCmd> make(int64_t numReqToTail);

  int64_t numReqToTail() const;

  tl::expected<std::string, std::string> serialize(bool pretty) override;
  static tl::expected<std::shared_ptr<SetNumReqToTailCmd>, std::string> from_json(const nlohmann::json& jObj);

private:
  int64_t numReqToTail_;
};

}

#endif //FPDB_FPDB_STORE_SERVER_INCLUDE_FPDB_STORE_SERVER_FLIGHT_ADAPTIVE_SETNUMREQTOTAILCMD_HPP

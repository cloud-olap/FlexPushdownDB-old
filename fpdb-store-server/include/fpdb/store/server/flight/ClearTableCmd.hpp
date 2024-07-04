//
// Created by Yifei Yang on 9/25/23.
//

#ifndef FPDB_FPDB_STORE_SERVER_INCLUDE_FPDB_STORE_SERVER_FLIGHT_CLEARTABLECMD_HPP
#define FPDB_FPDB_STORE_SERVER_INCLUDE_FPDB_STORE_SERVER_FLIGHT_CLEARTABLECMD_HPP

#include "fpdb/store/server/flight/CmdObject.hpp"

namespace fpdb::store::server::flight {
  
class ClearTableCmd : public CmdObject {
public:
  ClearTableCmd(long query_id,
                const std::string &producer,
                const std::string &consumer);

  static std::shared_ptr<ClearTableCmd> make(long query_id,
                                             const std::string &producer,
                                             const std::string &consumer);

  long query_id() const;
  const std::string& producer() const;
  const std::string& consumer() const;

  tl::expected<std::string, std::string> serialize(bool pretty) override;
  static tl::expected<std::shared_ptr<ClearTableCmd>, std::string> from_json(const nlohmann::json& jObj);

private:
  long query_id_;
  std::string producer_;
  std::string consumer_;
};

}

#endif //FPDB_FPDB_STORE_SERVER_INCLUDE_FPDB_STORE_SERVER_FLIGHT_CLEARTABLECMD_HPP

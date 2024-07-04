//
// Created by Yifei Yang on 4/18/22.
//

#ifndef FPDB_FPDB_STORE_SERVER_INCLUDE_FPDB_STORE_SERVER_FLIGHT_CLEARBITMAPCMD_HPP
#define FPDB_FPDB_STORE_SERVER_INCLUDE_FPDB_STORE_SERVER_FLIGHT_CLEARBITMAPCMD_HPP

#include "fpdb/store/server/flight/CmdObject.hpp"
#include "fpdb/store/server/flight/BitmapType.h"

namespace fpdb::store::server::flight {

class ClearBitmapCmd : public CmdObject {

public:
  ClearBitmapCmd(BitmapType bitmap_type, long query_id, const std::string &op);

  static std::shared_ptr<ClearBitmapCmd> make(BitmapType bitmap_type, long query_id, const std::string &op);

  BitmapType bitmap_type() const;
  long query_id() const;
  const std::string& op() const;

  tl::expected<std::string, std::string> serialize(bool pretty) override;
  static tl::expected<std::shared_ptr<ClearBitmapCmd>, std::string> from_json(const nlohmann::json& jObj);

private:
  BitmapType bitmap_type_;
  long query_id_;
  std::string op_;

};

}


#endif //FPDB_FPDB_STORE_SERVER_INCLUDE_FPDB_STORE_SERVER_FLIGHT_CLEARBITMAPCMD_HPP

//
// Created by Yifei Yang on 9/28/23.
//

#include "fpdb/store/server/flight/adaptive/SetNumReqToTailCmd.hpp"
#include "fpdb/store/server/flight/Util.hpp"
#include "fmt/format.h"

namespace fpdb::store::server::flight {

SetNumReqToTailCmd::SetNumReqToTailCmd(int64_t numReqToTail):
  CmdObject(CmdType::set_num_req_to_tail()),
  numReqToTail_(numReqToTail) {}

std::shared_ptr<SetNumReqToTailCmd> SetNumReqToTailCmd::make(int64_t numReqToTail) {
  return std::make_shared<SetNumReqToTailCmd>(numReqToTail);
}

int64_t SetNumReqToTailCmd::numReqToTail() const {
  return numReqToTail_;
}

tl::expected<std::string, std::string> SetNumReqToTailCmd::serialize(bool pretty) {
  nlohmann::json value;
  value.emplace(TypeJSONName.data(), type()->name());
  value.emplace(NumReqToTailJSONName.data(), numReqToTail_);
  return value.dump(pretty ? 2 : -1);
}

tl::expected<std::shared_ptr<SetNumReqToTailCmd>, std::string>
SetNumReqToTailCmd::from_json(const nlohmann::json &jObj) {
  if (!jObj.contains(NumReqToTailJSONName.data())) {
    return tl::make_unexpected(fmt::format("numReqToTail_ not specified in SetNumReqToTailCmd JSON '{}'", to_string(jObj)));
  }
  auto numReqToTail = jObj[NumReqToTailJSONName.data()].get<int64_t>();

  return SetNumReqToTailCmd::make(numReqToTail);
}
  
}

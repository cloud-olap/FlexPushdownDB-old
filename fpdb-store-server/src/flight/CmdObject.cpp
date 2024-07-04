//
// Created by matt on 4/2/22.
//

#include "fpdb/store/server/flight/CmdObject.hpp"
#include "fpdb/store/server/flight/SelectObjectContentCmd.hpp"
#include "fpdb/store/server/flight/PutBitmapCmd.hpp"
#include "fpdb/store/server/flight/ClearBitmapCmd.hpp"
#include "fpdb/store/server/flight/ClearTableCmd.hpp"
#include "fpdb/store/server/flight/adaptive/PutAdaptPushdownMetricsCmd.hpp"
#include "fpdb/store/server/flight/adaptive/ClearAdaptPushdownMetricsCmd.hpp"
#include "fpdb/store/server/flight/adaptive/SetAdaptPushdownCmd.hpp"
#include "fpdb/store/server/flight/adaptive/PushbackCompleteCmd.hpp"
#include "fpdb/store/server/flight/adaptive/PushbackDoubleExecCmd.hpp"
#include "fpdb/store/server/flight/adaptive/SetNumReqToTailCmd.hpp"
#include "fpdb/store/server/flight/Util.hpp"
#include <fmt/format.h>

namespace fpdb::store::server::flight {

CmdObject::CmdObject(std::shared_ptr<CmdType> type) : type_(std::move(type)) {}

tl::expected<std::shared_ptr<CmdObject>, std::string> CmdObject::deserialize(const std::string& cmd_string) {
  auto document = nlohmann::json::parse(cmd_string);
  if (!document.is_object()) {
    return tl::make_unexpected(fmt::format("Cannot parse CmdObject JSON '{}'", cmd_string));
  }

  if (!document.contains(TypeJSONName.data())) {
    return tl::make_unexpected(fmt::format("Type not specified in CmdObject JSON '{}'", cmd_string));
  }
  auto type = document[TypeJSONName.data()].get<std::string>();

  if (type == SelectObjectContentCmdTypeName.data()) {
    return SelectObjectContentCmd::from_json(document);
  } else if (type == PutBitmapCmdTypeName.data()) {
    return PutBitmapCmd::from_json(document);
  } else if (type == ClearBitmapCmdTypeName.data()) {
    return ClearBitmapCmd::from_json(document);
  } else if (type == ClearTableCmdTypeName.data()) {
    return ClearTableCmd::from_json(document);
  } else if (type == PutAdaptPushdownMetricsCmdTypeName.data()) {
    return PutAdaptPushdownMetricsCmd::from_json(document);
  } else if (type == ClearAdaptPushdownMetricsCmdTypeName.data()) {
    return ClearAdaptPushdownMetricsCmd::from_json(document);
  } else if (type == SetAdaptPushdownCmdTypeName.data()) {
    return SetAdaptPushdownCmd::from_json(document);
  } else if (type == PushbackCompleteCmdTypeName.data()) {
    return PushbackCompleteCmd::from_json(document);
  } else if (type == PushbackDoubleExecCmdTypeName.data()) {
    return PushbackDoubleExecCmd::from_json(document);
  } else if (type == SetNumReqToTailCmdTypeName.data()) {
    return SetNumReqToTailCmd::from_json(document);
  } else {
    return tl::make_unexpected(fmt::format("Unsupported cmd object type: '{}'", type));
  }
}

const std::shared_ptr<CmdType>& CmdObject::type() const {
  return type_;
}

} // namespace fpdb::store::server::flight
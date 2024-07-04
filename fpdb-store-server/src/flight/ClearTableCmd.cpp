//
// Created by Yifei Yang on 9/25/23.
//

#include "fpdb/store/server/flight/ClearTableCmd.hpp"
#include "fpdb/store/server/flight/Util.hpp"
#include "fmt/format.h"

namespace fpdb::store::server::flight {

ClearTableCmd::ClearTableCmd(long query_id,
                             const std::string &producer,
                             const std::string &consumer):
  CmdObject(CmdType::clear_table()),
  query_id_(query_id),
  producer_(producer),
  consumer_(consumer) {}

std::shared_ptr<ClearTableCmd> ClearTableCmd::make(long query_id,
                                                   const std::string &producer,
                                                   const std::string &consumer) {
  return std::make_shared<ClearTableCmd>(query_id, producer, consumer);
}

long ClearTableCmd::query_id() const {
  return query_id_;
}

const std::string& ClearTableCmd::producer() const {
  return producer_;
}

const std::string& ClearTableCmd::consumer() const {
  return consumer_;
}

tl::expected<std::string, std::string> ClearTableCmd::serialize(bool pretty) {
  nlohmann::json value;
  value.emplace(TypeJSONName.data(), type()->name());
  value.emplace(QueryIdJSONName.data(), query_id_);
  value.emplace(ProducerJSONName.data(), producer_);
  value.emplace(ConsumerJSONName, consumer_);
  return value.dump(pretty ? 2 : -1);
}

tl::expected<std::shared_ptr<ClearTableCmd>, std::string> ClearTableCmd::from_json(const nlohmann::json& jObj) {
  if (!jObj.contains(QueryIdJSONName.data())) {
    return tl::make_unexpected(fmt::format("query_id_ not specified in ClearTableCmd JSON '{}'", to_string(jObj)));
  }
  auto query_id = jObj[QueryIdJSONName.data()].get<long>();

  if (!jObj.contains(ProducerJSONName.data())) {
    return tl::make_unexpected(fmt::format("producer_ not specified in ClearTableCmd JSON '{}'", to_string(jObj)));
  }
  auto producer = jObj[ProducerJSONName.data()].get<std::string>();

  if (!jObj.contains(ConsumerJSONName.data())) {
    return tl::make_unexpected(fmt::format("consumer_ not specified in ClearTableCmd JSON '{}'", to_string(jObj)));
  }
  auto consumer = jObj[ConsumerJSONName.data()].get<std::string>();

  return ClearTableCmd::make(query_id, producer, consumer);
}
  
}

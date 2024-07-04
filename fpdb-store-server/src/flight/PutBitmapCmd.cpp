//
// Created by Yifei Yang on 4/17/22.
//

#include "fpdb/store/server/flight/PutBitmapCmd.hpp"
#include "fpdb/store/server/flight/Util.hpp"
#include "fmt/format.h"

namespace fpdb::store::server::flight {

PutBitmapCmd::PutBitmapCmd(BitmapType bitmap_type, long query_id, const std::string &op, bool valid,
                           const std::optional<int> &num_copies,
                           const std::optional<nlohmann::json> &bloom_filter):
  CmdObject(CmdType::put_bitmap()),
  bitmap_type_(bitmap_type),
  query_id_(query_id),
  op_(op),
  valid_(valid),
  num_copies_(num_copies),
  bloom_filter_(bloom_filter) {}

std::shared_ptr<PutBitmapCmd>
PutBitmapCmd::make(BitmapType bitmap_type, long query_id, const std::string &op, bool valid,
                   const std::optional<int> &num_copies,
                   const std::optional<nlohmann::json> &bloom_filter) {
  return std::make_shared<PutBitmapCmd>(bitmap_type, query_id, op, valid, num_copies, bloom_filter);
}

BitmapType PutBitmapCmd::bitmap_type() const {
  return bitmap_type_;
}

long PutBitmapCmd::query_id() const {
  return query_id_;
}

const std::string& PutBitmapCmd::op() const {
  return op_;
}

bool PutBitmapCmd::valid() const {
  return valid_;
}

const std::optional<int> &PutBitmapCmd::num_copies() const {
  return num_copies_;
}

const std::optional<nlohmann::json> &PutBitmapCmd::bloom_filter() const {
  return bloom_filter_;
}

tl::expected<std::string, std::string> PutBitmapCmd::serialize(bool pretty) {
  nlohmann::json value;
  value.emplace(TypeJSONName.data(), type()->name());
  value.emplace(BitmapTypeJSONName.data(), bitmap_type_);
  value.emplace(QueryIdJSONName.data(), query_id_);
  value.emplace(OpJSONName.data(), op_);
  value.emplace(ValidJSONName.data(), valid_);
  if (num_copies_.has_value()) {
    value.emplace(NumCopiesJSONName.data(), *num_copies_);
  }
  if (bloom_filter_.has_value()) {
    value.emplace(BloomFilterJSONName.data(), *bloom_filter_);
  }
  return value.dump(pretty ? 2 : -1);
}

tl::expected<std::shared_ptr<PutBitmapCmd>, std::string> PutBitmapCmd::from_json(const nlohmann::json& jObj) {
  if (!jObj.contains(BitmapTypeJSONName.data())) {
    return tl::make_unexpected(fmt::format("Bitmap type name not specified in PutBitmapCmd JSON '{}'", to_string(jObj)));
  }
  auto bitmap_type = jObj[BitmapTypeJSONName.data()].get<BitmapType>();

  if (!jObj.contains(QueryIdJSONName.data())) {
    return tl::make_unexpected(fmt::format("Query id not specified in PutBitmapCmd JSON '{}'", to_string(jObj)));
  }
  auto query_id = jObj[QueryIdJSONName.data()].get<long>();

  if (!jObj.contains(OpJSONName.data())) {
    return tl::make_unexpected(fmt::format("Op not specified in PutBitmapCmd JSON '{}'", to_string(jObj)));
  }
  auto op = jObj[OpJSONName.data()].get<std::string>();

  if (!jObj.contains(ValidJSONName.data())) {
    return tl::make_unexpected(fmt::format("Valid not specified in PutBitmapCmd JSON '{}'", to_string(jObj)));
  }
  auto valid = jObj[ValidJSONName.data()].get<bool>();

  std::optional<int> num_copies = std::nullopt;
  if (jObj.contains(NumCopiesJSONName.data())) {
    num_copies = jObj[NumCopiesJSONName.data()].get<int>();
  }
  std::optional<nlohmann::json> bloom_filter = std::nullopt;
  if (jObj.contains(BloomFilterJSONName.data())) {
    bloom_filter = jObj[BloomFilterJSONName.data()].get<nlohmann::json>();
  }

  return PutBitmapCmd::make(bitmap_type, query_id, op, valid, num_copies, bloom_filter);
}

}

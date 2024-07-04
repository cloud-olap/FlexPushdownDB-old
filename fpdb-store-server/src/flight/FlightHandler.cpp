//
// Created by matt on 4/2/22.
//

#include "fpdb/store/server/flight/FlightHandler.hpp"
#include "fpdb/store/server/flight/HeaderMiddlewareFactory.hpp"
#include "fpdb/store/server/flight/GetObjectTicket.hpp"
#include "fpdb/store/server/flight/Util.hpp"
#include "fpdb/executor/FPDBStoreExecution.h"
#include "fpdb/executor/physical/Globals.h"
#include "fpdb/executor/physical/serialization/PhysicalPlanDeserializer.h"
#include "fpdb/executor/physical/bloomfilter/BloomFilterUsePOp.h"
#include "fpdb/executor/physical/filter/FilterPOp.h"
#include "fpdb/executor/caf/CAFInit.h"
#include "fpdb/tuple/serialization/ArrowSerializer.h"
#include "fpdb/tuple/arrow/Arrays.h"
#include "fpdb/tuple/util/Util.h"

using namespace fpdb::executor::physical;
using namespace fpdb::tuple;

namespace fpdb::store::server::flight {

FlightHandler::FlightHandler(Location location,
                             std::shared_ptr<::caf::actor_system> actor_system,
                             std::string store_root_path_prefix,
                             int num_drives) :
  location_(std::move(location)),
  actor_system_(std::move(actor_system)),
  store_root_path_prefix_(std::move(store_root_path_prefix)),
  num_drives_(num_drives) {}

FlightHandler::~FlightHandler() {
  this->shutdown();
  if (ShowDebugMetricsOnExit && !bytes_disk_read_by_query_.empty()) {
    stringstream ss;
    ss << endl;
    ss << "Storage Metrics |" << endl << endl;
    ss << "Bytes Disk Read |" << endl;

    // sort map entries by key
    std::vector<long> keys;
    keys.reserve(bytes_disk_read_by_query_.size());
    for (const auto &metrics: bytes_disk_read_by_query_) {
      keys.emplace_back(metrics.first);
    }
    std::sort(keys.begin(), keys.end());

    // print
    for (auto key: keys) {
      stringstream formattedBytesDiskRead;
      int64_t bytesDiskRead = bytes_disk_read_by_query_[key];
      formattedBytesDiskRead << bytesDiskRead << " B" << " ("
                             << ((double) bytesDiskRead / 1024.0 / 1024.0 / 1024.0) << " GB)";
      ss << left << setw(60) << key;
      ss << left << setw(60) << formattedBytesDiskRead.str();
      ss << endl;
    }
    std::cout << ss.str() << std::endl;
  }
}

tl::expected<void, std::string> FlightHandler::init() {
  FlightServerOptions options(location_);
  options.middleware.emplace_back(HeaderMiddlewareKey, std::make_shared<HeaderMiddlewareFactory>());
  auto st = this->Init(options);
  if(!st.ok()) {
    return tl::make_unexpected(st.message());
  }

  // init vars for bitmap caches
  init_bitmap_cache();

  // init adapt_pushdown_manager
  init_adapt_pushdown_manager();

  // NOTE: This appears to swallow the signal and not allow anything else to handle it
  //  this->SetShutdownOnSignals({SIGTERM});

  // Init CAF objects for executor
  fpdb::executor::caf::CAFInit::initCAFGlobalMetaObjects();

  return {};
}

tl::expected<void, std::string> FlightHandler::serve() {
  auto st = this->Serve();
  if(!st.ok()) {
    return tl::make_unexpected(st.message());
  }
  return {};
}

tl::expected<void, std::string> FlightHandler::shutdown() {
  auto st = this->Shutdown();
  if(!st.ok()) {
    return tl::make_unexpected(st.message());
  }
  return {};
}

tl::expected<void, std::string> FlightHandler::wait() {
  auto st = this->Wait();
  if(!st.ok()) {
    return tl::make_unexpected(st.message());
  }
  return {};
}

int FlightHandler::port() {
  return FlightServerBase::port();
}

::arrow::Status FlightHandler::GetFlightInfo(const ServerCallContext& context, const FlightDescriptor& request,
                                             std::unique_ptr<FlightInfo>* info) {
  auto expected_flight_info = get_flight_info(context, request);
  if(!expected_flight_info.has_value()) {
    return expected_flight_info.error();
  }
  *info = std::move(expected_flight_info.value());
  return ::arrow::Status::OK();
}

::arrow::Status FlightHandler::DoGet(const ServerCallContext& context, const Ticket& request,
                                     std::unique_ptr<FlightDataStream>* stream) {
  auto expected_flight_stream = do_get(context, request);
  if(!expected_flight_stream.has_value()) {
    return expected_flight_stream.error();
  }
  *stream = std::move(expected_flight_stream.value());
  return ::arrow::Status::OK();
}

::arrow::Status FlightHandler::DoPut(const ServerCallContext& context,
                                     std::unique_ptr<FlightMessageReader> reader,
                                     std::unique_ptr<FlightMetadataWriter>) {
  auto put_result = do_put(context, reader);
  if (!put_result.has_value()) {
    return put_result.error();
  }
  return ::arrow::Status::OK();
}

tl::expected<HeaderMiddleware*, std::string>
FlightHandler::get_header_middleware(const ::arrow::flight::ServerCallContext& context) {
  auto* header_middleware = context.GetMiddleware(std::string{HeaderMiddlewareKey.data(), HeaderMiddlewareKey.size()});
  if(header_middleware == nullptr) {
    return tl::make_unexpected("HeaderMiddleware not found in ServerCallContext");
  }
  return dynamic_cast<HeaderMiddleware*>(header_middleware);
}

tl::expected<std::string, std::string> FlightHandler::parse_header(const std::string& key,
                                                                   const HeaderMiddleware& middleware) {
  auto it = middleware.headers().find(key);
  if(it == middleware.headers().end()) {
    return tl::make_unexpected(fmt::format("Header '{}' not found", key));
  }
  return std::string{it->second};
}

tl::expected<std::unique_ptr<FlightInfo>, ::arrow::Status>
FlightHandler::get_flight_info(const ServerCallContext& context, const FlightDescriptor& request) {

  auto expected_middleware = get_header_middleware(context);
  if(!expected_middleware.has_value()) {
    return tl::make_unexpected(MakeFlightError(FlightStatusCode::Internal, "Internal error"));
  }
  auto* middleware = expected_middleware.value();

  auto expected_bucket = parse_header({BucketHeaderKey.data(), BucketHeaderKey.size()}, *middleware);
  if(!expected_bucket.has_value()) {
    return tl::make_unexpected(MakeFlightError(FlightStatusCode::Failed, expected_bucket.error()));
  }
  auto bucket = expected_bucket.value();

  auto expected_object = parse_header({ObjectHeaderKey.data(), ObjectHeaderKey.size()}, *middleware);
  if(!expected_object.has_value()) {
    return tl::make_unexpected(MakeFlightError(FlightStatusCode::Failed, expected_object.error()));
  }
  auto object = expected_object.value();

  switch(request.type) {
    case FlightDescriptor::UNKNOWN: {
      return tl::make_unexpected(MakeFlightError(
        FlightStatusCode::Failed, fmt::format("FlightDescriptor type '{}' not supported", request.type)));
    }
    case FlightDescriptor::PATH: {
      return get_flight_info_for_path(context, request, bucket, object);
    }
    case FlightDescriptor::CMD: {
      return get_flight_info_for_cmd(context, request, bucket, object);
    }
  }

  return tl::make_unexpected(
    MakeFlightError(FlightStatusCode::Failed, fmt::format("Unrecognized FlightDescriptor type '{}'", request.type)));
}

tl::expected<std::unique_ptr<FlightInfo>, ::arrow::Status>
FlightHandler::get_flight_info_for_path(const ServerCallContext&, const FlightDescriptor& request,
                                        std::string bucket, std::string object) {

  // Create the ticket
  auto ticket_object = GetObjectTicket::make(std::move(bucket), std::move(object));
  auto exp_ticket = ticket_object->to_ticket(false);
  if (!exp_ticket.has_value()) {
    return tl::make_unexpected(MakeFlightError(FlightStatusCode::Failed, exp_ticket.error()));
  }

  // Create the endpoints (dummy for now)
  std::vector<FlightEndpoint> endpoints;
  FlightEndpoint endpoint{*exp_ticket, {}};
  endpoints.push_back(endpoint);

  // Create the FlightInfo object, contains the original descriptor and the endpoints to find the data
  FlightInfo::Data flight_data;
  flight_data.descriptor = request;
  flight_data.endpoints = endpoints;
  auto schemaBytes = ArrowSerializer::schema_to_bytes(::arrow::schema({}));
  flight_data.schema = std::string(schemaBytes.begin(), schemaBytes.end());
  flight_data.total_records = -1;
  flight_data.total_bytes = -1;

  return std::make_unique<FlightInfo>(flight_data);
}

tl::expected<std::unique_ptr<FlightInfo>, ::arrow::Status>
FlightHandler::get_flight_info_for_cmd(const ServerCallContext& context, const FlightDescriptor& request,
                                       std::string, std::string) {

  // Parse the cmd object
  auto expected_cmd_object = CmdObject::deserialize(request.cmd);
  if(!expected_cmd_object.has_value()) {
    return tl::make_unexpected(MakeFlightError(FlightStatusCode::Failed, expected_cmd_object.error()));
  }
  auto cmd_object = expected_cmd_object.value();

  switch(cmd_object->type()->id()) {
    case CmdTypeId::SELECT_OBJECT_CONTENT: {
      auto select_object_content_cmd = std::static_pointer_cast<SelectObjectContentCmd>(cmd_object);
      return get_flight_info_for_select_object_content_cmd(context, request, select_object_content_cmd);
    }
    default: {
      return tl::make_unexpected(MakeFlightError(FlightStatusCode::Failed,
                                                 fmt::format("Unknown cmd type: '{}'", cmd_object->type()->name())));
    }
  }
}

tl::expected<std::unique_ptr<FlightInfo>, ::arrow::Status> FlightHandler::get_flight_info_for_select_object_content_cmd(
  const ServerCallContext&, const FlightDescriptor& request,
  const std::shared_ptr<SelectObjectContentCmd>& select_object_content_cmd) {

  // Create the ticket
  auto ticket_object = SelectObjectContentTicket::make(select_object_content_cmd->query_id(),
                                                       select_object_content_cmd->fpdb_store_super_pop(),
                                                       select_object_content_cmd->query_plan_string(),
                                                       select_object_content_cmd->parallel_degree(),
                                                       ReqExtraInfo() /*unused*/);
  auto exp_ticket = ticket_object->to_ticket(false);
  if (!exp_ticket.has_value()) {
    return tl::make_unexpected(MakeFlightError(FlightStatusCode::Failed, exp_ticket.error()));
  }

  // Create the endpoints (dummy for now)
  std::vector<FlightEndpoint> endpoints;
  FlightEndpoint endpoint{*exp_ticket, {}};
  endpoints.push_back(endpoint);

  // Create the FlightInfo object, contains the original descriptor and the endpoints to find the data
  FlightInfo::Data flight_data;
  flight_data.descriptor = request;
  flight_data.endpoints = endpoints;
  auto schemaBytes = ArrowSerializer::schema_to_bytes(::arrow::schema({}));
  flight_data.schema = std::string(schemaBytes.begin(), schemaBytes.end());
  flight_data.total_records = -1;
  flight_data.total_bytes = -1;

  return std::make_unique<FlightInfo>(flight_data);
}

tl::expected<std::unique_ptr<FlightDataStream>, ::arrow::Status> FlightHandler::do_get(const ServerCallContext& context,
                                                                                       const Ticket& request) {

  auto expected_ticket_object = TicketObject::deserialize(request);
  if (!expected_ticket_object.has_value()) {
    return tl::make_unexpected(MakeFlightError(FlightStatusCode::Failed, expected_ticket_object.error()));
  }
  auto ticket_object = expected_ticket_object.value();

  switch(ticket_object->type()->id()) {
    case TicketTypeId::GET_OBJECT: {
      auto get_object_ticket = std::static_pointer_cast<GetObjectTicket>(ticket_object);
      return do_get_get_object(context, get_object_ticket);
    }
    case TicketTypeId::SELECT_OBJECT_CONTENT: {
      auto select_object_content_ticket = std::static_pointer_cast<SelectObjectContentTicket>(ticket_object);
      return do_get_select_object_content(context, select_object_content_ticket);
    }
    case TicketTypeId::GET_BITMAP: {
      auto get_bitmap_ticket = std::static_pointer_cast<GetBitmapTicket>(ticket_object);
      return do_get_get_bitmap(context, get_bitmap_ticket);
    }
    case TicketTypeId::GET_TABLE: {
      auto get_table_ticket = std::static_pointer_cast<GetTableTicket>(ticket_object);
      return do_get_get_table(context, get_table_ticket);
    }
    case TicketTypeId::GET_BATCH_LOAD_INFO: {
      auto get_batch_load_info_ticket = std::static_pointer_cast<GetBatchLoadInfoTicket>(ticket_object);
      return do_get_get_batch_load_info(context, get_batch_load_info_ticket);
    }
  }

  return tl::make_unexpected(MakeFlightError(FlightStatusCode::Failed,
             fmt::format("Unrecognized Flight Ticket type '{}' at FPDB store", ticket_object->type()->name())));
}

tl::expected<std::unique_ptr<FlightDataStream>, ::arrow::Status>
FlightHandler::do_get_get_object(const ServerCallContext&,
                                 const std::shared_ptr<GetObjectTicket>&) {
  // TODO: currently just return a toy table
  auto schema = ::arrow::schema({
                                        {field("f0", ::arrow::int32())},
                                        {field("f1", ::arrow::int32())},
                                        {field("f2", ::arrow::int32())},
                                });

  auto array_0_0 = Arrays::make<::arrow::Int32Type>({0, 1, 2, 3, 4, 5, 6, 7, 8, 9}).value();
  auto array_0_1 = Arrays::make<::arrow::Int32Type>({10, 11, 12, 13, 14, 15, 16, 17, 18, 19}).value();
  auto array_0_2 = Arrays::make<::arrow::Int32Type>({20, 21, 22, 23, 24, 25, 26, 27, 28, 29}).value();

  auto rb_0 = ::arrow::RecordBatch::Make(schema, 10, {array_0_0, array_0_1, array_0_2});
  auto rb_1 = ::arrow::RecordBatch::Make(schema, 10, {array_0_0, array_0_1, array_0_2});

  auto rb_reader = ::arrow::RecordBatchReader::Make({rb_0, rb_1});

  return std::make_unique<::arrow::flight::RecordBatchStream>(*rb_reader);
}

tl::expected<std::unique_ptr<FlightDataStream>, ::arrow::Status> FlightHandler::do_get_select_object_content(
  const ServerCallContext&, const std::shared_ptr<SelectObjectContentTicket>& select_object_content_ticket) {

  // run select_object_content
  auto exp_table = run_select_object_content(select_object_content_ticket->query_id(),
                                             select_object_content_ticket->fpdb_store_super_pop(),
                                             select_object_content_ticket->query_plan_string(),
                                             select_object_content_ticket->parallel_degree(),
                                             select_object_content_ticket->extra_info());
  if (!exp_table.has_value()) {
    return tl::make_unexpected(exp_table.error());
  }

  // make record batch stream and return
  auto exp_batches = tuple::util::Util::table_to_record_batches(*exp_table);
  if (!exp_batches.has_value()) {
    return tl::make_unexpected(MakeFlightError(FlightStatusCode::Failed, exp_batches.error()));
  }
  auto rb_reader = ::arrow::RecordBatchReader::Make(*exp_batches);
  return std::make_unique<::arrow::flight::RecordBatchStream>(*rb_reader);
}

tl::expected<std::shared_ptr<arrow::Table>, ::arrow::Status>
FlightHandler::run_select_object_content(long query_id,
                                         const std::string &fpdb_store_super_pop,
                                         const std::string &query_plan_string,
                                         int parallel_degree,
                                         const ReqExtraInfo &extra_info) {
  // adaptive pushdown, check if need to fall back as pullup
  std::shared_ptr<AdaptPushdownReqInfo> req;        // for adapt_pushdown_manager
  std::shared_ptr<AdaptPushdownReqInfo2> req2;      // for adapt_pushdown_manager2
  std::shared_ptr<AdaptPushdownReqInfo3> req3;      // for adapt_pushdown_manager3
  if (ENABLE_ADAPTIVE_PUSHDOWN) {
    switch (AdaptPushdownManagerV) {
      case AdaptPushdownManagerVersion::V1: {
        req = std::make_shared<AdaptPushdownReqInfo>(query_id,
                                                     fpdb_store_super_pop,
                                                     parallel_degree);
        auto adapt_res = adapt_pushdown_manager_.receiveOne(req);
        if (!adapt_res.has_value()) {
          return tl::make_unexpected(MakeFlightError(FlightStatusCode::Failed, adapt_res.error()));
        }
        if (!(*adapt_res)) {
          return tl::make_unexpected(MakeFlightError(ReqRejectStatusCode, "Resource limited"));
        }
        // admitted, check if need to wait
        adapt_pushdown_manager_.admitOne(req);
        break;
      }
      case AdaptPushdownManagerVersion::V2: {
        req2 = std::make_shared<AdaptPushdownReqInfo2>(query_id,
                                                       fpdb_store_super_pop,
                                                       parallel_degree);
        auto adapt_res2 = adapt_pushdown_manager2_.receiveOne(req2);
        if (!adapt_res2.has_value()) {
          return tl::make_unexpected(MakeFlightError(FlightStatusCode::Failed, adapt_res2.error()));
        }
        if (req2->isPushedBack_) {
          return tl::make_unexpected(MakeFlightError(ReqRejectStatusCode, "Resource limited"));
        }
        // admitted, wait is already checked in "receiveOne()" so noop here
        break;
      }
      case AdaptPushdownManagerVersion::V3: {
        req3 = std::make_shared<AdaptPushdownReqInfo3>(query_id,
                                                       fpdb_store_super_pop,
                                                       extra_info);
        auto adapt_res3 = adapt_pushdown_manager3_->receiveOne(req3);
        if (!adapt_res3.has_value()) {
          return tl::make_unexpected(MakeFlightError(FlightStatusCode::Failed, adapt_res3.error()));
        }
        if (req3->isPushedBack_) {
          return tl::make_unexpected(MakeFlightError(ReqRejectStatusCode, "Resource limited"));
        }
        // admitted, wait is already checked in "receiveOne()" so noop here
        break;
      }
    }
  }

  // deserialize the query plan
  auto exp_physical_plan = PhysicalPlanDeserializer::deserialize(query_plan_string,
                                                                 getStoreRootPath(update_scan_drive_id()));
  if (!exp_physical_plan.has_value()) {
    if (ENABLE_ADAPTIVE_PUSHDOWN) {
      switch (AdaptPushdownManagerV) {
        case AdaptPushdownManagerVersion::V1: {
          adapt_pushdown_manager_.finishOne(req);
          break;
        }
        case AdaptPushdownManagerVersion::V2: {
          adapt_pushdown_manager2_.finishOne(req2);
          break;
        }
        case AdaptPushdownManagerVersion::V3: {
          adapt_pushdown_manager3_->finishOne(req3);
          break;
        }
      }
    }
    return tl::make_unexpected(MakeFlightError(FlightStatusCode::Failed, exp_physical_plan.error()));
  }
  auto physical_plan = *exp_physical_plan;

  // wait for bitmaps ready if required
  // need to maintain a map for fetched bloom filters, bc each request should consume the same bloom filter only once
  std::unordered_map<std::string, std::shared_ptr<bloomfilter::BloomFilterBase>> consumed_bloom_filters;
  for (const auto &op_it: physical_plan->getPhysicalOps()) {
    auto op = op_it.second;

    // for each op, get the embedded bloom filters
    // it's guaranteed that bloom filters are ready before we get them here
    const auto &consumerToBloomFilterInfo = op->getConsumerToBloomFilterInfo();
    for (const auto &consumerToBloomFilterIt: consumerToBloomFilterInfo) {
      auto bloom_filter_info = consumerToBloomFilterIt.second;
      auto bloom_filter_key = BloomFilterCache::generateBloomFilterKey(query_id,
                                                                       bloom_filter_info->bloomFilterCreatePOp_);
      std::shared_ptr<bloomfilter::BloomFilterBase> bloom_filter;
      // check "consumed_bloom_filters" first
      auto consumed_bf_it = consumed_bloom_filters.find(bloom_filter_key);
      if (consumed_bf_it != consumed_bloom_filters.end()) {
        bloom_filter = consumed_bf_it->second;
      } else {
        auto exp_bloom_filter = bloom_filter_cache_.consumeBloomFilter(bloom_filter_key);
        if (!exp_bloom_filter.has_value()) {
          if (ENABLE_ADAPTIVE_PUSHDOWN) {
            switch (AdaptPushdownManagerV) {
              case AdaptPushdownManagerVersion::V1: {
                adapt_pushdown_manager_.finishOne(req);
                break;
              }
              case AdaptPushdownManagerVersion::V2: {
                adapt_pushdown_manager2_.finishOne(req2);
                break;
              }
              case AdaptPushdownManagerVersion::V3: {
                adapt_pushdown_manager3_->finishOne(req3);
                break;
              }
            }
          }
          return tl::make_unexpected(MakeFlightError(FlightStatusCode::Failed, exp_bloom_filter.error()));
        }
        bloom_filter = *exp_bloom_filter;
        consumed_bloom_filters.emplace(bloom_filter_key, bloom_filter);
      }
      bloom_filter_info->bloomFilter_ = bloom_filter;
    }

    // for each filter with bitmap pushdown enabled, wait until its bitmap is ready
    if (op->getType() == POpType::FILTER) {
      auto typedOp = std::static_pointer_cast<filter::FilterPOp>(op);
      if (!typedOp->isBitmapPushdownEnabled()) {
        continue;
      }

      // wait for bitmap ready
      auto bitmap_key = BitmapCache::generateBitmapKey(query_id, typedOp->getBitmapWrapper()->mirrorOp_);
      auto bitmap = get_bitmap_from_cache(bitmap_key, BitmapType::FILTER_COMPUTE);
      typedOp->setBitmap(bitmap);
    }
  }

  // execute the query plan
  auto execution = std::make_shared<executor::FPDBStoreExecution>(
          query_id,
          use_adapt_pushdown_actor_system_vec_ ? adapt_pushdown_actor_system_vec_.back() : actor_system_,
          physical_plan,
          [&] (const std::string &consumer, const std::shared_ptr<arrow::Table> &table) {
            auto table_key = executor::flight::TableCache::generateTableKey(query_id, fpdb_store_super_pop, consumer);
            put_table_into_cache(table_key, table);
          },
          [&] (const std::string &sender, const std::vector<int64_t> &bitmap) {
            auto bitmap_key = BitmapCache::generateBitmapKey(query_id, sender);
            put_bitmap_into_cache(bitmap_key, bitmap, BitmapType::FILTER_STORAGE, true);
          });
  const auto &result_table = execution->execute()->table();

#if SHOW_DEBUG_METRICS == true
  // save metrics of bytes disk read
  bytes_disk_read_mutex_.lock();
  bytes_disk_read_by_query_[query_id] += execution->getDebugMetrics().getDiskMetrics().getBytesFromDisk();
  bytes_disk_read_mutex_.unlock();
#endif

  // return query result
  if (ENABLE_ADAPTIVE_PUSHDOWN) {
    switch (AdaptPushdownManagerV) {
      case AdaptPushdownManagerVersion::V1: {
        adapt_pushdown_manager_.finishOne(req);
        break;
      }
      case AdaptPushdownManagerVersion::V2: {
        adapt_pushdown_manager2_.finishOne(req2);
        break;
      }
      case AdaptPushdownManagerVersion::V3: {
        adapt_pushdown_manager3_->finishOne(req3);
        break;
      }
    }
  }
  return result_table;
}

tl::expected<std::unique_ptr<FlightDataStream>, ::arrow::Status> FlightHandler::do_get_get_bitmap(
        const ServerCallContext&, const std::shared_ptr<GetBitmapTicket>& get_bitmap_ticket) {

  // get bitmap from bitmap cache
  auto query_id = get_bitmap_ticket->query_id();
  auto op = get_bitmap_ticket->op();
  auto bitmap_key = BitmapCache::generateBitmapKey(query_id, op);
  auto bitmap = get_bitmap_from_cache(bitmap_key, BitmapType::FILTER_STORAGE);
  if (!bitmap.has_value()) {
    return tl::make_unexpected(MakeFlightError(FlightStatusCode::Failed,
                                               fmt::format("Storage bitmap with key '{}' is not valid", bitmap_key)));
  }

  // make recordBatch from bitmap
  auto exp_record_batch = ArrowSerializer::bitmap_to_recordBatch(*bitmap);
  if (!exp_record_batch .has_value()) {
    return tl::make_unexpected(MakeFlightError(FlightStatusCode::Failed, exp_record_batch.error()));
  }

  // make flightDataStream
  auto rb_reader = ::arrow::RecordBatchReader::Make({*exp_record_batch});
  return std::make_unique<::arrow::flight::RecordBatchStream>(*rb_reader);
}

tl::expected<std::unique_ptr<FlightDataStream>, ::arrow::Status> FlightHandler::do_get_get_table(
        const ServerCallContext&, const std::shared_ptr<GetTableTicket>& get_table_ticket) {
  // get table from table cache
  auto query_id = get_table_ticket->query_id();
  auto producer = get_table_ticket->producer();
  auto consumer = get_table_ticket->consumer();
  auto wait_not_exist = get_table_ticket->wait_not_exist();
  auto table_key = executor::flight::TableCache::generateTableKey(query_id, producer, consumer);
  auto exp_table = get_table_from_cache(table_key, wait_not_exist);
  if (!exp_table.has_value()) {
    return tl::make_unexpected(exp_table.error());
  }

  // make record batch stream and return
  auto exp_batches = tuple::util::Util::table_to_record_batches(*exp_table);
  if (!exp_batches.has_value()) {
    return tl::make_unexpected(MakeFlightError(FlightStatusCode::Failed, exp_batches.error()));
  }
  auto rb_reader = ::arrow::RecordBatchReader::Make(*exp_batches);
  return std::make_unique<::arrow::flight::RecordBatchStream>(*rb_reader);
}

tl::expected<std::unique_ptr<FlightDataStream>, ::arrow::Status> FlightHandler::do_get_get_batch_load_info(
        const ServerCallContext&, const std::shared_ptr<GetBatchLoadInfoTicket>& get_batch_load_info_ticket) {
  auto query_id = get_batch_load_info_ticket->query_id();
  auto producer = get_batch_load_info_ticket->producer();
  auto consumers = get_batch_load_info_ticket->consumers();
  size_t num_consumers = consumers.size();
  std::vector<std::shared_ptr<arrow::Table>> tables{num_consumers};
  std::vector<int64_t> lengths;
  lengths.resize(num_consumers);
  std::optional<std::shared_ptr<arrow::Schema>> schema = std::nullopt;

  // load tables and record lengths
  for (size_t i = 0; i < num_consumers; ++i) {
    auto table_key = executor::flight::TableCache::generateTableKey(query_id, producer, consumers[i]);
    auto exp_table = get_table_from_cache(table_key, false);
    if (!exp_table.has_value()) {
      return tl::make_unexpected(exp_table.error());
    }
    tables[i] = *exp_table;
    lengths[i] = (*exp_table)->num_rows();
  }

  // concatenate tables as a whole and store
  auto exp_concatenated_table = arrow::ConcatenateTables(tables);
  if (!exp_concatenated_table.ok()) {
    return tl::make_unexpected(exp_concatenated_table.status());
  }
  auto table_key = executor::flight::TableCache::generateTableKey(
          query_id, get_batch_load_info_ticket->batch_load_pop(), "");
  table_cache_.produceTable(table_key, *exp_concatenated_table);

  // make lengths as a record batch and return
  auto exp_length_batch = Util::makeTableLengthBatch(lengths);
  if (!exp_length_batch.has_value()) {
    return tl::make_unexpected(exp_length_batch.error());
  }
  auto rb_reader = ::arrow::RecordBatchReader::Make({*exp_length_batch});
  return std::make_unique<::arrow::flight::RecordBatchStream>(*rb_reader);
}

tl::expected<void, ::arrow::Status> FlightHandler::do_put(const ServerCallContext& context,
                                                          const std::unique_ptr<FlightMessageReader> &reader) {
  switch(reader->descriptor().type) {
    case FlightDescriptor::CMD: {
      return do_put_for_cmd(context, reader);
    }
    default: {
      return tl::make_unexpected(MakeFlightError(FlightStatusCode::Failed,
                                                 fmt::format("FlightDescriptor type '{}' not supported for DoPut",
                                                             reader->descriptor().type)));
    }
  }
}

tl::expected<void, ::arrow::Status>
FlightHandler::do_put_for_cmd(const ServerCallContext& context,
                              const std::unique_ptr<FlightMessageReader> &reader) {
  // Parse the cmd object
  auto expected_cmd_object = CmdObject::deserialize(reader->descriptor().cmd);
  if(!expected_cmd_object.has_value()) {
    return tl::make_unexpected(MakeFlightError(FlightStatusCode::Failed, expected_cmd_object.error()));
  }
  auto cmd_object = expected_cmd_object.value();

  switch (cmd_object->type()->id()) {
    case CmdTypeId::SELECT_OBJECT_CONTENT: {
      auto select_object_content_cmd = std::static_pointer_cast<SelectObjectContentCmd>(cmd_object);
      return do_put_select_object_content(context, select_object_content_cmd, reader);
    }
    case CmdTypeId::PUT_BITMAP: {
      auto put_bitmap_cmd = std::static_pointer_cast<PutBitmapCmd>(cmd_object);
      return do_put_put_bitmap(context, put_bitmap_cmd, reader);
    }
    case CmdTypeId::CLEAR_BITMAP: {
      auto clear_bitmap_cmd = std::static_pointer_cast<ClearBitmapCmd>(cmd_object);
      return do_put_clear_bitmap(context, clear_bitmap_cmd);
    }
    case CmdTypeId::CLEAR_TABLE: {
      auto clear_table_cmd = std::static_pointer_cast<ClearTableCmd>(cmd_object);
      return do_put_clear_table(context, clear_table_cmd);
    }
    case CmdTypeId::PUT_ADAPT_PUSHDOWN_METRICS: {
      auto put_adapt_pushdown_metrics = std::static_pointer_cast<PutAdaptPushdownMetricsCmd>(cmd_object);
      return do_put_put_adapt_pushdown_metrics(context, put_adapt_pushdown_metrics);
    }
    case CmdTypeId::CLEAR_ADAPT_PUSHDOWN_METRICS: {
      auto clear_adapt_pushdown_metrics = std::static_pointer_cast<ClearAdaptPushdownMetricsCmd>(cmd_object);
      return do_put_clear_adapt_pushdown_metrics(context, clear_adapt_pushdown_metrics);
    }
    case CmdTypeId::SET_ADAPT_PUSHDOWN: {
      auto set_adapt_pushdown = std::static_pointer_cast<SetAdaptPushdownCmd>(cmd_object);
      return do_put_set_adapt_pushdown(context, set_adapt_pushdown);
    }
    case CmdTypeId::PUSHBACK_COMPLETE: {
      auto pushback_complete = std::static_pointer_cast<PushbackCompleteCmd>(cmd_object);
      return do_put_pushback_complete(context, pushback_complete);
    }
    case CmdTypeId::SET_NUM_REQ_TO_TAIL: {
      auto set_num_req_to_tail = std::static_pointer_cast<SetNumReqToTailCmd>(cmd_object);
      return do_put_set_num_req_to_tail(context, set_num_req_to_tail);
    }
    default: {
      return tl::make_unexpected(MakeFlightError(FlightStatusCode::Failed,
                                                 fmt::format("Cmd type '{}' not supported for DoPut",
                                                             cmd_object->type()->name())));;
    }
  }
}

tl::expected<void, ::arrow::Status> FlightHandler::do_put_select_object_content(
        const ServerCallContext&,
        const std::shared_ptr<SelectObjectContentCmd>& select_object_content_cmd,
        const std::unique_ptr<FlightMessageReader>&) {
  // run select_object_content
  auto exp_table = run_select_object_content(select_object_content_cmd->query_id(),
                                             select_object_content_cmd->fpdb_store_super_pop(),
                                             select_object_content_cmd->query_plan_string(),
                                             select_object_content_cmd->parallel_degree(),
                                             ReqExtraInfo() /*unused*/);
  if (!exp_table.has_value()) {
    return tl::make_unexpected(exp_table.error());
  }
  return {};
}

tl::expected<void, ::arrow::Status>
FlightHandler::do_put_put_bitmap(const ServerCallContext&,
                                 const std::shared_ptr<PutBitmapCmd>& put_bitmap_cmd,
                                 const std::unique_ptr<FlightMessageReader> &reader) {
  // bitmap type
  auto bitmap_type = put_bitmap_cmd->bitmap_type();

  // read bitmap record batch
  ::arrow::RecordBatchVector recordBatches;
  auto status = reader->ReadAll(&recordBatches);
  if (!status.ok()) {
    return tl::make_unexpected(status);
  }

  // treat filter bitmap and bloom filter bitmap separately
  if (bitmap_type == BitmapType::BLOOM_FILTER_COMPUTE) {
    // num copies and
    auto num_copies = put_bitmap_cmd->num_copies();
    if (!num_copies.has_value()) {
      return tl::make_unexpected(MakeFlightError(FlightStatusCode::Failed,
                                                 "Num copies not set when putting bloom filter bitmap"));
    }

    // bloom filter json object
    auto bloom_filter_jobj = put_bitmap_cmd->bloom_filter();
    if (!bloom_filter_jobj.has_value()) {
      return tl::make_unexpected(MakeFlightError(FlightStatusCode::Failed,
                                                 "Bloom filter not set when putting bloom filter bitmap"));
    }
    auto exp_bloom_filter = BloomFilterBase::fromJson(*bloom_filter_jobj);
    if (!exp_bloom_filter.has_value()) {
      return tl::make_unexpected(MakeFlightError(FlightStatusCode::Failed, exp_bloom_filter.error()));
    }
    auto bloom_filter = *exp_bloom_filter;

    // bloom filter bitmap
    auto res = bloom_filter->saveBitmapRecordBatches(recordBatches);
    if (!res.has_value()) {
      return tl::make_unexpected(MakeFlightError(FlightStatusCode::Failed, res.error()));
    }

    // bloom filter key
    auto bloom_filter_key = BloomFilterCache::generateBloomFilterKey(put_bitmap_cmd->query_id(), put_bitmap_cmd->op());

    // put bloom filter
    bloom_filter_cache_.produceBloomFilter(bloom_filter_key, bloom_filter, *num_copies);
  } else {
    // check if only contains one batch
    if (recordBatches.size() != 1) {
      return tl::make_unexpected(MakeFlightError(FlightStatusCode::Failed,
              "RecordBatch stream for filter bitmap should only contain one recordBatch"));
    }

    // record batch to bitmap
    auto exp_bitmap = ArrowSerializer::recordBatch_to_bitmap(recordBatches[0]);
    if (!exp_bitmap.has_value()) {
      return tl::make_unexpected(MakeFlightError(FlightStatusCode::Failed, exp_bitmap.error()));
    }

    // bitmap key
    auto bitmap_key = BitmapCache::generateBitmapKey(put_bitmap_cmd->query_id(), put_bitmap_cmd->op());
    auto valid = put_bitmap_cmd->valid();

    // put bitmap
    put_bitmap_into_cache(bitmap_key, *exp_bitmap, put_bitmap_cmd->bitmap_type(), valid);
  }

  return {};
}

tl::expected<void, ::arrow::Status>
FlightHandler::do_put_clear_bitmap(const ServerCallContext&,
                                   const std::shared_ptr<ClearBitmapCmd>& clear_bitmap_cmd) {
  auto query_id = clear_bitmap_cmd->query_id();
  const auto &op = clear_bitmap_cmd->op();
  auto bitmap_type = clear_bitmap_cmd->bitmap_type();
  if (bitmap_type == BitmapType::BLOOM_FILTER_COMPUTE) {
    // bloom filter key
    auto bloom_filter_key = BloomFilterCache::generateBloomFilterKey(query_id, op);
    // just consumer the bloom filter
    bloom_filter_cache_.consumeBloomFilter(bloom_filter_key);
  } else {
    // bitmap key
    auto bitmap_key = BitmapCache::generateBitmapKey(query_id, op);
    // just consume the bitmap
    get_bitmap_from_cache(bitmap_key, bitmap_type);
  }

  return {};
}

tl::expected<void, ::arrow::Status>
FlightHandler::do_put_clear_table(const ServerCallContext&,
                                  const std::shared_ptr<ClearTableCmd>& clear_table_cmd) {
  auto query_id = clear_table_cmd->query_id();
  auto producer = clear_table_cmd->producer();
  auto consumer = clear_table_cmd->consumer();
  auto table_key = executor::flight::TableCache::generateTableKey(query_id, producer, consumer);
  // just consumer the table
  get_table_from_cache(table_key, false);
  return {};
}

tl::expected<void, ::arrow::Status> FlightHandler::do_put_put_adapt_pushdown_metrics(
        const ServerCallContext&,
        const std::shared_ptr<PutAdaptPushdownMetricsCmd>& put_adapt_pushdown_metrics_cmd) {
  // save metrics of adaptive pushdown
  switch (AdaptPushdownManagerV) {
    case AdaptPushdownManagerVersion::V1: {
      adapt_pushdown_manager_.addAdaptPushdownMetrics(put_adapt_pushdown_metrics_cmd->getAdaptPushdownMetrics());
      break;
    }
    case AdaptPushdownManagerVersion::V2: {
      adapt_pushdown_manager2_.addAdaptPushdownMetrics(put_adapt_pushdown_metrics_cmd->getAdaptPushdownMetrics());
      break;
    }
    case AdaptPushdownManagerVersion::V3: {
      adapt_pushdown_manager3_->addAdaptPushdownMetrics(put_adapt_pushdown_metrics_cmd->getAdaptPushdownMetrics());
      break;
    }
  }

  return {};
}

tl::expected<void, ::arrow::Status> FlightHandler::do_put_clear_adapt_pushdown_metrics(
        const ServerCallContext&,
        const std::shared_ptr<ClearAdaptPushdownMetricsCmd>&) {
  // clear metrics of adaptive pushdown
  switch (AdaptPushdownManagerV) {
    case AdaptPushdownManagerVersion::V1: {
      adapt_pushdown_manager_.clearAdaptPushdownMetrics();
      break;
    }
    case AdaptPushdownManagerVersion::V2: {
      adapt_pushdown_manager2_.clearAdaptPushdownMetrics();
      break;
    }
    case AdaptPushdownManagerVersion::V3: {
      adapt_pushdown_manager3_->clearAdaptPushdownMetrics();
      break;
    }
  }

  // also reset to use the default actor_system
  use_adapt_pushdown_actor_system_vec_ = false;
  adapt_pushdown_actor_system_vec_.clear();

  return {};
}

tl::expected<void, ::arrow::Status> FlightHandler::do_put_set_adapt_pushdown(
        const ServerCallContext&,
        const std::shared_ptr<SetAdaptPushdownCmd>& set_adapt_pushdown_cmd) {
  // set flag and param of adaptive pushdown
  ENABLE_ADAPTIVE_PUSHDOWN = set_adapt_pushdown_cmd->enableAdaptPushdown();
  if (MaxThreads != set_adapt_pushdown_cmd->maxThreads()) {
    MaxThreads = set_adapt_pushdown_cmd->maxThreads();
    adapt_pushdown_actor_system_cfg_.set("caf.scheduler.max-threads", MaxThreads);
    adapt_pushdown_actor_system_vec_.emplace_back(
            std::make_shared<::caf::actor_system>(adapt_pushdown_actor_system_cfg_));
    use_adapt_pushdown_actor_system_vec_ = true;
  }

  // clear number of fall pack requests
  if (AdaptPushdownManagerV == AdaptPushdownManagerVersion::V1) {
    adapt_pushdown_manager_.clearNumFallBackReqs();
  }

  return {};
}

tl::expected<void, ::arrow::Status> FlightHandler::do_put_pushback_complete(
          const ServerCallContext&,
          const std::shared_ptr<PushbackCompleteCmd>& pushback_complete_cmd) {
  // adapt_pushdown_manager v2 and v3 require this
  switch (AdaptPushdownManagerV) {
    case AdaptPushdownManagerVersion::V2: {
      auto req2 = std::make_shared<AdaptPushdownReqInfo2>(pushback_complete_cmd->query_id(),
                                                          pushback_complete_cmd->op());
      req2->isPushedBack_ = true;
      adapt_pushdown_manager2_.finishOne(req2);
      break;
    }
    case AdaptPushdownManagerVersion::V3: {
      auto req3 = std::make_shared<AdaptPushdownReqInfo3>(pushback_complete_cmd->query_id(),
                                                          pushback_complete_cmd->op(),
                                                          ReqExtraInfo() /*unused*/);
      req3->isPushedBack_ = true;
      adapt_pushdown_manager3_->finishOne(req3);
      break;
    }
    default: {
      return tl::make_unexpected(MakeFlightError(
              FlightStatusCode::Failed, "PushbackCompleteCmd should not be called when using AdaptPushdownManager v1"));
    }
  }
  return {};
}

tl::expected<void, ::arrow::Status> FlightHandler::do_put_set_num_req_to_tail(
        const ServerCallContext&,
        const std::shared_ptr<SetNumReqToTailCmd> set_num_req_to_tail_cmd) {
  // only adapt_pushdown_manager v3 requires this
  if (AdaptPushdownManagerV != AdaptPushdownManagerVersion::V3) {
    return {};
  }
  adapt_pushdown_manager3_->setNumReqToTail(set_num_req_to_tail_cmd->numReqToTail());
  return {};
}

std::optional<std::vector<int64_t>> FlightHandler::get_bitmap_from_cache(const std::string &key,
                                                                         BitmapType bitmap_type) {
  // get corresponding vars
  auto& bitmap_cache = bitmap_cache_map_[bitmap_type];
  auto& bitmap_mutex = bitmap_mutex_map_[bitmap_type];
  auto& bitmap_cvs = bitmap_cvs_map_[bitmap_type];

  // get bitmap until it's ready
  std::unique_lock lock(*bitmap_mutex);

  tl::expected<std::optional<std::vector<int64_t>>, std::string> exp_bitmap;
  auto cv = std::make_shared<std::condition_variable_any>();
  bitmap_cvs[key] = cv;

  cv->wait(lock, [&] {
    exp_bitmap = bitmap_cache->consumeBitmap(key);
    return exp_bitmap.has_value();
  });

  bitmap_cvs.erase(key);
  return *exp_bitmap;
}

tl::expected<std::shared_ptr<arrow::Table>, ::arrow::Status>
FlightHandler::get_table_from_cache(const std::string &key, bool wait_not_exist) {
  // try to get table from table cache
  auto exp_table = table_cache_.consumeTable(key);
  if (exp_table.has_value()) {
    return *exp_table;
  }

  // wait if needed, otherwise return error
  if (!wait_not_exist) {
    return tl::make_unexpected(MakeFlightError(FlightStatusCode::Failed, exp_table.error()));
  }

  std::unique_lock lock(table_mutex_);

  auto cv = std::make_shared<std::condition_variable_any>();
  table_cvs_[key] = cv;

  cv->wait(lock, [&] {
    exp_table = table_cache_.consumeTable(key);
    return exp_table.has_value();
  });

  table_cvs_.erase(key);
  return *exp_table;
}

void FlightHandler::put_bitmap_into_cache(const std::string &key,
                                          const std::vector<int64_t> &bitmap,
                                          BitmapType bitmap_type,
                                          bool valid) {
  // get corresponding vars
  auto& bitmap_cache = bitmap_cache_map_[bitmap_type];
  auto& bitmap_mutex = bitmap_mutex_map_[bitmap_type];
  auto& bitmap_cvs = bitmap_cvs_map_[bitmap_type];

  // put bitmap
  bitmap_cache->produceBitmap(key, bitmap, valid);

  bitmap_mutex->lock();
  auto cvIt = bitmap_cvs.find(key);
  if (cvIt != bitmap_cvs.end()) {
    cvIt->second->notify_one();
  }
  bitmap_mutex->unlock();
}

void FlightHandler::put_table_into_cache(const std::string &key, const std::shared_ptr<arrow::Table> &table) {
  // put table
  table_cache_.produceTable(key, table);

  // notify if needed
  table_mutex_.lock();
  auto cvIt = table_cvs_.find(key);
  if (cvIt != table_cvs_.end()) {
    cvIt->second->notify_one();
  }
  table_mutex_.unlock();
}

void FlightHandler::init_bitmap_cache() {
  auto bitmap_types = {BitmapType::FILTER_COMPUTE, BitmapType::FILTER_STORAGE};
  for (auto bitmap_type: bitmap_types) {
    bitmap_cache_map_[bitmap_type] = std::make_shared<BitmapCache>();
    bitmap_mutex_map_[bitmap_type] = std::make_shared<std::mutex>();
    bitmap_cvs_map_[bitmap_type] = std::unordered_map<std::string, std::shared_ptr<std::condition_variable_any>>();
  }
}

void FlightHandler::init_adapt_pushdown_manager() {
  // for adapt_pushdown_manager v3
  if (AdaptPushdownManagerV == AdaptPushdownManagerVersion::V3) {
    if (EnablePaAwareAdaptPushdown) {
      adapt_pushdown_manager3_ = std::make_shared<AdaptPushdownManager3Pa>();
    } else {
      adapt_pushdown_manager3_ = std::make_shared<AdaptPushdownManager3>();
    }
  }
}

std::string FlightHandler::getStoreRootPath(int driveId) {
  return fmt::format("{}-{}", store_root_path_prefix_, driveId);
}

int FlightHandler::update_scan_drive_id() {
  std::unique_lock lock(update_scan_drive_id_mutex_);
  if (++scan_drive_id_ >= num_drives_) {
    scan_drive_id_ -= num_drives_;
  }
  return scan_drive_id_;
}

} // namespace fpdb::store::server::flight
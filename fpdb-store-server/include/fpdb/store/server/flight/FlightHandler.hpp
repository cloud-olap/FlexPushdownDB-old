//
// Created by matt on 4/2/22.
//

#ifndef FPDB_FPDB_STORE_SERVER_INCLUDE_FPDB_STORE_SERVER_FLIGHT_FLIGHTHANDLER_HPP
#define FPDB_FPDB_STORE_SERVER_INCLUDE_FPDB_STORE_SERVER_FLIGHT_FLIGHTHANDLER_HPP

#include <arrow/api.h>
#include <arrow/flight/api.h>
#include <tl/expected.hpp>
#include "mutex"
#include "condition_variable"

#include "fpdb/store/server/flight/SelectObjectContentCmd.hpp"
#include "fpdb/store/server/flight/PutBitmapCmd.hpp"
#include "fpdb/store/server/flight/ClearBitmapCmd.hpp"
#include "fpdb/store/server/flight/ClearTableCmd.hpp"
#include "fpdb/store/server/flight/adaptive/PutAdaptPushdownMetricsCmd.hpp"
#include "fpdb/store/server/flight/adaptive/ClearAdaptPushdownMetricsCmd.hpp"
#include "fpdb/store/server/flight/adaptive/SetAdaptPushdownCmd.hpp"
#include "fpdb/store/server/flight/adaptive/PushbackCompleteCmd.hpp"
#include "fpdb/store/server/flight/adaptive/SetNumReqToTailCmd.hpp"
#include "fpdb/store/server/flight/adaptive/AdaptPushdownManager.hpp"
#include "fpdb/store/server/flight/adaptive/AdaptPushdownManager2.hpp"
#include "fpdb/store/server/flight/adaptive/AdaptPushdownManager3Pa.hpp"
#include "fpdb/store/server/flight/adaptive/ReqExtraInfo.hpp"
#include "fpdb/store/server/flight/GetObjectTicket.hpp"
#include "fpdb/store/server/flight/SelectObjectContentTicket.hpp"
#include "fpdb/store/server/flight/GetBitmapTicket.hpp"
#include "fpdb/store/server/flight/GetTableTicket.hpp"
#include "fpdb/store/server/flight/GetBatchLoadInfoTicket.hpp"
#include "fpdb/store/server/flight/HeaderMiddleware.hpp"
#include "fpdb/store/server/flight/BitmapType.h"
#include "fpdb/store/server/flight/BitmapCache.hpp"
#include "fpdb/store/server/flight/BloomFilterCache.hpp"
#include "fpdb/store/server/flight/Util.hpp"
#include "fpdb/store/server/caf/ActorManager.hpp"
#include "fpdb/executor/flight/TableCache.h"
#include "fpdb/executor/physical/PhysicalPlan.h"

using namespace ::arrow::flight;

namespace fpdb::store::server::flight {

class FlightHandler : FlightServerBase {

public:
  /**
   *
   * @param location
   */
  explicit FlightHandler(Location location,
                         std::shared_ptr<::caf::actor_system> actor_system,
                         std::string store_root_path_prefix,
                         int num_drives);

  /**
   *
   */
  ~FlightHandler() override;

  /**
   *
   * @return
   */
  tl::expected<void, std::string> init();

  /**
   *
   * @return
   */
  tl::expected<void, std::string> serve();

  /**
   *
   * @return
   */
  tl::expected<void, std::string> shutdown();

  /**
   *
   * @return
   */
  tl::expected<void, std::string> wait();

  /**
   *
   * @return
   */
  int port();

  /**
   *
   * @param context
   * @param request
   * @param info
   * @return
   */
  ::arrow::Status GetFlightInfo(const ServerCallContext& context, const FlightDescriptor& request,
                                std::unique_ptr<FlightInfo>* info) override;

  /**
   *
   * @param context
   * @param request
   * @param stream
   * @return
   */
  ::arrow::Status DoGet(const ServerCallContext& context, const Ticket& request,
                        std::unique_ptr<FlightDataStream>* stream) override;

  /**
   *
   * @param context
   * @param reader
   * @param writer
   * @return
   */
  ::arrow::Status DoPut(const ServerCallContext& context,
                        std::unique_ptr<FlightMessageReader> reader,
                        std::unique_ptr<FlightMetadataWriter> writer) override;

private:
  /**
   *
   * @param context
   * @return
   */
  static tl::expected<HeaderMiddleware*, std::string> get_header_middleware(const ServerCallContext& context);

  /**
   *
   * @param key
   * @param middleware
   * @return
   */
  static tl::expected<std::string, std::string> parse_header(const std::string& key,
                                                             const HeaderMiddleware& middleware);

  /**
   *
   * @param context
   * @param request
   * @return
   */
  tl::expected<std::unique_ptr<FlightInfo>, ::arrow::Status> get_flight_info(const ServerCallContext& context,
                                                                             const FlightDescriptor& request);

  /**
   *
   * @param context
   * @param request
   * @param bucket
   * @param object
   * @return
   */
  static tl::expected<std::unique_ptr<FlightInfo>, ::arrow::Status>
  get_flight_info_for_path(const ServerCallContext& context, const FlightDescriptor& request, std::string bucket,
                           std::string object);

  /**
   *
   * @param context
   * @param request
   * @param bucket
   * @param object
   * @return
   */
  tl::expected<std::unique_ptr<FlightInfo>, ::arrow::Status>
  get_flight_info_for_cmd(const ServerCallContext& context, const FlightDescriptor& request, std::string bucket,
                          std::string object);

  /**
   *
   * @param context
   * @param request
   * @param bucket
   * @param object
   * @param select_object_content_cmd
   * @return
   */
  static tl::expected<std::unique_ptr<FlightInfo>, ::arrow::Status> get_flight_info_for_select_object_content_cmd(
    const ServerCallContext& context, const FlightDescriptor& request,
    const std::shared_ptr<SelectObjectContentCmd>& select_object_content_cmd);

  /**
   *
   * @param context
   * @param request
   * @return
   */
  tl::expected<std::unique_ptr<FlightDataStream>, ::arrow::Status> do_get(const ServerCallContext& context,
                                                                          const Ticket& request);

  /**
   *
   * @param context
   * @param get_object_ticket
   * @return
   */
  tl::expected<std::unique_ptr<FlightDataStream>, ::arrow::Status>
  do_get_get_object(const ServerCallContext& context,
                    const std::shared_ptr<GetObjectTicket>& get_object_ticket);

  /**
   *
   * @param context
   * @param select_object_content_ticket
   * @return
   */
  tl::expected<std::unique_ptr<FlightDataStream>, ::arrow::Status>
  do_get_select_object_content(const ServerCallContext& context,
                               const std::shared_ptr<SelectObjectContentTicket>& select_object_content_ticket);

  /**
   *
   * @param query_id
   * @param fpdb_store_super_pop
   * @param query_plan_string
   * @param parallel_degree
   * @return
   */
  tl::expected<std::shared_ptr<arrow::Table>, ::arrow::Status> run_select_object_content(
          long query_id,
          const std::string &fpdb_store_super_pop,
          const std::string &query_plan_string,
          int parallel_degree,
          const ReqExtraInfo &extra_info);

  /**
   *
   * @param context
   * @param get_bitmap_ticket
   * @return
   */
  tl::expected<std::unique_ptr<FlightDataStream>, ::arrow::Status>
  do_get_get_bitmap(const ServerCallContext& context,
                    const std::shared_ptr<GetBitmapTicket>& get_bitmap_ticket);

  /**
   *
   * @param context
   * @param get_table_ticket
   * @return
   */
  tl::expected<std::unique_ptr<FlightDataStream>, ::arrow::Status>
  do_get_get_table(const ServerCallContext& context,
                   const std::shared_ptr<GetTableTicket>& get_table_ticket);


  /**
   *
   * @param context
   * @param get_batch_load_info_ticket
   * @return
   */
  tl::expected<std::unique_ptr<FlightDataStream>, ::arrow::Status>
  do_get_get_batch_load_info(const ServerCallContext& context,
                             const std::shared_ptr<GetBatchLoadInfoTicket>& get_batch_load_info_ticket);

  /**
   *
   * @param context
   * @param reader
   * @return
   */
  tl::expected<void, ::arrow::Status> do_put(const ServerCallContext& context,
                                             const std::unique_ptr<FlightMessageReader> &reader);

  /**
   *
   * @param context
   * @param reader
   * @return
   */
  tl::expected<void, ::arrow::Status> do_put_for_cmd(const ServerCallContext& context,
                                                     const std::unique_ptr<FlightMessageReader> &reader);

  /**
   *
   * @param context
   * @param put_bitmap_cmd
   * @param reader
   * @return
   */
  tl::expected<void, ::arrow::Status> do_put_select_object_content(
          const ServerCallContext& context,
          const std::shared_ptr<SelectObjectContentCmd>& put_bitmap_cmd,
          const std::unique_ptr<FlightMessageReader> &reader);

  /**
   *
   * @param context
   * @param query_id
   * @param op
   * @param reader
   * @return
   */
  tl::expected<void, ::arrow::Status> do_put_put_bitmap(const ServerCallContext& context,
                                                        const std::shared_ptr<PutBitmapCmd>& put_bitmap_cmd,
                                                        const std::unique_ptr<FlightMessageReader> &reader);

  /**
   *
   * @param context
   * @param clear_bitmap_cmd
   * @return
   */
  tl::expected<void, ::arrow::Status> do_put_clear_bitmap(const ServerCallContext& context,
                                                          const std::shared_ptr<ClearBitmapCmd>& clear_bitmap_cmd);

  /**
   *
   * @param context
   * @param clear_table_cmd
   * @return
   */
  tl::expected<void, ::arrow::Status> do_put_clear_table(const ServerCallContext& context,
                                                         const std::shared_ptr<ClearTableCmd>& clear_table_cmd);

  /**
   *
   * @param context
   * @param put_adapt_pushdown_metrics_cmd
   * @return
   */
  tl::expected<void, ::arrow::Status> do_put_put_adapt_pushdown_metrics(
          const ServerCallContext& context,
          const std::shared_ptr<PutAdaptPushdownMetricsCmd>& put_adapt_pushdown_metrics_cmd);

  /**
   *
   * @param context
   * @param clear_adapt_pushdown_metrics_cmd
   * @return
   */
  tl::expected<void, ::arrow::Status> do_put_clear_adapt_pushdown_metrics(
          const ServerCallContext& context,
          const std::shared_ptr<ClearAdaptPushdownMetricsCmd>& clear_adapt_pushdown_metrics_cmd);

  /**
   *
   * @param context
   * @param set_adapt_pushdown_cmd
   * @return
   */
  tl::expected<void, ::arrow::Status> do_put_set_adapt_pushdown(
          const ServerCallContext& context,
          const std::shared_ptr<SetAdaptPushdownCmd>& set_adapt_pushdown_cmd);

  /**
   *
   * @param context
   * @param pushback_complete_cmd
   * @return
   */
  tl::expected<void, ::arrow::Status> do_put_pushback_complete(
          const ServerCallContext& context,
          const std::shared_ptr<PushbackCompleteCmd>& pushback_complete_cmd);

  /**
   *
   * @param context
   * @param set_num_req_to_tail_cmd
   * @return
   */
  tl::expected<void, ::arrow::Status> do_put_set_num_req_to_tail(
          const ServerCallContext& context,
          const std::shared_ptr<SetNumReqToTailCmd> set_num_req_to_tail_cmd);

  /**
   *
   * @param key
   * @param bitmap_type
   * @return
   */
  std::optional<std::vector<int64_t>> get_bitmap_from_cache(const std::string &key,
                                                            BitmapType bitmap_type);

  /**
   *
   * @param key
   * @return
   */
  tl::expected<std::shared_ptr<arrow::Table>, ::arrow::Status> get_table_from_cache(const std::string &key,
                                                                                    bool wait_not_exist);

  /**
   *
   * @param key
   * @param bitmap
   * @param bitmap_type
   * @param valid
   */
  void put_bitmap_into_cache(const std::string &key,
                             const std::vector<int64_t> &bitmap,
                             BitmapType bitmap_type,
                             bool valid);

  /**
   *
   * @param key
   * @param table
   */
  void put_table_into_cache(const std::string &key, const std::shared_ptr<arrow::Table> &table);

  /**
   * init bitmap caches, as well as mutex and cond_var used for them
   */
  void init_bitmap_cache();

  /**
   * init adapt_pushdown_manager in case it's a ptr
   */
  void init_adapt_pushdown_manager();

  /**
   * Get root path of the corresponding drive
   */
  std::string getStoreRootPath(int driveId);

  /**
   * Update current drive id for scan, in a round-robin fashion
   */
  int update_scan_drive_id();

  ::arrow::flight::Location location_;
  std::shared_ptr<::caf::actor_system> actor_system_;

  std::string store_root_path_prefix_;
  int num_drives_;
  int scan_drive_id_ = 0;    // updated in round-robin
  std::mutex update_scan_drive_id_mutex_;

  // bitmap caches for FILTER_COMPUTE and FILTER_STORAGE
  std::unordered_map<BitmapType, std::shared_ptr<BitmapCache>> bitmap_cache_map_;

  // bloom filter cache
  BloomFilterCache bloom_filter_cache_;

  // table cache (e.x. for shuffle result)
  executor::flight::TableCache table_cache_;

  // mutex and cv for bitmap caches
  std::unordered_map<BitmapType, std::shared_ptr<std::mutex>> bitmap_mutex_map_;
  std::unordered_map<BitmapType,
          std::unordered_map<std::string, std::shared_ptr<std::condition_variable_any>>> bitmap_cvs_map_;

  // mutex and cv for table cache
  std::mutex table_mutex_;
  std::unordered_map<std::string, std::shared_ptr<std::condition_variable_any>> table_cvs_;

  // adaptive pushdown
  AdaptPushdownManager adapt_pushdown_manager_;
  AdaptPushdownManager2 adapt_pushdown_manager2_;
  std::shared_ptr<AdaptPushdownManager3> adapt_pushdown_manager3_;
  ::caf::actor_system_config adapt_pushdown_actor_system_cfg_;
  // FIXME: unsure why if just using one actor_system and replace by a new one when modifying actor_system_config,
  //  it may stuck (probably in its destructor), so here a work-around is to store all newly created actor_system
  //  in a vector (to avoid calling the destructor) and use the last one
  std::vector<std::shared_ptr<::caf::actor_system>> adapt_pushdown_actor_system_vec_;
  bool use_adapt_pushdown_actor_system_vec_ = false;

#if SHOW_DEBUG_METRICS == true
  // used for collect bytes of disk read per query
  std::unordered_map<long, int64_t> bytes_disk_read_by_query_;
  std::mutex bytes_disk_read_mutex_;
#endif
};

} // namespace fpdb::store::server::flight

#endif // FPDB_FPDB_STORE_SERVER_INCLUDE_FPDB_STORE_SERVER_FLIGHT_FLIGHTHANDLER_HPP

//
// Created by Yifei Yang on 2/23/22.
//

#ifndef FPDB_FPDB_STORE_SERVER_INCLUDE_FPDB_STORE_SERVER_FLIGHT_UTIL_HPP
#define FPDB_FPDB_STORE_SERVER_INCLUDE_FPDB_STORE_SERVER_FLIGHT_UTIL_HPP

#include "adaptive/AdaptPushdownManagerVersion.hpp"
#include <arrow/api.h>
#include <arrow/flight/api.h>
#include <tl/expected.hpp>
#include "string"
#include "thread"

namespace fpdb::store::server::flight {

static constexpr std::string_view TypeJSONName = "type";
static constexpr std::string_view BucketJSONName = "bucket";
static constexpr std::string_view ObjectJSONName = "object";
static constexpr std::string_view QueryIdJSONName = "query_id";
static constexpr std::string_view FPDBStoreSuperPOpJSONName = "fpdb_store_super_pop";
static constexpr std::string_view QueryPlanJSONName = "query_plan";
static constexpr std::string_view ParallelDegreeJSONName = "parallel_degree";
static constexpr std::string_view ExtraInfoJSONName = "extra_info";
static constexpr std::string_view IsDoubleExecJSONName = "is_double_exec";
static constexpr std::string_view SenderIpJSONName = "sender_ip";
static constexpr std::string_view SenderPortJSONName = "sender_port";
static constexpr std::string_view OpJSONName = "op";
static constexpr std::string_view ValidJSONName = "valid";
static constexpr std::string_view BitmapTypeJSONName = "bitmap_type";
static constexpr std::string_view NumCopiesJSONName = "num_copies";
static constexpr std::string_view BloomFilterJSONName = "bloom_filter";
static constexpr std::string_view ProducerJSONName = "producer";
static constexpr std::string_view ConsumerJSONName = "consumer";
static constexpr std::string_view ConsumersJSONName = "consumers";
static constexpr std::string_view BatchLoadPOpJSONName = "batch_load_pop";
static constexpr std::string_view WaitNotExistJSONName = "wait_not_exist";
static constexpr std::string_view AdaptPushdownMetricsJSONName = "adapt_pushdown_metrics";
static constexpr std::string_view EnableAdaptPushdownJSONName = "enable_adapt_pushdown";
static constexpr std::string_view MaxThreadsJSONName = "max_threads";
static constexpr std::string_view NumReqToTailJSONName = "num_req_to_tail";

static constexpr std::string_view GetObjectCmdTypeName = "get_object";
static constexpr std::string_view SelectObjectContentCmdTypeName = "select_object_content";
static constexpr std::string_view PutBitmapCmdTypeName = "put_bitmap";
static constexpr std::string_view ClearBitmapCmdTypeName = "clear_bitmap";
static constexpr std::string_view ClearTableCmdTypeName = "clear_table";
static constexpr std::string_view PutAdaptPushdownMetricsCmdTypeName = "put_adapt_pushdown_metrics";
static constexpr std::string_view ClearAdaptPushdownMetricsCmdTypeName = "clear_adapt_pushdown_metrics";
static constexpr std::string_view SetAdaptPushdownCmdTypeName = "clear_adapt_pushdown";
static constexpr std::string_view PushbackCompleteCmdTypeName = "pushback_complete";
static constexpr std::string_view PushbackDoubleExecCmdTypeName = "pushback_double_exec";
static constexpr std::string_view SetNumReqToTailCmdTypeName = "set_num_req_to_tail";
static constexpr std::string_view GetObjectTicketTypeName = "get_object";
static constexpr std::string_view SelectObjectContentTicketTypeName = "select_object_content";
static constexpr std::string_view GetBitmapTicketTypeName = "get_bitmap";
static constexpr std::string_view GetTableTicketTypeName = "get_table";
static constexpr std::string_view GetBatchLoadInfoTicketTypeName = "get_batch_load_info";

static constexpr std::string_view HeaderMiddlewareKey = "header_middleware";
static constexpr std::string_view BucketHeaderKey = "bucket";
static constexpr std::string_view ObjectHeaderKey = "object";

static constexpr arrow::flight::FlightStatusCode ReqRejectStatusCode = arrow::flight::FlightStatusCode::Unavailable;
inline int MaxThreads = std::thread::hardware_concurrency();     // max #threads for pushdown (CPU) requests
inline int MaxIoThreads = std::thread::hardware_concurrency() / 2;   // max #threads for pushback (IO) requests
static constexpr AdaptPushdownManagerVersion AdaptPushdownManagerV = AdaptPushdownManagerVersion::V3;
static constexpr bool EnablePushbackTailReqDoubleExec = false;   // this can only be set for adapt_pushdown_manager V3
static constexpr bool EnablePaAwareAdaptPushdown = false;    // this can only be set for adapt_pushdown_manager V3

static constexpr bool ShowDebugMetricsOnExit = false;

class Util {

public:
  /**
   * Used to generate an arrow table denoting the operator at storage side is finished,
   * to enable pipelining between the storage and compute layer.
   */
  static tl::expected<std::shared_ptr<arrow::Table>, std::string> getEndTable();
  static bool isEndTable(const std::shared_ptr<arrow::Table> &table);

  /**
   * Used to batch load tables constructed at storage side.
   */
  static tl::expected<std::shared_ptr<arrow::RecordBatch>, arrow::Status>
  makeTableLengthBatch(const std::vector<int64_t> &lengths);
  static tl::expected<void, std::string> readTableLengthBatch(const std::shared_ptr<arrow::RecordBatch> &recordBatch,
                                                              std::vector<int64_t> *lengths);

private:
  static constexpr std::string_view EndTableColumnName = "complete_col";
  static constexpr std::string_view EndTableRowValue = "complete";
  static constexpr std::string_view TableLengthColumnName = "table_length";

};

}

#endif //FPDB_FPDB_STORE_SERVER_INCLUDE_FPDB_STORE_SERVER_FLIGHT_UTIL_HPP

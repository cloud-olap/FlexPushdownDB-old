//
// Created by Yifei Yang on 9/28/22.
//

#ifndef FPDB_FPDB_EXECUTOR_INCLUDE_FPDB_EXECUTOR_FLIGHT_FLIGHTHANDLER_H
#define FPDB_FPDB_EXECUTOR_INCLUDE_FPDB_EXECUTOR_FLIGHT_FLIGHTHANDLER_H

#include <fpdb/executor/flight/TableCache.h>
#include <fpdb/store/server/flight/GetTableTicket.hpp>
#include <fpdb/store/server/flight/adaptive/PushbackDoubleExecCmd.hpp>
#include <arrow/api.h>
#include <arrow/flight/api.h>
#include <tl/expected.hpp>
#include <future>

using namespace ::arrow::flight;

namespace fpdb::executor::flight {

class FlightHandler : public FlightServerBase {

public:
  // A global flight server used for slave nodes in distributed node.
  // Also used by double-exec for tail pushback requests.
  inline static std::unique_ptr<FlightHandler> daemonServer_ = nullptr;
  static tl::expected<void, std::string> startDaemonFlightServer(int port,
          std::future<tl::expected<void, std::basic_string<char>>> &flight_future);
  static void stopDaemonFlightServer(
          std::future<tl::expected<void, std::basic_string<char>>> &flight_future);

  FlightHandler(Location location, const std::string &host);
  ~FlightHandler() override;

  const std::string &getHost() const;
  int getPort() const;

  tl::expected<void, std::string> init();
  tl::expected<void, std::string> serve();
  tl::expected<void, std::string> shutdown();
  tl::expected<void, std::string> wait();

  void putTable(long queryId, const std::string &producer, const std::string &consumer,
                const std::shared_ptr<arrow::Table> &table);

  ::arrow::Status DoGet(const ServerCallContext& context, const Ticket& request,
                        std::unique_ptr<FlightDataStream>* stream) override;

  ::arrow::Status DoPut(const ServerCallContext& context,
                        std::unique_ptr<FlightMessageReader> reader,
                        std::unique_ptr<FlightMetadataWriter> writer) override;

private:
  tl::expected<std::unique_ptr<FlightDataStream>, ::arrow::Status> do_get(const ServerCallContext& context,
                                                                          const Ticket& request);

  tl::expected<std::unique_ptr<FlightDataStream>, ::arrow::Status>
  do_get_get_table(const ServerCallContext& context,
                   const std::shared_ptr<store::server::flight::GetTableTicket>& get_table_ticket);

  tl::expected<void, ::arrow::Status> do_put(const ServerCallContext& context,
                                             const std::unique_ptr<FlightMessageReader> &reader);

  tl::expected<void, ::arrow::Status> do_put_for_cmd(const ServerCallContext& context,
                                                     const std::unique_ptr<FlightMessageReader> &reader);

  tl::expected<void, ::arrow::Status> do_put_pushback_double_exec(
          const ServerCallContext& context,
          const std::shared_ptr<store::server::flight::PushbackDoubleExecCmd>& pushback_double_exec_cmd);

  Location location_;
  std::string host_;
  TableCache table_cache_;

};

}


#endif //FPDB_FPDB_EXECUTOR_INCLUDE_FPDB_EXECUTOR_FLIGHT_FLIGHTHANDLER_H

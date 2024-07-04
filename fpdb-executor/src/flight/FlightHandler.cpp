//
// Created by Yifei Yang on 9/28/22.
//

#include <fpdb/executor/flight/FlightHandler.h>
#include <fpdb/executor/physical/fpdb-store/FPDBStoreSuperPOp.h>
#include <fpdb/tuple/util/Util.h>
#include <fpdb/util/Util.h>
#include <fmt/format.h>
#include <iostream>

using namespace fpdb::store::server::flight;

namespace fpdb::executor::flight {

tl::expected<void, std::string> FlightHandler::startDaemonFlightServer(int port,
        std::future<tl::expected<void, std::basic_string<char>>> &flight_future) {
  // if already started
  if (daemonServer_ != nullptr) {
    return {};
  }

  // local ip
  auto expLocalIp = fpdb::util::getLocalPrivateIp();
  if (!expLocalIp.has_value()) {
    return tl::make_unexpected(expLocalIp.error());
  }

  // make flight server
  ::arrow::flight::Location location;
  auto st = ::arrow::flight::Location::ForGrpcTcp("0.0.0.0", port, &location);
  if (!st.ok()) {
    return tl::make_unexpected("Cannot open flight server: " + st.message());
  }
  fpdb::executor::flight::FlightHandler::daemonServer_ = std::make_unique<fpdb::executor::flight::FlightHandler>(
          location, *expLocalIp);

  // init
  auto res = fpdb::executor::flight::FlightHandler::daemonServer_->init();
  if (!res.has_value()) {
    return tl::make_unexpected("Cannot open flight server: " + res.error());
  }

  // start
  flight_future = std::async(std::launch::async, [=]() {
    return fpdb::executor::flight::FlightHandler::daemonServer_->serve();
  });
  // Bit of a hack to check if the flight server failed on "serve"
  if (flight_future.wait_for(100ms) == std::future_status::ready) {
    return tl::make_unexpected("Cannot open flight server: " + flight_future.get().error());
  }
  std::cout << "Daemon flight server started at port: "
            << fpdb::executor::flight::FlightHandler::daemonServer_->port()
            << std::endl;
  return {};
}

void FlightHandler::stopDaemonFlightServer(
        std::future<tl::expected<void, std::basic_string<char>>> &flight_future) {
  if (daemonServer_ != nullptr) {
    fpdb::executor::flight::FlightHandler::daemonServer_->shutdown();
    fpdb::executor::flight::FlightHandler::daemonServer_->wait();
    flight_future.wait();
    fpdb::executor::flight::FlightHandler::daemonServer_.reset();
    std::cout << "Daemon flight server stopped" << std::endl;
  }
}

FlightHandler::FlightHandler(Location location, const std::string &host):
  location_(std::move(location)),
  host_(host) {}

FlightHandler::~FlightHandler() {
  this->shutdown();
}

const std::string &FlightHandler::getHost() const {
  return host_;
}

int FlightHandler::getPort() const {
  return FlightServerBase::port();
}

tl::expected<void, std::string> FlightHandler::init() {
  FlightServerOptions options(location_);
  auto st = this->Init(options);
  if (!st.ok()) {
    return tl::make_unexpected(st.message());
  }
  return {};
}

tl::expected<void, std::string> FlightHandler::serve() {
  auto st = this->Serve();
  if (!st.ok()) {
    return tl::make_unexpected(st.message());
  }
  return {};
}

tl::expected<void, std::string> FlightHandler::shutdown() {
  auto st = this->Shutdown();
  if (!st.ok()) {
    return tl::make_unexpected(st.message());
  }
  return {};
}

tl::expected<void, std::string> FlightHandler::wait() {
  auto st = this->Wait();
  if (!st.ok()) {
    return tl::make_unexpected(st.message());
  }
  return {};
}

void FlightHandler::putTable(long queryId, const std::string &producer, const std::string &consumer,
                             const std::shared_ptr<arrow::Table> &table) {
  table_cache_.produceTable(TableCache::generateTableKey(queryId, producer, consumer), table);
}

::arrow::Status FlightHandler::DoGet(const ServerCallContext& context, const Ticket& request,
                                     std::unique_ptr<FlightDataStream>* stream) {
  auto expected_flight_stream = do_get(context, request);
  if (!expected_flight_stream.has_value()) {
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

tl::expected<std::unique_ptr<FlightDataStream>, ::arrow::Status> FlightHandler::do_get(const ServerCallContext& context,
                                                                                       const Ticket& request) {

  auto expected_ticket_object = TicketObject::deserialize(request);
  if (!expected_ticket_object.has_value()) {
    return tl::make_unexpected(MakeFlightError(FlightStatusCode::Failed, expected_ticket_object.error()));
  }
  auto ticket_object = expected_ticket_object.value();

  switch(ticket_object->type()->id()) {
    case TicketTypeId::GET_TABLE: {
      auto get_table_ticket = std::static_pointer_cast<GetTableTicket>(ticket_object);
      return do_get_get_table(context, get_table_ticket);
    }
    default: {
      return tl::make_unexpected(MakeFlightError(FlightStatusCode::Failed,
              fmt::format("Unrecognized Flight Ticket type '{}' for compute cluster", ticket_object->type()->name())));
    }
  }
}

tl::expected<std::unique_ptr<FlightDataStream>, ::arrow::Status> FlightHandler::do_get_get_table(
        const ServerCallContext&, const std::shared_ptr<GetTableTicket>& get_table_ticket) {
  // get table from table cache
  auto query_id = get_table_ticket->query_id();
  auto producer = get_table_ticket->producer();
  auto consumer = get_table_ticket->consumer();
  auto table_key = TableCache::generateTableKey(query_id, producer, consumer);
  auto exp_table = table_cache_.consumeTable(table_key);
  if (!exp_table.has_value()) {
    return tl::make_unexpected(MakeFlightError(FlightStatusCode::Failed, exp_table.error()));
  }

  // make record batch stream and return
  auto exp_batches = tuple::util::Util::table_to_record_batches(*exp_table);
  if (!exp_batches.has_value()) {
    return tl::make_unexpected(MakeFlightError(FlightStatusCode::Failed, exp_batches.error()));
  }
  auto rb_reader = ::arrow::RecordBatchReader::Make(*exp_batches);
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
    case CmdTypeId::PUSHBACK_DOUBLE_EXEC: {
      auto pushback_double_exec = std::static_pointer_cast<PushbackDoubleExecCmd>(cmd_object);
      return do_put_pushback_double_exec(context, pushback_double_exec);
    }
    default: {
      return tl::make_unexpected(MakeFlightError(FlightStatusCode::Failed,
                                                 fmt::format("Cmd type '{}' not supported for DoPut",
                                                             cmd_object->type()->name())));;
    }
  }
}

tl::expected<void, ::arrow::Status> FlightHandler::do_put_pushback_double_exec(
        const ServerCallContext&,
        const std::shared_ptr<store::server::flight::PushbackDoubleExecCmd>& pushback_double_exec_cmd) {
  // check if pushback exists
  auto pushback_key = fpdb_store::makePushbackDoubleExecKey(
          pushback_double_exec_cmd->query_id(), pushback_double_exec_cmd->op());
  auto op_it = fpdb_store::OpsWithPushbackExec.find(pushback_key);
  if (op_it == fpdb_store::OpsWithPushbackExec.end()) {
    // if pushback exec hasn't been made, skip
    return {};
  }
  auto op = op_it->second;

  // try to create a pushdown exec if pushback exec is not finished yet
  if (op->doubleExecMutex_ == nullptr) {
    return {};
  }
  {
    std::unique_lock lock(*op->doubleExecMutex_);
    if (op->isOneExecFinished_) {
      return {};
    }
    op->doubleExecCv_ = std::make_shared<std::condition_variable_any>();
  }
  op->pushback_double_exec();
  return {};
}

}

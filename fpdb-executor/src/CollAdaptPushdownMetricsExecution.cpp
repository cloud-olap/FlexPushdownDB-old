//
// Created by Yifei Yang on 10/31/22.
//

#include <fpdb/executor/CollAdaptPushdownMetricsExecution.h>
#include <fpdb/executor/physical/Globals.h>
#include <fpdb/executor/physical/filter/FilterPOp.h>
#include <fpdb/executor/physical/file/RemoteFileScanPOp.h>
#include <fpdb/executor/physical/fpdb-store/FPDBStoreSuperPOp.h>
#include <fpdb/executor/flight/FlightClients.h>
#include <fpdb/store/server/flight/adaptive/PutAdaptPushdownMetricsCmd.hpp>

namespace fpdb::executor {

CollAdaptPushdownMetricsExecution::CollAdaptPushdownMetricsExecution(
        long queryId,
        const shared_ptr<::caf::actor_system> &actorSystem,
        const vector<::caf::node_id> &nodes,
        const ::caf::actor &localSegmentCacheActor,
        const vector<::caf::actor> &remoteSegmentCacheActors,
        const shared_ptr<PhysicalPlan> &physicalPlan,
        bool isDistributed,
        const std::shared_ptr<fpdb::catalogue::obj_store::FPDBStoreConnector> &fpdbStoreConnector):
  Execution(queryId, actorSystem, nodes,
            localSegmentCacheActor, remoteSegmentCacheActors,
            physicalPlan, isDistributed),
  fpdbStoreConnector_(fpdbStoreConnector) {}

shared_ptr<TupleSet> CollAdaptPushdownMetricsExecution::execute() {
  // execute
  preExecute();
  boot();
  start();
  join();

  // compute and send metrics of adaptive pushdown to store
  sendAdaptPushdownMetricsToStore();
  return legacyCollateOperator_->tuples();
}

void CollAdaptPushdownMetricsExecution::preExecute() {
  // Set query id, and add physical operators to operator directory
  // Also set flag for collecting adaptive pushdown metrics
  for (const auto &opIt: physicalPlan_->getPhysicalOps()) {
    auto op = opIt.second;
    op->setQueryId(queryId_);
    auto result = opDirectory_.insert(POpDirectoryEntry(op, nullptr, false));
    if (!result.has_value()) {
      throw runtime_error(result.error());
    }
    if (op->getType() == POpType::REMOTE_FILE_SCAN) {
      static_pointer_cast<file::RemoteFileScanPOp>(op)->setGetAdaptPushdownMetrics(true);
    } else if (op->getType() == POpType::FPDB_STORE_SUPER) {
      static_pointer_cast<fpdb_store::FPDBStoreSuperPOp>(op)->setGetAdaptPushdownMetrics(true);
    }
  }
}

void CollAdaptPushdownMetricsExecution::join() {
  SPDLOG_DEBUG("Waiting for all operators to complete");

  auto handle_err = [&](const ::caf::error &err) {
    throw runtime_error(to_string(err));
  };

  bool allComplete = false;
  (*rootActor_)->receive_while([&] { return !allComplete; })(
          [&](const Envelope &e) {
            const auto &msg = e.message();
            SPDLOG_DEBUG("Query root actor received message  |  query: '{}', messageKind: '{}', from: '{}'",
                         queryId_, msg.getTypeString(), msg.sender());

            auto errAct = [&](const std::string &errMsg) {
              allComplete = true;
              close();
              throw runtime_error(errMsg);
            };

            switch (msg.type()) {
              case MessageType::COMPLETE: {
                this->opDirectory_.setComplete(msg.sender())
                        .map_error(errAct);
                allComplete = this->opDirectory_.allComplete();
                break;
              }

              case MessageType::ADAPT_PUSHDOWN_METRICS: {
                auto adaptPushdownMetricsMsg = ((AdaptPushdownMetricsMessage &) msg);
                addAdaptPushdownMetrics(adaptPushdownMetricsMsg.getKey(), adaptPushdownMetricsMsg.getExecTime());
                break;
              }

#if SHOW_DEBUG_METRICS == true
              case MessageType::TRANSFER_METRICS: {
                auto transferMetricsMsg = ((TransferMetricsMessage &) msg);
                debugMetrics_.add(transferMetricsMsg.getTransferMetrics());
                break;
              }
#endif

              case MessageType::ERROR: {
                errAct(fmt::format("ERROR: {}, from {}", ((ErrorMessage &) msg).getContent(), msg.sender()));
              }
              default: {
                errAct(fmt::format("Invalid message type sent to the root actor: {}, from {}", msg.getTypeString(), msg.sender()));
              }
            }

          },
          handle_err);

  stopTime_ = chrono::steady_clock::now();
}

bool CollAdaptPushdownMetricsExecution::useDetached(const shared_ptr<PhysicalOp> &op) {
  // Here only make blocking op detached, ignore performance considerations because we here want to
  // measure metrics of a single pullup/pushdown request.
  return (op->getType() == POpType::FPDB_STORE_SUPER && ENABLE_FILTER_BITMAP_PUSHDOWN)
         || op->getType() == POpType::FPDB_STORE_TABLE_CACHE_LOAD;
}

void CollAdaptPushdownMetricsExecution::sendAdaptPushdownMetricsToStore() {
  // send metrics to store
  auto cmdObj = fpdb::store::server::flight::PutAdaptPushdownMetricsCmd::make(adaptPushdownMetrics_);
  auto expCmd = cmdObj->serialize(false);
  if (!expCmd.has_value()) {
    throw std::runtime_error(expCmd.error());
  }

  for (const auto &host: fpdbStoreConnector_->getHosts()) {
    // make flight client and connect
    auto client = flight::GlobalFlightClients.getFlightClient(host, fpdbStoreConnector_->getFlightPort());

    // send to host
    auto descriptor = ::arrow::flight::FlightDescriptor::Command(*expCmd);
    std::unique_ptr<arrow::flight::FlightStreamWriter> writer;
    std::unique_ptr<arrow::flight::FlightMetadataReader> metadataReader;
    auto status = client->DoPut(descriptor, nullptr, &writer, &metadataReader);
    if (!status.ok()) {
      throw std::runtime_error(status.message());
    }
    status = writer->Close();
    if (!status.ok()) {
      throw std::runtime_error(status.message());
    }
  }
}

void CollAdaptPushdownMetricsExecution::addAdaptPushdownMetrics(const std::string &key, int64_t execTime) {
  adaptPushdownMetrics_[key] = execTime;
}

}

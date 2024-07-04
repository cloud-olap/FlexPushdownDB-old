//
// Created by Yifei Yang on 1/14/22.
//

#include <fpdb/main/Server.h>
#include <fpdb/main/ExecConfig.h>
#include <fpdb/executor/caf/CAFInit.h>
#include <fpdb/executor/caf/CAFAdaptPushdownUtil.h>
#include <fpdb/executor/flight/FlightHandler.h>
#include <fpdb/executor/physical/Globals.h>
#include <fpdb/aws/AWSClient.h>
#include <fpdb/util/Util.h>
#include <iostream>
#include <future>
#include <utility>

using namespace fpdb::util;

namespace fpdb::main {

void Server::start() {
  // start the daemon AWS client
  const auto &awsConfig = AWSConfig::parseAWSConfig();
  fpdb::aws::AWSClient::daemonClient_ = make_shared<AWSClient>(awsConfig);
  fpdb::aws::AWSClient::daemonClient_->init();
  std::cout << "Daemon AWS client started" << std::endl;

  // start the daemon flight server if needed
  if (fpdb::executor::physical::USE_FLIGHT_COMM) {
    auto res = executor::flight::FlightHandler::startDaemonFlightServer(ExecConfig::parseFlightPort(), flight_future_);
    if (!res.has_value()) {
      throw std::runtime_error(res.error());
    }
  }

  // read remote Ips and server port
  const auto &remoteIps = readRemoteIps();
  int CAFServerPort = ExecConfig::parseCAFServerPort();

  // create the actor system
  actorSystemConfig_ = std::make_shared<ActorSystemConfig>(CAFServerPort, remoteIps, true);
  fpdb::executor::caf::CAFInit::initCAFGlobalMetaObjects();
  actorSystem_ = std::make_shared<::caf::actor_system>(*actorSystemConfig_);

  // open the port
  auto res = actorSystem_->middleman().open(actorSystemConfig_->port_);
  if (!res) {
    throw std::runtime_error("Cannot open CAF server at port: " + to_string(res.error()));
  } else {
    std::cout << "CAF server opened at port: " << actorSystemConfig_->port_ << std::endl;
  }

  // make actor system for adaptive pushdown if needed
  if (ENABLE_ADAPTIVE_PUSHDOWN) {
    fpdb::executor::caf::CAFAdaptPushdownUtil::startDaemonAdaptPushdownActorSystem();
    std::cout << "Daemon actor system for adaptive pushdown started" << std::endl;
  }

  std::cout << "Server started" << std::endl;
}

void Server::stop() {
  // stop the daemon AWS client
  fpdb::aws::AWSClient::daemonClient_->shutdown();
  std::cout << "Daemon AWS client stopped" << std::endl;

  // stop the daemon flight server if needed
  if (fpdb::executor::physical::USE_FLIGHT_COMM) {
    executor::flight::FlightHandler::stopDaemonFlightServer(flight_future_);
  }

  // close the actor system
  if (actorSystem_) {
    auto res = actorSystem_->middleman().close(actorSystemConfig_->port_);
    if (!res) {
      throw std::runtime_error("Cannot close CAF server at port: " + to_string(res.error()));
    }
  }
  std::cout << "CAF server stopped" << std::endl;

  // stop actor system for adaptive pushdown if needed
  if (ENABLE_ADAPTIVE_PUSHDOWN) {
    fpdb::executor::caf::CAFAdaptPushdownUtil::stopDaemonAdaptPushdownActorSystem();
    std::cout << "Daemon actor system for adaptive pushdown stopped" << std::endl;
  }

  std::cout << "Server stopped" << std::endl;
}

}

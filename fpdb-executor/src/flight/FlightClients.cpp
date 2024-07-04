//
// Created by Yifei Yang on 11/17/22.
//

#include <fpdb/executor/flight/FlightClients.h>

namespace fpdb::executor::flight {

arrow::flight::FlightClient* FlightClients::getFlightClient(const std::string &host, int port) {
  std::unique_lock lock(mutex_);

  // check if already made before
  std::string key = generateFlightClientKey(host, port);
  auto clientIt = clients_.find(key);
  if (clientIt != clients_.end()) {
    return clientIt->second.get();
  }

  // if not made yet, make one and save
  arrow::flight::Location clientLocation;
  auto status = arrow::flight::Location::ForGrpcTcp(host, port, &clientLocation);
  if (!status.ok()) {
    throw std::runtime_error(status.message());
  }

  arrow::flight::FlightClientOptions clientOptions = arrow::flight::FlightClientOptions::Defaults();
  std::unique_ptr<arrow::flight::FlightClient> client;
  status = arrow::flight::FlightClient::Connect(clientLocation, clientOptions, &client);
  if (!status.ok()) {
    throw std::runtime_error(status.message());
  }

  auto clientRawPtr = client.get();
  clients_[key] = std::move(client);
  return clientRawPtr;
}

void FlightClients::reset() {
  clients_.clear();
}

std::string FlightClients::generateFlightClientKey(const std::string &host, int port) {
  return host + ":" + std::to_string(port);
}

}

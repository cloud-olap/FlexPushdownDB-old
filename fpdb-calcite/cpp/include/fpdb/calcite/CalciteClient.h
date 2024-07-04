//
// Created by Yifei Yang on 10/13/21.
//

#ifndef FPDB_CALCITE_CPP_INCLUDE_CALCITE_H
#define FPDB_CALCITE_CPP_INCLUDE_CALCITE_H

#include <fpdb/calcite/CalciteConfig.h>
#include <../gen-cpp/CalciteServer.h>
#include <memory>
#include <mutex>

using namespace std;

namespace fpdb::calcite {

class CalciteClient {
public:
  explicit CalciteClient(const shared_ptr<CalciteConfig> &calciteConfig);
  void startClient();
  void startServer();
  void stopClient();
  void shutdownServer();
  string planQuery(const string& query, const string& schemaName, bool useHeuristicJoinOrdering = true);

private:
  shared_ptr<CalciteConfig> calciteConfig_;
  shared_ptr<::apache::thrift::transport::TTransport> transport_;
  shared_ptr<CalciteServerClient> calciteServerClient_;
  std::mutex mutex_;
};

}


#endif //FPDB_CALCITE_CPP_INCLUDE_CALCITE_H

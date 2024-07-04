//
// Created by Yifei Yang on 2/17/22.
//

#ifndef FPDB_FPDB_STORE_SERVER_INCLUDE_FPDB_STORE_SERVER_FILE_ARROWREMOTEFILEINPUTSTREAM_H
#define FPDB_FPDB_STORE_SERVER_INCLUDE_FPDB_STORE_SERVER_FILE_ARROWREMOTEFILEINPUTSTREAM_H

#include <arrow/api.h>
#include <arrow/io/interfaces.h>
#include <grpcpp/grpcpp.h>
#include <tl/expected.hpp>
#include <FileService.grpc.pb.h>

namespace fpdb::store::server::file {

class ArrowRemoteFileInputStream : public ::arrow::io::RandomAccessFile {

public:
  ArrowRemoteFileInputStream(const std::string &bucket,
                             const std::string &object,
                             const std::string &host,
                             int port);

  static std::shared_ptr<ArrowRemoteFileInputStream> make(const std::string &bucket,
                                                          const std::string &object,
                                                          const std::string &host,
                                                          int port);

  ~ArrowRemoteFileInputStream() = default;

  ::arrow::Result<int64_t> Read(int64_t nbytes, void* out) override;

  ::arrow::Result<std::shared_ptr<::arrow::Buffer>> Read(int64_t nbytes) override;

  ::arrow::Status Seek(int64_t position) override;

  ::arrow::Result<int64_t> GetSize() override;

  ::arrow::Status Close() override;

  ::arrow::Result<int64_t> Tell() const override;

  bool closed() const override;

  int64_t getBytesRead() const;

private:
  tl::expected<int64_t, std::string> read(int64_t nbytes, uint8_t* out);

  std::string bucket_;
  std::string object_;

  int64_t position_ = 0;
  std::shared_ptr<::grpc::Channel> channel_;
  std::shared_ptr<FileService::Stub> stub_;
  int64_t bytesRead_ = 0;
};

}


#endif //FPDB_FPDB_STORE_SERVER_INCLUDE_FPDB_STORE_SERVER_FILE_ARROWREMOTEFILEINPUTSTREAM_H

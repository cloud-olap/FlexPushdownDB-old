//
// Created by Yifei Yang on 2/17/22.
//

#include <fpdb/store/server/file/ArrowRemoteFileInputStream.h>
#include <fmt/format.h>

namespace fpdb::store::server::file {

ArrowRemoteFileInputStream::ArrowRemoteFileInputStream(const std::string &bucket,
                                                       const std::string &object,
                                                       const std::string &host,
                                                       int port):
  bucket_(bucket),
  object_(object) {

  grpc::ChannelArguments chanArgs;
  chanArgs.SetMaxReceiveMessageSize(INT_MAX);
  channel_ = CreateCustomChannel(fmt::format("{}:{}", host, port),
                           ::grpc::InsecureChannelCredentials(),
                           chanArgs);
  stub_ = FileService::NewStub(channel_);
}

std::shared_ptr<ArrowRemoteFileInputStream> ArrowRemoteFileInputStream::make(const std::string &bucket,
                                                                             const std::string &object,
                                                                             const std::string &host,
                                                                             int port) {
  return std::make_shared<ArrowRemoteFileInputStream>(bucket, object, host, port);
}

::arrow::Result<int64_t> ArrowRemoteFileInputStream::Read(int64_t nbytes, void* out) {
  auto result = read(nbytes, static_cast<uint8_t*>(out));
  if (result.has_value()) {
    position_ += *result;
    return ::arrow::Result<int64_t>(*result);
  } else {
    return ::arrow::Status::IOError(result.error());
  }
}

::arrow::Result<std::shared_ptr<::arrow::Buffer>> ArrowRemoteFileInputStream::Read(int64_t nbytes) {
  auto expBuffer = arrow::AllocateBuffer(nbytes, arrow::default_memory_pool());
  if (!expBuffer.ok()) {
    return expBuffer;
  }
  auto result = read(nbytes, (*expBuffer)->mutable_data());
  if (result.has_value()) {
    position_ += *result;
    return expBuffer;
  } else {
    return ::arrow::Status::IOError(result.error());
  }
}

tl::expected<int64_t, std::string> ArrowRemoteFileInputStream::read(int64_t nbytes, uint8_t* out) {
  std::vector<std::string> chunksRead;
  std::vector<int64_t> sizesRead;

  // make request
  ReadFileRequest request;
  request.set_bucket(bucket_);
  request.set_object(object_);
  request.mutable_option()->set_type(ReadOption_ReadType_RANGE);
  request.mutable_option()->set_position(position_);
  request.mutable_option()->set_length(nbytes);

  // collect responses
  ::grpc::ClientContext context;
  ReadFileResponse response;
  auto reader = stub_->ReadFile(&context, request);
  while (reader->Read(&response)) {
    chunksRead.emplace_back(response.data());
    sizesRead.emplace_back(response.bytes_read());
  }
  ::grpc::Status status = reader->Finish();
  if (!status.ok()) {
    return tl::make_unexpected(status.error_message());
  }

  // combine chunks
  int64_t offset = 0;
  for (uint i = 0; i < chunksRead.size(); ++i) {
    auto chunk = chunksRead[i];
    int64_t sizeRead = sizesRead[i];
    memcpy(out + offset, chunk.c_str(), sizeRead);
    offset += sizeRead;
  }

  bytesRead_ += nbytes;
  return offset;
}

::arrow::Status ArrowRemoteFileInputStream::Seek(int64_t position) {
  position_ = position;
  return ::arrow::Status::OK();
}

::arrow::Result<int64_t> ArrowRemoteFileInputStream::GetSize() {
  // make request
  GetFileSizeRequest request;
  request.set_bucket(bucket_);
  request.set_object(object_);

  // call
  ::grpc::ClientContext context;
  GetFileSizeResponse response;
  auto grpcStatus = stub_->GetFileSize(&context, request, &response);

  // check response
  if (grpcStatus.ok()) {
    return ::arrow::Result<int64_t>(response.size());
  } else {
    return ::arrow::Status::IOError(grpcStatus.error_message());
  }
}

::arrow::Status ArrowRemoteFileInputStream::Close() {
  return ::arrow::Status::OK();
}

::arrow::Result<int64_t> ArrowRemoteFileInputStream::Tell() const {
  return arrow::Result<int64_t>(position_);
}

bool ArrowRemoteFileInputStream::closed() const {
  return false;
}

int64_t ArrowRemoteFileInputStream::getBytesRead() const {
  return bytesRead_;
}

}

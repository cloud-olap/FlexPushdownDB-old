//
// Created by Yifei Yang on 1/21/22.
//

#ifndef FPDB_FPDB_EXECUTOR_INCLUDE_FPDB_EXECUTOR_MESSAGE_MESSAGETYPE_H
#define FPDB_FPDB_EXECUTOR_INCLUDE_FPDB_EXECUTOR_MESSAGE_MESSAGETYPE_H

namespace fpdb::executor::message {

enum MessageType {
  START,
  STOP,
  CONNECT,
  COMPLETE,
  ERROR,
  SCAN,
  TRANSFER_METRICS,
  DISK_METRICS,
  PRED_TRANS_METRICS,
  TUPLESET,
  TUPLESET_BUFFER,
  TUPLESET_INDEX,
  TUPLESET_READY_REMOTE,
  TUPLESET_WAIT_REMOTE,
  TUPLESET_SIZE,
  BLOOM_FILTER,
  BITMAP,
  ADAPT_PUSHDOWN_METRICS,
  PUSHDOWN_FALL_BACK,
  LOAD_REQUEST,
  LOAD_RESPONSE,
  STORE_REQUEST,
  WEIGHT_REQUEST,
  CACHE_METRICS
};

}

#endif //FPDB_FPDB_EXECUTOR_INCLUDE_FPDB_EXECUTOR_MESSAGE_MESSAGETYPE_H

//
// Created by Yifei Yang on 1/12/22.
//

#ifndef FPDB_FPDB_EXECUTOR_INCLUDE_FPDB_EXECUTOR_CAFSERIALIZATION_CAFMESSAGESERIALIZER_H
#define FPDB_FPDB_EXECUTOR_INCLUDE_FPDB_EXECUTOR_CAFSERIALIZATION_CAFMESSAGESERIALIZER_H

#include <fpdb/executor/message/Message.h>
#include <fpdb/executor/message/StartMessage.h>
#include <fpdb/executor/message/ConnectMessage.h>
#include <fpdb/executor/message/CompleteMessage.h>
#include <fpdb/executor/message/ErrorMessage.h>
#include <fpdb/executor/message/ScanMessage.h>
#include <fpdb/executor/message/TransferMetricsMessage.h>
#include <fpdb/executor/message/DiskMetricsMessage.h>
#include <fpdb/executor/message/TupleSetMessage.h>
#include <fpdb/executor/message/TupleSetBufferMessage.h>
#include <fpdb/executor/message/TupleSetIndexMessage.h>
#include <fpdb/executor/message/TupleSetReadyRemoteMessage.h>
#include <fpdb/executor/message/TupleSetWaitRemoteMessage.h>
#include <fpdb/executor/message/TupleSetSizeMessage.h>
#include <fpdb/executor/message/BloomFilterMessage.h>
#include <fpdb/executor/message/BitmapMessage.h>
#include <fpdb/executor/message/AdaptPushdownMetricsMessage.h>
#include <fpdb/executor/message/cache/LoadRequestMessage.h>
#include <fpdb/executor/message/cache/LoadResponseMessage.h>
#include <fpdb/executor/message/cache/StoreRequestMessage.h>
#include <fpdb/executor/message/cache/WeightRequestMessage.h>
#include <fpdb/executor/message/cache/CacheMetricsMessage.h>
#include <fpdb/caf/CAFUtil.h>
#include <fpdb/tuple/caf-serialization/CAFTupleKeyElementSerializer.h>

using namespace fpdb::executor::message;

using MessagePtr = std::shared_ptr<fpdb::executor::message::Message>;

CAF_BEGIN_TYPE_ID_BLOCK(Message, fpdb::caf::CAFUtil::Message_first_custom_type_id)
CAF_ADD_TYPE_ID(Message, (MessagePtr))
CAF_ADD_TYPE_ID(Message, (StartMessage))
CAF_ADD_TYPE_ID(Message, (ConnectMessage))
CAF_ADD_TYPE_ID(Message, (CompleteMessage))
CAF_ADD_TYPE_ID(Message, (ErrorMessage))
CAF_ADD_TYPE_ID(Message, (ScanMessage))
CAF_ADD_TYPE_ID(Message, (TransferMetricsMessage))
CAF_ADD_TYPE_ID(Message, (DiskMetricsMessage))
CAF_ADD_TYPE_ID(Message, (TupleSetMessage))
CAF_ADD_TYPE_ID(Message, (TupleSetBufferMessage))
CAF_ADD_TYPE_ID(Message, (TupleSetIndexMessage))
CAF_ADD_TYPE_ID(Message, (TupleSetReadyRemoteMessage))
CAF_ADD_TYPE_ID(Message, (TupleSetWaitRemoteMessage))
CAF_ADD_TYPE_ID(Message, (TupleSetSizeMessage))
CAF_ADD_TYPE_ID(Message, (BloomFilterMessage))
CAF_ADD_TYPE_ID(Message, (BitmapMessage))
CAF_ADD_TYPE_ID(Message, (AdaptPushdownMetricsMessage))
// For the following cache messages, we have to implement `inspect` for concrete derived shared_ptr type one by one,
// because SegmentCacheActor directly uses the concrete derived types rather than base type Message used by other actors
CAF_ADD_TYPE_ID(Message, (LoadRequestMessage))
CAF_ADD_TYPE_ID(Message, (LoadResponseMessage))
CAF_ADD_TYPE_ID(Message, (StoreRequestMessage))
CAF_ADD_TYPE_ID(Message, (WeightRequestMessage))
CAF_ADD_TYPE_ID(Message, (CacheMetricsMessage))
CAF_ADD_TYPE_ID(Message, (std::shared_ptr<LoadResponseMessage>))
CAF_ADD_TYPE_ID(Message, (std::shared_ptr<LoadRequestMessage>))
CAF_ADD_TYPE_ID(Message, (std::shared_ptr<StoreRequestMessage>))
CAF_ADD_TYPE_ID(Message, (std::shared_ptr<WeightRequestMessage>))
CAF_ADD_TYPE_ID(Message, (std::shared_ptr<CacheMetricsMessage>))
CAF_END_TYPE_ID_BLOCK(Message)

// Variant-based approach on MessagePtr
namespace caf {

template<>
struct variant_inspector_traits<MessagePtr> {
  using value_type = MessagePtr;

  // Lists all allowed types and gives them a 0-based index.
  static constexpr type_id_t allowed_types[] = {
          type_id_v<none_t>,
          type_id_v<StartMessage>,
          type_id_v<ConnectMessage>,
          type_id_v<CompleteMessage>,
          type_id_v<ErrorMessage>,
          type_id_v<ScanMessage>,
          type_id_v<TransferMetricsMessage>,
          type_id_v<DiskMetricsMessage>,
          type_id_v<TupleSetMessage>,
          type_id_v<TupleSetBufferMessage>,
          type_id_v<TupleSetIndexMessage>,
          type_id_v<TupleSetReadyRemoteMessage>,
          type_id_v<TupleSetWaitRemoteMessage>,
          type_id_v<TupleSetSizeMessage>,
          type_id_v<BloomFilterMessage>,
          type_id_v<BitmapMessage>,
          type_id_v<AdaptPushdownMetricsMessage>
  };

  // Returns which type in allowed_types corresponds to x.
  static auto type_index(const value_type &x) {
    if (!x)
      return 0;
    else if (x->type() == MessageType::START)
      return 1;
    else if (x->type() == MessageType::CONNECT)
      return 2;
    else if (x->type() == MessageType::COMPLETE)
      return 3;
    else if (x->type() == MessageType::ERROR)
      return 4;
    else if (x->type() == MessageType::SCAN)
      return 5;
    else if (x->type() == MessageType::TRANSFER_METRICS)
      return 6;
    else if (x->type() == MessageType::DISK_METRICS)
      return 7;
    else if (x->type() == MessageType::TUPLESET)
      return 8;
    else if (x->type() == MessageType::TUPLESET_BUFFER)
      return 9;
    else if (x->type() == MessageType::TUPLESET_INDEX)
      return 10;
    else if (x->type() == MessageType::TUPLESET_READY_REMOTE)
      return 11;
    else if (x->type() == MessageType::TUPLESET_WAIT_REMOTE)
      return 12;
    else if (x->type() == MessageType::TUPLESET_SIZE)
      return 13;
    else if (x->type() == MessageType::BLOOM_FILTER)
      return 14;
    else if (x->type() == MessageType::BITMAP)
      return 15;
    else if (x->type() == MessageType::ADAPT_PUSHDOWN_METRICS)
      return 16;
    else
      return -1;
  }

  // Applies f to the value of x.
  template<class F>
  static auto visit(F &&f, const value_type &x) {
    switch (type_index(x)) {
      case 1:
        return f(dynamic_cast<StartMessage &>(*x));
      case 2:
        return f(dynamic_cast<ConnectMessage &>(*x));
      case 3:
        return f(dynamic_cast<CompleteMessage &>(*x));
      case 4:
        return f(dynamic_cast<ErrorMessage &>(*x));
      case 5:
        return f(dynamic_cast<ScanMessage &>(*x));
      case 6:
        return f(dynamic_cast<TransferMetricsMessage &>(*x));
      case 7:
        return f(dynamic_cast<DiskMetricsMessage &>(*x));
      case 8:
        return f(dynamic_cast<TupleSetMessage &>(*x));
      case 9:
        return f(dynamic_cast<TupleSetBufferMessage &>(*x));
      case 10:
        return f(dynamic_cast<TupleSetIndexMessage &>(*x));
      case 11:
        return f(dynamic_cast<TupleSetReadyRemoteMessage &>(*x));
      case 12:
        return f(dynamic_cast<TupleSetWaitRemoteMessage &>(*x));
      case 13:
        return f(dynamic_cast<TupleSetSizeMessage &>(*x));
      case 14:
        return f(dynamic_cast<BloomFilterMessage &>(*x));
      case 15:
        return f(dynamic_cast<BitmapMessage &>(*x));
      case 16:
        return f(dynamic_cast<AdaptPushdownMetricsMessage &>(*x));
      default: {
        none_t dummy;
        return f(dummy);
      }
    }
  }

  // Assigns a value to x.
  template<class U>
  static void assign(value_type &x, U value) {
    if constexpr (std::is_same<U, none_t>::value)
      x.reset();
    else
      x = std::make_shared<U>(std::move(value));
  }

  // Create a default-constructed object for `type` and then call the
  // continuation with the temporary object to perform remaining load steps.
  template<class F>
  static bool load(type_id_t type, F continuation) {
    switch (type) {
      default:
        return false;
      case type_id_v<none_t>: {
        none_t dummy;
        continuation(dummy);
        return true;
      }
      case type_id_v<StartMessage>: {
        auto tmp = StartMessage{};
        continuation(tmp);
        return true;
      }
      case type_id_v<ConnectMessage>: {
        auto tmp = ConnectMessage{};
        continuation(tmp);
        return true;
      }
      case type_id_v<CompleteMessage>: {
        auto tmp = CompleteMessage{};
        continuation(tmp);
        return true;
      }
      case type_id_v<ErrorMessage>: {
        auto tmp = ErrorMessage{};
        continuation(tmp);
        return true;
      }
      case type_id_v<ScanMessage>: {
        auto tmp = ScanMessage{};
        continuation(tmp);
        return true;
      }
      case type_id_v<TransferMetricsMessage>: {
        auto tmp = TransferMetricsMessage{};
        continuation(tmp);
        return true;
      }
      case type_id_v<DiskMetricsMessage>: {
        auto tmp = DiskMetricsMessage{};
        continuation(tmp);
        return true;
      }
      case type_id_v<TupleSetMessage>: {
        auto tmp = TupleSetMessage{};
        continuation(tmp);
        return true;
      }
      case type_id_v<TupleSetBufferMessage>: {
        auto tmp = TupleSetBufferMessage{};
        continuation(tmp);
        return true;
      }
      case type_id_v<TupleSetIndexMessage>: {
        auto tmp = TupleSetIndexMessage{};
        continuation(tmp);
        return true;
      }
      case type_id_v<TupleSetReadyRemoteMessage>: {
        auto tmp = TupleSetReadyRemoteMessage{};
        continuation(tmp);
        return true;
      }
      case type_id_v<TupleSetWaitRemoteMessage>: {
        auto tmp = TupleSetWaitRemoteMessage{};
        continuation(tmp);
        return true;
      }
      case type_id_v<TupleSetSizeMessage>: {
        auto tmp = TupleSetSizeMessage{};
        continuation(tmp);
        return true;
      }
      case type_id_v<BloomFilterMessage>: {
        auto tmp = BloomFilterMessage{};
        continuation(tmp);
        return true;
      }
      case type_id_v<BitmapMessage>: {
        auto tmp = BitmapMessage{};
        continuation(tmp);
        return true;
      }
      case type_id_v<AdaptPushdownMetricsMessage>: {
        auto tmp = AdaptPushdownMetricsMessage{};
        continuation(tmp);
        return true;
      }
    }
  }
};

template <>
struct inspector_access<MessagePtr> : variant_inspector_access<MessagePtr> {
  // nop
};

} // namespace caf

#endif //FPDB_FPDB_EXECUTOR_INCLUDE_FPDB_EXECUTOR_CAFSERIALIZATION_CAFMESSAGESERIALIZER_H

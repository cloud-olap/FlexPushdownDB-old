//
// Created by Yifei Yang on 3/16/22.
//

#ifndef FPDB_FPDB_EXECUTOR_INCLUDE_FPDB_EXECUTOR_PHYSICAL_BLOOMFILTER_BLOOMFILTERUSEPOP_H
#define FPDB_FPDB_EXECUTOR_INCLUDE_FPDB_EXECUTOR_PHYSICAL_BLOOMFILTER_BLOOMFILTERUSEPOP_H

#include <fpdb/executor/physical/PhysicalOp.h>
#include <fpdb/executor/message/BloomFilterMessage.h>

namespace fpdb::executor::physical::bloomfilter {

class BloomFilterUsePOp: public PhysicalOp {
  
public:
  explicit BloomFilterUsePOp(const std::string &name,
                             const std::vector<std::string> &projectColumnNames,
                             int nodeId,
                             const std::vector<std::string> &bloomFilterColumnNames);

  BloomFilterUsePOp() = default;
  BloomFilterUsePOp(const BloomFilterUsePOp&) = default;
  BloomFilterUsePOp& operator=(const BloomFilterUsePOp&) = default;
  ~BloomFilterUsePOp() = default;

  void onReceive(const Envelope &envelope) override;
  void clear() override;
  std::string getTypeString() const override;

  const std::vector<std::string> &getBloomFilterColumnNames() const;
  const std::optional<std::shared_ptr<BloomFilterBase>> &getBloomFilter() const;
  void setBloomFilter(const std::shared_ptr<BloomFilterBase> &bloomFilter);
  bool receivedBloomFilter() const;
  void clearProducersExceptBloomFilterCreate();

private:
  void onStart();
  void onTupleSet(const TupleSetMessage &msg);
  void onBloomFilter(const BloomFilterMessage &msg);
  void onComplete(const CompleteMessage &);

  tl::expected<void, std::string> filterAndSend();

  std::vector<std::string> bloomFilterColumnNames_;

  std::optional<std::shared_ptr<TupleSet>> receivedTupleSet_;
  std::optional<std::shared_ptr<BloomFilterBase>> bloomFilter_;

// caf inspect
public:
  template <class Inspector>
  friend bool inspect(Inspector& f, BloomFilterUsePOp& op) {
    return f.object(op).fields(f.field("name", op.name_),
                               f.field("type", op.type_),
                               f.field("projectColumnNames", op.projectColumnNames_),
                               f.field("nodeId", op.nodeId_),
                               f.field("queryId", op.queryId_),
                               f.field("opContext", op.opContext_),
                               f.field("producers", op.producers_),
                               f.field("consumers", op.consumers_),
                               f.field("consumerToBloomFilterInfo", op.consumerToBloomFilterInfo_),
                               f.field("isSeparated", op.isSeparated_),
                               f.field("bloomFilterColumnNames", op.bloomFilterColumnNames_));
  }
};

}


#endif //FPDB_FPDB_EXECUTOR_INCLUDE_FPDB_EXECUTOR_PHYSICAL_BLOOMFILTER_BLOOMFILTERUSEPOP_H

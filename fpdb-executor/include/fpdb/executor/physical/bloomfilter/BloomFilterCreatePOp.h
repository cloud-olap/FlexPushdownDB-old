//
// Created by Yifei Yang on 3/16/22.
//

#ifndef FPDB_FPDB_EXECUTOR_INCLUDE_FPDB_EXECUTOR_PHYSICAL_BLOOMFILTER_BLOOMFILTERCREATEPOP_H
#define FPDB_FPDB_EXECUTOR_INCLUDE_FPDB_EXECUTOR_PHYSICAL_BLOOMFILTER_BLOOMFILTERCREATEPOP_H

#include <fpdb/executor/physical/PhysicalOp.h>
#include <fpdb/executor/physical/bloomfilter/BloomFilterCreateAbstractKernel.h>
#include <fpdb/executor/physical/bloomfilter/BloomFilter.h>
#include <fpdb/executor/physical/fpdb-store/FPDBStoreBloomFilterInfo.h>

namespace fpdb::executor::physical::bloomfilter {

class BloomFilterCreatePOp: public PhysicalOp {

public:
  explicit BloomFilterCreatePOp(const std::string &name,
                                const std::vector<std::string> &projectColumnNames,
                                int nodeId,
                                const std::vector<std::string> &bloomFilterColumnNames,
                                double desiredFalsePositiveRate = BloomFilter::DefaultDesiredFalsePositiveRate);
  BloomFilterCreatePOp() = default;
  BloomFilterCreatePOp(const BloomFilterCreatePOp&) = default;
  BloomFilterCreatePOp& operator=(const BloomFilterCreatePOp&) = default;
  ~BloomFilterCreatePOp() = default;

  void onReceive(const Envelope &envelope) override;
  void clear() override;
  std::string getTypeString() const override;
  void produce(const std::shared_ptr<PhysicalOp> &op) override;

  const std::shared_ptr<BloomFilterCreateAbstractKernel> &getKernel() const;
  const std::set<std::string> &getBloomFilterUsePOps() const;
  const std::set<std::string> &getPassTupleSetConsumers() const;

  void setBloomFilterUsePOps(const std::set<std::string> &bloomFilterUsePOps);
  void setPassTupleSetConsumers(const std::set<std::string> &passTupleSetConsumers);
  void addBloomFilterUsePOp(const std::shared_ptr<PhysicalOp> &bloomFilterUsePOp);
  void addFPDBStoreBloomFilterConsumer(const std::shared_ptr<PhysicalOp> &fpdbStoreBloomFilterConsumer);
  void setBloomFilterInfo(const fpdb_store::FPDBStoreBloomFilterCreateInfo &bloomFilterInfo);

private:
  void onStart();
  void onTupleSet(const TupleSetMessage &msg);
  void onComplete(const CompleteMessage &);
  void putBloomFilterToStore(const std::shared_ptr<BloomFilterBase> &bloomFilter);
  void notifyFPDBStoreBloomFilterUsers();

  std::shared_ptr<BloomFilterCreateAbstractKernel> kernel_;
  std::set<std::string> bloomFilterUsePOps_;
  std::set<std::string> passTupleSetConsumers_;

  // set only when pushing down bloom filter
  std::optional<fpdb_store::FPDBStoreBloomFilterCreateInfo> bloomFilterInfo_;
  std::set<std::string> fpdbStoreBloomFilterConsumers_;

// caf inspect
public:
  template <class Inspector>
  friend bool inspect(Inspector& f, BloomFilterCreatePOp& op) {
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
                               f.field("kernel", op.kernel_),
                               f.field("bloomFilterUsePOps", op.bloomFilterUsePOps_),
                               f.field("passTupleSetConsumers", op.passTupleSetConsumers_),
                               f.field("bloomFilterInfo", op.bloomFilterInfo_),
                               f.field("fpdbStoreBloomFilterConsumers", op.fpdbStoreBloomFilterConsumers_));
  }
};

}


#endif //FPDB_FPDB_EXECUTOR_INCLUDE_FPDB_EXECUTOR_PHYSICAL_BLOOMFILTER_BLOOMFILTERCREATEPOP_H

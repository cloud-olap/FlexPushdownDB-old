//
// Created by Yifei Yang on 2/26/22.
//

#include <fpdb/plan/prephysical/separable/SeparablePrePOpTransformer.h>
#include <fpdb/plan/prephysical/HashJoinPrePOp.h>
#include <fpdb/catalogue/obj-store/ObjStoreCatalogueEntry.h>

namespace fpdb::plan::prephysical::separable {

SeparablePrePOpTransformer::SeparablePrePOpTransformer(const std::shared_ptr<CatalogueEntry> &catalogueEntry) {
  setSeparableTraits(catalogueEntry);
}

void SeparablePrePOpTransformer::transform(const std::shared_ptr<PrePhysicalPlan> &prePhysicalPlan) {
  // transform in DFS
  auto optSeparableSuperPrePOp = transformDfs(prePhysicalPlan->getRootOp());

  // if the plan is completely separable from the root, then we need to set the root op,
  // otherwise it's already set during transformDfs()
  if (optSeparableSuperPrePOp.has_value()) {
    prePhysicalPlan->setRootOp(*optSeparableSuperPrePOp);
  }
}

void SeparablePrePOpTransformer::setSeparableTraits(const std::shared_ptr<CatalogueEntry> &catalogueEntry) {
  switch (catalogueEntry->getType()) {
    case CatalogueEntryType::LOCAL_FS: {
      separableTraits_ = SeparableTraits::localFSSeparableTraits();
      return;
    }
    case CatalogueEntryType::OBJ_STORE: {
      auto objStoreCatalogueEntry = std::static_pointer_cast<obj_store::ObjStoreCatalogueEntry>(catalogueEntry);
      switch (objStoreCatalogueEntry->getStoreType()) {
        case obj_store::ObjStoreType::S3: {
          separableTraits_ = SeparableTraits::S3SeparableTraits();
          return;
        }
        case obj_store::ObjStoreType::FPDB_STORE: {
          separableTraits_ = SeparableTraits::FPDBStoreSeparableTraits();
          return;
        }
        default: {
          separableTraits_ = SeparableTraits::unknownStoreSeparableTraits();
          return;
        }
      }
    }
    default: {
      separableTraits_ = SeparableTraits::unknownStoreSeparableTraits();
      return;
    }
  }
}

std::optional<std::shared_ptr<SeparableSuperPrePOp>>
SeparablePrePOpTransformer::transformDfs(const std::shared_ptr<PrePhysicalOp> &op) {
  // transform recursively
  std::vector<std::shared_ptr<PrePhysicalOp>> newProducers;
  std::vector<std::shared_ptr<SeparableSuperPrePOp>> producerSeparablePrePOps;
  for (const auto &producer: op->getProducers()) {
    auto optSeparableSuperPrePOp = transformDfs(producer);
    if (optSeparableSuperPrePOp.has_value()) {
      producerSeparablePrePOps.emplace_back(*optSeparableSuperPrePOp);
      newProducers.emplace_back(*optSeparableSuperPrePOp);
    } else {
      newProducers.emplace_back(producer);
    }
  }

  bool separable;
  if (op->getType() == PrePOpType::HASH_JOIN) {
    separable = std::static_pointer_cast<HashJoinPrePOp>(op)->isPushable()
            && separableTraits_->isSeparable(PrePOpType::HASH_JOIN);
  } else {
    separable = separableTraits_->isSeparable(op->getType());
  }

  if (separable && producerSeparablePrePOps.size() == newProducers.size()) {
    // if separable and each producer forms a SeparableSuperPrePOp, then combine them as a larger one
    newProducers.clear();
    for (const auto &producerSeparablePrePOp: producerSeparablePrePOps) {
      newProducers.emplace_back(producerSeparablePrePOp->getRootOp());
    }
    op->setProducers(newProducers);
    return std::make_shared<SeparableSuperPrePOp>(op->getId(), op);
  } else {
    // otherwise, just update producers
    op->setProducers(newProducers);
    return std::nullopt;
  }
}

}

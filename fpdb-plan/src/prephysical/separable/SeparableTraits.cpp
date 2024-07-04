//
// Created by Yifei Yang on 2/26/22.
//

#include <fpdb/plan/prephysical/separable/SeparableTraits.h>
#include "fpdb/plan/prephysical/separable/Globals.h"

namespace fpdb::plan::prephysical::separable {

SeparableTraits::SeparableTraits(const std::set<PrePOpType> &separablePrePOpTypes):
  separablePrePOpTypes_(separablePrePOpTypes) {}

std::shared_ptr<SeparableTraits> SeparableTraits::S3SeparableTraits() {
  return std::make_shared<SeparableTraits>(std::set<PrePOpType>{
    FILTERABLE_SCAN
  });
}

std::shared_ptr<SeparableTraits> SeparableTraits::FPDBStoreSeparableTraits() {
  // GROUP is excluded because it's separable only when using two-phase group-by, and only the first phase is separable,
  // so that flag is set by 'StoreTransformTraits'.
  std::set<PrePOpType> separablePrePOpTypes{FILTERABLE_SCAN, FILTER, AGGREGATE, PROJECT};
  if (ENABLE_CO_LOCATED_JOIN_PUSHDOWN) {
    separablePrePOpTypes.emplace(HASH_JOIN);
  }
  return std::make_shared<SeparableTraits>(separablePrePOpTypes);
}

std::shared_ptr<SeparableTraits> SeparableTraits::localFSSeparableTraits() {
  return std::make_shared<SeparableTraits>(std::set<PrePOpType>{});
}

std::shared_ptr<SeparableTraits> SeparableTraits::unknownStoreSeparableTraits() {
  return std::make_shared<SeparableTraits>(std::set<PrePOpType>{});
}

bool SeparableTraits::isSeparable(PrePOpType prePOpType) const {
  return separablePrePOpTypes_.find(prePOpType) != separablePrePOpTypes_.end();
}

}

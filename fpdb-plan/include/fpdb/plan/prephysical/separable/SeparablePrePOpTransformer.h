//
// Created by Yifei Yang on 2/26/22.
//

#ifndef FPDB_FPDB_PLAN_INCLUDE_FPDB_PLAN_PREPHYSICAL_SEPARABLEPREPOPTRANSFORMER_H
#define FPDB_FPDB_PLAN_INCLUDE_FPDB_PLAN_PREPHYSICAL_SEPARABLEPREPOPTRANSFORMER_H

#include <fpdb/plan/prephysical/separable/SeparableSuperPrePOp.h>
#include <fpdb/plan/prephysical/separable/SeparableTraits.h>
#include <fpdb/plan/prephysical/PrePhysicalPlan.h>
#include <fpdb/catalogue/CatalogueEntry.h>
#include <tl/expected.hpp>

using namespace fpdb::plan::prephysical;
using namespace fpdb::catalogue;

namespace fpdb::plan::prephysical::separable {

/**
 * Transform a group of PrePhysicalOp into a SeparablePrePOp, according to SeparableTraits of the underlying store
 */
class SeparablePrePOpTransformer {

public:
  SeparablePrePOpTransformer(const std::shared_ptr<CatalogueEntry> &catalogueEntry);

  void transform(const std::shared_ptr<PrePhysicalPlan> &prePhysicalPlan);

private:
  void setSeparableTraits(const std::shared_ptr<CatalogueEntry> &catalogueEntry);

  std::optional<std::shared_ptr<SeparableSuperPrePOp>> transformDfs(const std::shared_ptr<PrePhysicalOp> &op);

  std::shared_ptr<SeparableTraits> separableTraits_;

};

}


#endif //FPDB_FPDB_PLAN_INCLUDE_FPDB_PLAN_PREPHYSICAL_SEPARABLEPREPOPTRANSFORMER_H

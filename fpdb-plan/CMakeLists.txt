
#-------------------------------------------------------------------------------------------------------------
# Target
#-----------------------------------------------------------------------------------------------------------------------

add_library(fpdb-plan STATIC
        include/fpdb/plan/prephysical/PrePOpType.h
        src/calcite/CalcitePlanJsonDeserializer.cpp include/fpdb/plan/calcite/CalcitePlanJsonDeserializer.h
        src/prephysical/PrePhysicalPlan.cpp include/fpdb/plan/prephysical/PrePhysicalPlan.h
        src/prephysical/PrePhysicalOp.cpp include/fpdb/plan/prephysical/PrePhysicalOp.h
        src/prephysical/SortPrePOp.cpp include/fpdb/plan/prephysical/SortPrePOp.h
        include/fpdb/plan/prephysical/SortOrder.h
        src/prephysical/SortKey.cpp include/fpdb/plan/prephysical/SortKey.h
        src/prephysical/LimitSortPrePOp.cpp include/fpdb/plan/prephysical/LimitSortPrePOp.h
        include/fpdb/plan/prephysical/AggregatePrePFunctionType.h
        src/prephysical/AggregatePrePFunction.cpp include/fpdb/plan/prephysical/AggregatePrePFunction.h
        src/prephysical/AggregatePrePOp.cpp include/fpdb/plan/prephysical/AggregatePrePOp.h
        src/prephysical/GroupPrePOp.cpp include/fpdb/plan/prephysical/GroupPrePOp.h
        include/fpdb/plan/prephysical/JoinType.h
        src/prephysical/HashJoinPrePOp.cpp include/fpdb/plan/prephysical/HashJoinPrePOp.h
        src/prephysical/NestedLoopJoinPrePOp.cpp include/fpdb/plan/prephysical/NestedLoopJoinPrePOp.h
        src/prephysical/ProjectPrePOp.cpp include/fpdb/plan/prephysical/ProjectPrePOp.h
        src/prephysical/FilterPrePOp.cpp include/fpdb/plan/prephysical/FilterPrePOp.h
        src/prephysical/FilterableScanPrePOp.cpp include/fpdb/plan/prephysical/FilterableScanPrePOp.h
        include/fpdb/plan/prephysical/separable/Globals.h
        src/prephysical/separable/SeparableSuperPrePOp.cpp include/fpdb/plan/prephysical/separable/SeparableSuperPrePOp.h
        src/prephysical/separable/SeparablePrePOpTransformer.cpp include/fpdb/plan/prephysical/separable/SeparablePrePOpTransformer.h
        src/prephysical/separable/SeparableTraits.cpp include/fpdb/plan/prephysical/separable/SeparableTraits.h
        src/Mode.cpp include/fpdb/plan/Mode.h
        src/Util.cpp include/fpdb/plan/Util.h)

target_include_directories(fpdb-plan PUBLIC include)

set_target_properties(fpdb-plan PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_link_libraries(fpdb-plan PUBLIC fpdb-calcite)
target_link_libraries(fpdb-plan PUBLIC fpdb-catalogue)
target_link_libraries(fpdb-plan PUBLIC fpdb-expression-gandiva)


#-----------------------------------------------------------------------------------------------------------------------
# Diagnostics
#-----------------------------------------------------------------------------------------------------------------------

#showTargetProps(fpdb-plan)

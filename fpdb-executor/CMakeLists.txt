#-----------------------------------------------------------------------------------------------------------------------
# Target
#-----------------------------------------------------------------------------------------------------------------------

add_library(fpdb-executor STATIC
#        messages
        src/message/Envelope.cpp include/fpdb/executor/message/Envelope.h
        include/fpdb/executor/message/MessageType.h
        src/message/Message.cpp include/fpdb/executor/message/Message.h
        src/message/StartMessage.cpp include/fpdb/executor/message/StartMessage.h
        src/message/ConnectMessage.cpp include/fpdb/executor/message/ConnectMessage.h
        src/message/CompleteMessage.cpp include/fpdb/executor/message/CompleteMessage.h
        src/message/ScanMessage.cpp include/fpdb/executor/message/ScanMessage.h
        src/message/TupleSetMessage.cpp include/fpdb/executor/message/TupleSetMessage.h
        src/message/TupleSetBufferMessage.cpp include/fpdb/executor/message/TupleSetBufferMessage.h
        src/message/TupleSetIndexMessage.cpp include/fpdb/executor/message/TupleSetIndexMessage.h
        src/message/TupleSetReadyRemoteMessage.cpp include/fpdb/executor/message/TupleSetReadyRemoteMessage.h
        src/message/TupleSetWaitRemoteMessage.cpp include/fpdb/executor/message/TupleSetWaitRemoteMessage.h
        src/message/TupleSetSizeMessage.cpp include/fpdb/executor/message/TupleSetSizeMessage.h
        src/message/TransferMetricsMessage.cpp include/fpdb/executor/message/TransferMetricsMessage.h
        src/message/DiskMetricsMessage.cpp include/fpdb/executor/message/DiskMetricsMessage.h
        src/message/ErrorMessage.cpp include/fpdb/executor/message/ErrorMessage.h
        src/message/BloomFilterMessage.cpp include/fpdb/executor/message/BloomFilterMessage.h
        src/message/BitmapMessage.cpp include/fpdb/executor/message/BitmapMessage.h
        src/message/AdaptPushdownMetricsMessage.cpp include/fpdb/executor/message/AdaptPushdownMetricsMessage.h
        src/message/PushdownFallBackMessage.cpp include/fpdb/executor/message/PushdownFallBackMessage.h
        src/message/cache/CacheMetricsMessage.cpp include/fpdb/executor/message/cache/CacheMetricsMessage.h
        src/message/cache/LoadRequestMessage.cpp include/fpdb/executor/message/cache/LoadRequestMessage.h
        src/message/cache/LoadResponseMessage.cpp include/fpdb/executor/message/cache/LoadResponseMessage.h
        src/message/cache/StoreRequestMessage.cpp include/fpdb/executor/message/cache/StoreRequestMessage.h
        src/message/cache/WeightRequestMessage.cpp include/fpdb/executor/message/cache/WeightRequestMessage.h
#        base physical operators, operator actors, segment cache actors and required sources
        src/cache/SegmentCacheActor.cpp include/fpdb/executor/cache/SegmentCacheActor.h
        include/fpdb/executor/physical/Forward.h
        include/fpdb/executor/physical/POpRelationshipType.h
        src/physical/POpConnection.cpp include/fpdb/executor/physical/POpConnection.h
        src/physical/LocalPOpDirectoryEntry.cpp include/fpdb/executor/physical/LocalPOpDirectoryEntry.h
        src/physical/LocalPOpDirectory.cpp include/fpdb/executor/physical/LocalPOpDirectory.h
        src/physical/POpDirectory.cpp include/fpdb/executor/physical/POpDirectory.h
        src/physical/POpDirectoryEntry.cpp include/fpdb/executor/physical/POpDirectoryEntry.h
        src/physical/POpActor.cpp include/fpdb/executor/physical/POpActor.h
        include/fpdb/executor/physical/POpActor2.h
        src/physical/POpContext.cpp include/fpdb/executor/physical/POpContext.h
        src/physical/PhysicalOp.cpp include/fpdb/executor/physical/PhysicalOp.h
        include/fpdb/executor/physical/POpType.h
#        table communication across the compute cluster using flight
        src/flight/FlightHandler.cpp include/fpdb/executor/flight/FlightHandler.h
        src/flight/FlightClients.cpp include/fpdb/executor/flight/FlightClients.h
        src/flight/TableCache.cpp include/fpdb/executor/flight/TableCache.h
#        physical operators
        include/fpdb/executor/physical/Globals.h
        src/physical/sort/SortPOp.cpp include/fpdb/executor/physical/sort/SortPOp.h
        src/physical/limitsort/LimitSortPOp.cpp include/fpdb/executor/physical/limitsort/LimitSortPOp.h
        src/physical/aggregate/AggregatePOp.cpp include/fpdb/executor/physical/aggregate/AggregatePOp.h
        src/physical/aggregate/AggregateResult.cpp include/fpdb/executor/physical/aggregate/AggregateResult.h
        include/fpdb/executor/physical/aggregate/function/AggregateFunctionType.h
        src/physical/aggregate/function/AggregateFunction.cpp include/fpdb/executor/physical/aggregate/function/AggregateFunction.h
        src/physical/aggregate/function/Sum.cpp include/fpdb/executor/physical/aggregate/function/Sum.h
        src/physical/aggregate/function/Count.cpp include/fpdb/executor/physical/aggregate/function/Count.h
        src/physical/aggregate/function/MinMax.cpp include/fpdb/executor/physical/aggregate/function/MinMax.h
        src/physical/aggregate/function/AvgBase.cpp include/fpdb/executor/physical/aggregate/function/AvgBase.h
        src/physical/aggregate/function/Avg.cpp include/fpdb/executor/physical/aggregate/function/Avg.h
        src/physical/aggregate/function/AvgReduce.cpp include/fpdb/executor/physical/aggregate/function/AvgReduce.h
        src/physical/bloomfilter/BloomFilterCreatePOp.cpp include/fpdb/executor/physical/bloomfilter/BloomFilterCreatePOp.h
        src/physical/bloomfilter/BloomFilterCreateAbstractKernel.cpp include/fpdb/executor/physical/bloomfilter/BloomFilterCreateAbstractKernel.h
        src/physical/bloomfilter/BloomFilterCreateKernel.cpp include/fpdb/executor/physical/bloomfilter/BloomFilterCreateKernel.h
        src/physical/bloomfilter/BloomFilterCreateArrowKernel.cpp include/fpdb/executor/physical/bloomfilter/BloomFilterCreateArrowKernel.h
        src/physical/bloomfilter/BloomFilterUsePOp.cpp include/fpdb/executor/physical/bloomfilter/BloomFilterUsePOp.h
        src/physical/bloomfilter/BloomFilterUseKernel.cpp include/fpdb/executor/physical/bloomfilter/BloomFilterUseKernel.h
        src/physical/bloomfilter/BloomFilter.cpp include/fpdb/executor/physical/bloomfilter/BloomFilter.h
        src/physical/bloomfilter/BloomFilterBase.cpp include/fpdb/executor/physical/bloomfilter/BloomFilterBase.h
        src/physical/bloomfilter/ArrowBloomFilter.cpp include/fpdb/executor/physical/bloomfilter/ArrowBloomFilter.h
        src/physical/bloomfilter/UniversalHashFunction.cpp include/fpdb/executor/physical/bloomfilter/UniversalHashFunction.h
        src/physical/cache/CacheLoadPOp.cpp include/fpdb/executor/physical/cache/CacheLoadPOp.h
        src/physical/cache/CacheHelper.cpp include/fpdb/executor/physical/cache/CacheHelper.h
        src/physical/collate/CollatePOp.cpp include/fpdb/executor/physical/collate/CollatePOp.h
        src/physical/collate/CollatePOp2.cpp include/fpdb/executor/physical/collate/CollatePOp2.h
        src/physical/collect/CollectPOp.cpp include/fpdb/executor/physical/collect/CollectPOp.h
        src/physical/file/FileScanAbstractPOp.cpp include/fpdb/executor/physical/file/FileScanAbstractPOp.h
        src/physical/file/LocalFileScanPOp.cpp include/fpdb/executor/physical/file/LocalFileScanPOp.h
        src/physical/file/RemoteFileScanPOp.cpp include/fpdb/executor/physical/file/RemoteFileScanPOp.h
        src/physical/file/FileScanKernel.cpp include/fpdb/executor/physical/file/FileScanKernel.h
        src/physical/file/LocalFileScanKernel.cpp include/fpdb/executor/physical/file/LocalFileScanKernel.h
        src/physical/file/RemoteFileScanKernel.cpp include/fpdb/executor/physical/file/RemoteFileScanKernel.h
        src/physical/file/FileScanPOp2.cpp include/fpdb/executor/physical/file/FileScanPOp2.h
        src/physical/filter/FilterPOp.cpp include/fpdb/executor/physical/filter/FilterPOp.h
        src/physical/group/GroupPOp.cpp include/fpdb/executor/physical/group/GroupPOp.h
        include/fpdb/executor/physical/group/GroupKernelType.h
        src/physical/group/GroupAbstractKernel.cpp include/fpdb/executor/physical/group/GroupAbstractKernel.h
        src/physical/group/GroupKernel.cpp include/fpdb/executor/physical/group/GroupKernel.h
        src/physical/group/GroupArrowKernel.cpp include/fpdb/executor/physical/group/GroupArrowKernel.h
        src/physical/join/hashjoin/HashJoinBuildPOp.cpp include/fpdb/executor/physical/join/hashjoin/HashJoinBuildPOp.h
        src/physical/join/hashjoin/HashJoinProbePOp.cpp include/fpdb/executor/physical/join/hashjoin/HashJoinProbePOp.h
        src/physical/join/hashjoin/HashJoinBuildKernel.cpp include/fpdb/executor/physical/join/hashjoin/HashJoinBuildKernel.h
        src/physical/join/hashjoin/HashJoinProbeAbstractKernel.cpp include/fpdb/executor/physical/join/hashjoin/HashJoinProbeAbstractKernel.h
        src/physical/join/hashjoin/HashJoinProbeKernel.cpp include/fpdb/executor/physical/join/hashjoin/HashJoinProbeKernel.h
        src/physical/join/hashjoin/HashSemiJoinProbeKernel.cpp include/fpdb/executor/physical/join/hashjoin/HashSemiJoinProbeKernel.h
        src/physical/join/hashjoin/HashJoinPredicate.cpp include/fpdb/executor/physical/join/hashjoin/HashJoinPredicate.h
        src/physical/join/hashjoin/RecordBatchHashJoiner.cpp include/fpdb/executor/physical/join/hashjoin/RecordBatchHashJoiner.h
        src/physical/join/hashjoin/RecordBatchHashSemiJoiner.cpp include/fpdb/executor/physical/join/hashjoin/RecordBatchHashSemiJoiner.h
        src/physical/join/hashjoin/HashJoinArrowPOp.cpp include/fpdb/executor/physical/join/hashjoin/HashJoinArrowPOp.h
        src/physical/join/hashjoin/HashJoinArrowKernel.cpp include/fpdb/executor/physical/join/hashjoin/HashJoinArrowKernel.h
        src/physical/join/nestedloopjoin/NestedLoopJoinPOp.cpp include/fpdb/executor/physical/join/nestedloopjoin/NestedLoopJoinPOp.h
        src/physical/join/nestedloopjoin/NestedLoopJoinKernel.cpp include/fpdb/executor/physical/join/nestedloopjoin/NestedLoopJoinKernel.h
        src/physical/join/nestedloopjoin/RecordBatchNestedLoopJoiner.cpp include/fpdb/executor/physical/join/nestedloopjoin/RecordBatchNestedLoopJoiner.h
        src/physical/join/OuterJoinHelper.cpp include/fpdb/executor/physical/join/OuterJoinHelper.h
        src/physical/merge/MergePOp.cpp include/fpdb/executor/physical/merge/MergePOp.h
        src/physical/merge/MergeKernel.cpp include/fpdb/executor/physical/merge/MergeKernel.h
        src/physical/project/ProjectPOp.cpp include/fpdb/executor/physical/project/ProjectPOp.h
        src/physical/shuffle/ShufflePOp.cpp include/fpdb/executor/physical/shuffle/ShufflePOp.h
        src/physical/shuffle/ShuffleKernel.cpp include/fpdb/executor/physical/shuffle/ShuffleKernel.h
        src/physical/shuffle/ShuffleKernel2.cpp include/fpdb/executor/physical/shuffle/ShuffleKernel2.h
        src/physical/shuffle/RecordBatchShuffler.cpp include/fpdb/executor/physical/shuffle/RecordBatchShuffler.h
        src/physical/shuffle/ShuffleBatchLoadPOp.cpp include/fpdb/executor/physical/shuffle/ShuffleBatchLoadPOp.h
        src/physical/split/SplitPOp.cpp include/fpdb/executor/physical/split/SplitPOp.h
        src/physical/s3/S3GetPOp.cpp include/fpdb/executor/physical/s3/S3GetPOp.h
        src/physical/s3/S3SelectPOp.cpp include/fpdb/executor/physical/s3/S3SelectPOp.h
        src/physical/s3/S3SelectScanAbstractPOp.cpp include/fpdb/executor/physical/s3/S3SelectScanAbstractPOp.h
        src/physical/s3/S3CSVParser.cpp include/fpdb/executor/physical/s3/S3CSVParser.h
        src/physical/s3/S3SelectCSVParseOptions.cpp include/fpdb/executor/physical/s3/S3SelectCSVParseOptions.h
        src/physical/s3/S3SelectScanKernel.cpp include/fpdb/executor/physical/s3/S3SelectScanKernel.h
        src/physical/s3/SelectPOp.cpp include/fpdb/executor/physical/s3/SelectPOp.h
        src/physical/fpdb-store/FPDBStoreSuperPOp.cpp include/fpdb/executor/physical/fpdb-store/FPDBStoreSuperPOp.h
        src/physical/fpdb-store/FPDBStoreFileScanPOp.cpp include/fpdb/executor/physical/fpdb-store/FPDBStoreFileScanPOp.h
        src/physical/fpdb-store/FPDBStoreTableCacheLoadPOp.cpp include/fpdb/executor/physical/fpdb-store/FPDBStoreTableCacheLoadPOp.h
        src/physical/fpdb-store/FPDBStoreSuperPOpUtil.cpp include/fpdb/executor/physical/fpdb-store/FPDBStoreSuperPOpUtil.h
        src/physical/fpdb-store/FPDBStoreFilterBitmapWrapper.cpp include/fpdb/executor/physical/fpdb-store/FPDBStoreFilterBitmapWrapper.h
        src/physical/fpdb-store/FPDBStoreBloomFilterInfo.cpp include/fpdb/executor/physical/fpdb-store/FPDBStoreBloomFilterInfo.h
#        physical plan
        src/physical/PhysicalPlan.cpp include/fpdb/executor/physical/PhysicalPlan.h
        src/physical/prune/PartitionPruner.cpp include/fpdb/executor/physical/prune/PartitionPruner.h
        src/physical/transform/PrePToPTransformer.cpp include/fpdb/executor/physical/transform/PrePToPTransformer.h
        src/physical/transform/PrePToS3PTransformer.cpp include/fpdb/executor/physical/transform/PrePToS3PTransformer.h
        src/physical/transform/PrePToFPDBStorePTransformer.cpp include/fpdb/executor/physical/transform/PrePToFPDBStorePTransformer.h
        src/physical/transform/PrePToPTransformerUtil.cpp include/fpdb/executor/physical/transform/PrePToPTransformerUtil.h
        src/physical/transform/StoreTransformTraits.cpp include/fpdb/executor/physical/transform/StoreTransformTraits.h
#        CAF serialization
        include/fpdb/executor/caf-serialization/CAFMessageSerializer.h
        include/fpdb/executor/caf-serialization/CAFPOpSerializer.h
        include/fpdb/executor/caf-serialization/CAFAggregateFunctionSerializer.h
        include/fpdb/executor/caf-serialization/CAFHashJoinProbeAbstractKernelSerializer.h
        include/fpdb/executor/caf-serialization/CAFFileScanKernelSerializer.h
        include/fpdb/executor/caf-serialization/CAFGroupAbstractKernelSerializer.h
        include/fpdb/executor/caf-serialization/CAFBloomFilterCreateAbstractKernelSerializer.h
        include/fpdb/executor/caf-serialization/CAFBloomFilterSerializer.h
#        serialization
        src/physical/serialization/PhysicalPlanSerializer.cpp include/fpdb/executor/physical/serialization/PhysicalPlanSerializer.h
        src/physical/serialization/PhysicalPlanDeserializer.cpp include/fpdb/executor/physical/serialization/PhysicalPlanDeserializer.h
#        CAF initialization
        src/caf/CAFInit.cpp include/fpdb/executor/caf/CAFInit.h
        src/caf/CAFAdaptPushdownUtil.cpp include/fpdb/executor/caf/CAFAdaptPushdownUtil.h
#        metrics
        include/fpdb/executor/metrics/Globals.h
        src/metrics/DebugMetrics.cpp include/fpdb/executor/metrics/DebugMetrics.h
        src/metrics/TransferMetrics.cpp include/fpdb/executor/metrics/TransferMetrics.h
        src/metrics/DiskMetrics.cpp include/fpdb/executor/metrics/DiskMetrics.h
#        executor
        src/Globals.cpp include/fpdb/executor/Globals.h
        src/Executor.cpp include/fpdb/executor/Executor.h
        src/Execution.cpp include/fpdb/executor/Execution.h
        src/FPDBStoreExecution.cpp include/fpdb/executor/FPDBStoreExecution.h
        src/CollAdaptPushdownMetricsExecution.cpp include/fpdb/executor/CollAdaptPushdownMetricsExecution.h)

target_include_directories(fpdb-executor PUBLIC include)

target_link_libraries(fpdb-executor PUBLIC fpdb-plan)
target_link_libraries(fpdb-executor PUBLIC fpdb-cache)
target_link_libraries(fpdb-executor PUBLIC fpdb-caf)
target_link_libraries(fpdb-executor PUBLIC graphviz_gvc_shared)
target_link_libraries(fpdb-executor PUBLIC graphviz_dot_layout_shared)
target_link_libraries(fpdb-executor PUBLIC graphviz_neato_layout_shared)
target_link_libraries(fpdb-executor PUBLIC primesieve_static)
target_link_libraries(fpdb-executor PRIVATE fpdb-store-server-file)
target_link_libraries(fpdb-executor PUBLIC fpdb-store-server-flight-api)


#-----------------------------------------------------------------------------------------------------------------------
# Microbenchmark
#-----------------------------------------------------------------------------------------------------------------------

add_executable(fpdb-executor-bench
        bench/Globals.h
        bench/MainBenchmark.cpp
        bench/GroupBenchmark.cpp
        bench/HashJoinBenchmark.cpp
        bench/ShuffleBenchmark.cpp
        bench/BloomFilterBenchmark.cpp)

target_include_directories(fpdb-executor-bench PRIVATE include)

target_link_libraries(fpdb-executor-bench spdlog::spdlog)
target_link_libraries(fpdb-executor-bench doctest::doctest)
target_link_libraries(fpdb-executor-bench nanobench::nanobench)
target_link_libraries(fpdb-executor-bench fpdb-executor)


#-----------------------------------------------------------------------------------------------------------------------
# Executable
#-----------------------------------------------------------------------------------------------------------------------

add_executable(fpdb-executor-hash-partitioner-exec
        executable/HashPartitionerMain.cpp
        src/physical/shuffle/ShuffleKernel.cpp include/fpdb/executor/physical/shuffle/ShuffleKernel.h
        src/physical/shuffle/ShuffleKernel2.cpp include/fpdb/executor/physical/shuffle/ShuffleKernel2.h
        src/physical/shuffle/RecordBatchShuffler.cpp include/fpdb/executor/physical/shuffle/RecordBatchShuffler.h)

target_include_directories(fpdb-executor-hash-partitioner-exec PRIVATE include)

target_link_libraries(fpdb-executor-hash-partitioner-exec fpdb-tuple)
target_link_libraries(fpdb-executor-hash-partitioner-exec fpdb-aws)
target_link_libraries(fpdb-executor-hash-partitioner-exec nlohmann_json)


#-----------------------------------------------------------------------------------------------------------------------
# Diagnostics
#-----------------------------------------------------------------------------------------------------------------------

#showTargetProps(fpdb-executor)

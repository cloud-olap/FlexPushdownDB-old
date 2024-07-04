# FlexPushdownDB

-----------------

A research cloud OLAP engine using hybrid caching and computation pushdown.

[1] Yifei Yang, Matt Youill, Matthew Woicik, Yizhou Liu, Xiangyao Yu, Marco Serafini, Ashraf Aboulnaga, Michael Stonebraker, FlexPushdownDB: Hybrid Pushdown and Caching in a Cloud DBMS, VLDB 2021.


## Clone the Repo

```
git clone https://github.com/cloud-olap/FlexPushdownDB-Dev.git
cd FlexPushdownDB-Dev
```


## Set up the System

Compiler needed: 
- Linux: LLVM-12 or later versions.
- Mac OS: LLVM-13.

#### To set up the system locally to develop:

1. Install required dependency. For Ubuntu, `./tools/project/bin/ubuntu-prerequisites.sh`. For other Linux OS or Mac OS, required dependencies listed in `tools/project/bin/ubuntu-prerequisites.sh` have to be manually installed.
3. Set `RESOURCE_PATH` of `fpdb-calcite/java/main/resources/config/exec.conf` to the path of `resources`.
3. Build the system `./resources/script/build.sh`.

#### To set up the system in an EC2 cluster:

1. Create a cluster of EC2 nodes, within which one is the coordinator, the others are executors. The coordinator is not necessarily as powerful as the executor.
2. Log in the coordinator. Put public IP of all nodes (including the coordinator) into `resources/config/cluster_ips`. Each line is one IP.
3. Set up the system `./resources/script/setup.sh`. If all required dependencies are already installed, the system can also be set up by `./resources/script/build.sh`, `./resources/script/deploy.sh`.

#### Configurable parameters in `resources/script/util.sh` used when setting up the system:

- `install_dependency` whether to install required dependencies when running `resources/script/setup.sh`.
- `clean` whether to clean before the build starts.
- `build_parallel` parallel degree used for CMake build.
- `build_dir_name` name of the directory for the built files.
- `deploy_dir_name` name of the directory used for deploying to and running on a cluster.
- `temp_dir_name` name of the directory to store temp files after the system starts.
- `pem_path` the pem file (AWS private key) used to log in created EC2 nodes.


## Prepare Data and Metadata
1. Set up AWS access credentials and config. Run `aws configure` or manually set `~/.aws/credentials` and `~/.aws/config`.
2. Organize data files as follows on one S3 bucket: 
   1) Under `resources/metadata/`, create a directory of which the absolute path denotes the schema name, e.g. data files under `s3://tpch-sf0.01/csv/` belong to schema `tpch-sf0.01/csv/`.
   2) For each table `T` with a single partition, name the file as `T.tbl` for CSV format or `T.parquet` for Parquet format; for each table `T` with multiple partitions, create a directory `T_sharded/` and put partition files named as `T.tbl.0/T.parquet.0`, `T.tbl.1/T.parquet.1`... into it.
3. Register data by creating metadata files under `resources/metadata/`. Create a directory named as the schema name, and organize metadata files as follows:
   1) `schema.json` For each table, specify `name`, `format`(`delimiter` if CSV), `numPartitions`, `fields`.
   2) `stats.json` For each table, specify `name`, `rowCount`, `apxColumnLength`(no need to be accurate, just an estimation, used by W-LFU).
   3) `zoneMap.json` For each table, specify `name`, collected min_max values of each partition on some columns, leave it empty if not collected.
   

## Run End-to-end Tests

#### To run tests locally in a single node:
1. Start the Calcite server `java -jar fpdb-calcite/java/target/flexpushdowndb.thrift.calcite-1.0-SNAPSHOT.jar &`.
2. `cd <build directory>/fpdb-main`.
3. Run tests `./fpdb-main-test -ts=<test-suite> -tc=<test-case>`, available single-node test suites are `ssb-sf1-single_node-no-parallel`, `tpch-sf0.01-single_node-no-parallel`, `tpch-sf0.01-single_node-parallel`.
4. When finished, stop the Calcite server.

#### To run tests in a cluster:
1. Make sure the security group of your nodes allows inbound and outbound traffic for TCP protocol at `CAF_SERVER_PORT` (4242 by default, can be changed by instructions below). 
2. `cd ~/FPDB-build/`.
3. Start the system `./resources/script/start.sh`.
4. `cd fpdb-main`.
5. Run tests `./fpdb-main-test -ts=<test-suite> -tc=<test-case>`, available test suites are `ssb-sf1-single_node-no-parallel`, `tpch-sf0.01-single_node-no-parallel`, `tpch-sf0.01-single_node-parallel`, `tpch-sf0.01-distributed`.
6. When finished, stop the system `~/FPDB-build/resources/script/stop.sh`.

#### Note:
1. On large tests, check "ulimit -n", make sure the output number is high enough, otherwise it may throw "Too many open files" exception.


## System Configurations

#### Execution config (resources/config/exec.conf):
- `S3_BUCKET` s3 bucket name where the data is in.
- `SCHEMA_NAME` the name of the schema to query.
- `CACHE_SIZE` size of local cache.
- `MODE` execution mode, can be one of `PULLUP`, `PUSHDOWN_ONLY`, `CACHING_ONLY`, `HYBRID`. Currently SSB tests support all modes, TPC-H tests only support `PULLUP` and `CACHING_ONLY`.
- `CACHING_POLICY` cache replacement policy, can be one of `LRU`, `LFU`, `LFU-S` (size normalized LFU), `W-LFU` (Weighted LFU).
- `PARALLEL_DEGREE` execution parallel degree of the same kind of operator (e.g. join), independent of data parallel degree which corresponds to the number of partitions of a table.
- `SHOW_OP_TIMES` whether to show execution times of each operator and each kind of operator.
- `SHOW_SCAN_METRICS` whether to show scan metrics like data load speed, selectivity, data conversion speed...
- `CAF_SERVER_PORT` port used by CAF for cross-node communications.
- `IS_DISTRIBUTED` whether the system runs across a cluster or just locally.

#### AWS config (resources/config/aws.conf):
- `S3_CLIENT_TYPE` type of S3 client, can be one of `S3`, `AIRMETTLE`, `MINIO`. For `AIRMETTLE` and `MINIO`, access key and endpoint need to be set in `fpdb-aws/src/AWSClient.cpp`.
- `NETWORK_LIMIT` used to throttle data loading from S3, set to 0 if not throttling.

#### Calcite config cpp side (resources/config/calcite.conf):
- `SERVER_PORT` port for the Calcite server.
- `JAR_NAME` built jar name of the Calcite server.

#### Calcite config java side (fpdb-calcite/java/main/resources/config/exec.conf):
- `SERVER_PORT` port for the Calcite server, should be kept same as cpp side.
- `RESOURCE_PATH` absolute path of `resources/` used for metadata fetching.
- `exec.conf.ec2` is a fixed config for EC2 deployment and does not need to be changed.


## To Do
### Refactoring
- Use a single flight client instead of creating one for each request
- Check if file readers are too slow?
  - Use all SSD drives?
  - Use threads in Arrow readers?
  

# download data from s3 on all storage nodes
# run deploy.sh before running this

# import util
pushd "$(dirname "$0")" > /dev/null
cd ..
util_param_path=$(pwd)"/util_param.sh"
source "$util_param_path"
util_func_path=$(pwd)"/util_func.sh"
source "$util_func_path"
popd > /dev/null

# configurable parameters
data_relative_dirs=("tpch-sf50/parquet")

# add node ip to ssh
for node_ip in "${fpdb_store_ips[@]}"
do
  check_or_add_to_known_hosts "$node_ip"
done

# download data on each storage node in parallel
pids=()
for node_ip in "${fpdb_store_ips[@]}"
do
  run_command "$pem_path" "$node_ip" "$deploy_dir""/resources/script/fpdb-store/prepare_data_from_s3.sh" "${data_relative_dirs[@]}" &
  pids[${#pids[@]}]=$!
done

# wait
for pid in "${pids[@]}"
do
    wait $pid
done

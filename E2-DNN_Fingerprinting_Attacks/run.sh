#!/bin/bash

OUTPUT_BASE_DIR="./outdata"
HW_MON_NAME="exp4"
interface="/sys/bus/iio/devices/iio:device0/in_voltage10_vccpsddr_raw"


DEFAULT_TIME="1000"
COLLECT_TIMES=101
COLLECTION_TIME_US=1100000
PYTHON_DURATION_SEC=2

for model_file in models-*.txt; do
  suffix=${model_file#models-}
  suffix=${suffix%.txt}

  while IFS= read -r dnn_name || [[ -n "$dnn_name" ]]; do
    dnn_name=$(echo "$dnn_name" | sed 's/^[[:space:]]*//')
    [[ -z "$dnn_name" ]] && continue

    param="/usr/share/vitis_ai_library/models/${dnn_name}/${dnn_name}.xmodel"

    for ((i = 1; i <= COLLECT_TIMES; i++)); do
      echo "Launching Python inference on core 0"
      taskset -c 0 python3 ./start-$suffix.py "$param" "$PYTHON_DURATION_SEC" &
      python_pid=$!
      sleep 1s

      output_path="${OUTPUT_BASE_DIR}/${HW_MON_NAME}_${interface_name}/${dnn_name}/${i}.txt"
      echo ${output_path}
      mkdir -p "$(dirname "$output_path")"

      core_id=$((idx + 1))
      echo "Starting gather for $interface on core $core_id"
      ./exp "$interface" "$output_path" "$DEFAULT_TIME" "$COLLECTION_TIME_US" "$core_id" &
      echo {./exp "$output_path"}


      wait $python_pid
      sleep 1
    done
  done < "$model_file"
done

echo "All collections processed in $OUTPUT_BASE_DIR."

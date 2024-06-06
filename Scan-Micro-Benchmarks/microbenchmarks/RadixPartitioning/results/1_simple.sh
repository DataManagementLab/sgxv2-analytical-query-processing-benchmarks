#!/usr/bin/env bash

echo "Simple Bench"
export SGX_DBG_OPTIN=1

result_file="./paper-simple-data.csv"

for i in {0..0}
do
  echo "$i"
  numactl --physcpubind="$i" --membind=0 -- ./simple --modes loop_const --data_size=65536000 --min_output_size_exp 0 \
  --max_output_size_exp 15 --min_unrolling_factor 1 --max_unrolling_factor 16 --random_mode linear \
  | tail -n +$((i > 0 ? 2 : 1)) \
  | tee -a "$result_file"
done

echo "Done. Stored results in $result_file"

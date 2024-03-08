#!/usr/bin/env bash

echo "Radix Partitioning Bench"
export SGX_DBG_OPTIN=1

result_file="./histogram.csv"

for i in {0..9}
do
  echo "$i"
  numactl --physcpubind="$i" --membind=0 -- ./radixpart --unroll=false --repeat=1 --user_check=true --min_radix_bits=0 \
  --max_radix_bits=12 --s=65536000 --min_num_keys_exp=30 --max_num_keys_exp=30 \
  | tail -n +$((i > 0 ? 2 : 1)) \
  | tee -a "$result_file"
  numactl --physcpubind="$i" --membind=0 -- ./radixpart --unroll=true --repeat=1 --user_check=true --min_radix_bits=0 \
  --max_radix_bits=12 --s=65536000 --min_num_keys_exp=30 --max_num_keys_exp=30 \
  | tail -n +$((i > 0 ? 2 : 1)) \
  | tee -a "$result_file"
done

echo "Done. Stored results in $result_file"
